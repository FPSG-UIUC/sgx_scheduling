/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */



// App.cpp : Define the entry point for the console application.
//

#include <string.h>
#include <assert.h>
#include <fstream>
#include <pthread.h>
#include <iostream>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/syscall.h>

#include "Enclave_u.h"
#include "sgx_urts.h"
#include "sgx_tseal.h"

#include "rwlock.h"
#include "ErrorSupport.h"

#include "cifar10_reader.hpp"
#include "types.h"

#include "SideChannels.h"

#define ENCLAVE_NAME "libenclave.signed.so"
#define TOKEN_NAME "Enclave.token"

#define THREAD_NUM 2

// Global data
sgx_enclave_id_t global_eid[3] = {0, 1, 2};
sgx_launch_token_t token = {0};
rwlock_t lock_eid;
struct sealed_buf_t *sealed_buf;
struct data ds;

using namespace std;

// Ocall function
void print(const char *str)
{
    cout<<str;
}

void ioctl_set_msg(int file_desc, char *message, enum call_type type)
{
	if (SIDE_CHANNELS_ON == 0) {
		return;
	}

	int ret_val;

	switch (type) {
	case APPEND_ADDR:
		ret_val = ioctl(file_desc, IOCTL_APPEND_ADDR, message);
		break;
	case PASS_SPECIAL_ADDR:
		ret_val = ioctl(file_desc, IOCTL_PASS_SPECIAL_ADDR, message);
		break;
	case START_MONITORING:
		ret_val = ioctl(file_desc, IOCTL_START_MONITORING, message);
		break;
	case STOP_MONITORING:
		ret_val = ioctl(file_desc, IOCTL_STOP_MONITORING, message);
		break;
	case WAIT:
		ret_val = ioctl(file_desc, IOCTL_WAIT, message);
		break;
    case JOIN:
		ret_val = ioctl(file_desc, IOCTL_JOIN, message);
		break;
	default:
		printf("ioctl type not found\n");
		ret_val = -1;
		break;
	}

	if (ret_val < 0) {
		printf("ioctl failed:%d\n", ret_val);
		exit(-1);
	}
}

int file_desc = 0;
int setup_kernel_channel()
{
	// Open microscope ioctl device from kernel
	file_desc = open(DEVICE_FILE_NAME_PATH, 0);
	if (file_desc < 0) {
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME_PATH);
		exit(-1);
	}
}

int send_image_address(void *addr)
{
	char buffer[80], *msg = NULL;

	// Write the start of nuke_addr into the buffer
	sprintf(buffer, "%p", addr);
	msg = buffer;

	// Send ds address to ioctl device in the kernel
	ioctl_set_msg(file_desc, msg, APPEND_ADDR);
}

int send_model_address(void *addr)
{
	char buffer[80], *msg = NULL;

	// Write the start of nuke_addr into the buffer
	sprintf(buffer, "%p", addr);
	msg = buffer;

	// Send model address to ioctl device in the kernel
	ioctl_set_msg(file_desc, msg, PASS_SPECIAL_ADDR);
}

int start_controlled_side_channel(void)
{
	ioctl_set_msg(file_desc, NULL, START_MONITORING);
}

int stop_controlled_side_channel(void)
{
	ioctl_set_msg(file_desc, NULL, STOP_MONITORING);
}

int pause_thread_until_good_batch(void)
{
	ioctl_set_msg(file_desc, NULL, WAIT);
}

int pthread_join_hijack(pthread_t trd)
{
    static int num_joins = 0;
    num_joins += 1;
	ioctl_set_msg(file_desc, NULL, JOIN);
    if (num_joins == THREAD_NUM) {
        pthread_cancel(trd);
    }
}

// load_and_initialize_enclave():
//		To load and initialize the enclave
sgx_status_t load_and_initialize_enclave(sgx_enclave_id_t *eid, struct sealed_buf_t *sealed_buf)
{
    sgx_status_t ret = SGX_SUCCESS;
    int retval = 0;
    int updated = 0;

    ret = sgx_create_enclave(ENCLAVE_NAME, SGX_DEBUG_FLAG, &token, &updated, eid, NULL);
    if(ret != SGX_SUCCESS)
        return ret;

    ret = initialize_enclave(*eid, &retval, sealed_buf);
    if(ret == SGX_SUCCESS && retval != 0)
    {
        ret = SGX_ERROR_UNEXPECTED;
        sgx_destroy_enclave(*eid);
    }

    return ret;
}

bool increase_and_seal_data_in_enclave(unsigned int tidx)
{
    // size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
    sgx_status_t ret = SGX_SUCCESS;
    int retval = 0;
    sgx_enclave_id_t current_eid = 0;

    // Enter the enclave to increase and seal the secret data for 100 times.
    current_eid = global_eid[tidx];
    ret = increase_and_seal_data(current_eid, &retval, tidx,
            sealed_buf, tidx, &ds);

    if(ret != SGX_SUCCESS)
    {
        ret_error_support(ret);
        return false;
    }
    else if(retval != 0)
    {
        return false;
    }

    return true;
}


void handler(int signo, siginfo_t *info, void *extra)
{
    cout << pthread_self() << " caught signal " << signo << endl;
    sleep(5);
}


void set_sig_handler(void)
{
    struct sigaction action;
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = handler;
    if(sigaction(2, &action, NULL) == -1)
    {
        cout << "sigusr:sigaction" << endl;
        _exit(0);
    }
}


void *thread_func(void* i)
{
    int idx = *((int *)i);

    if(idx == 0)
    {
        set_sig_handler();

        // syscall(SYS_tgkill, getppid(), pthread_self(), 1);
        pthread_kill(pthread_self(), 2);
        cout << "syscalled " << (int)getppid() << ":" << pthread_self() << endl;
    }

    // Riccardo
    // pause_thread_until_good_batch();

    printf("Thread woken up\n");
    if(increase_and_seal_data_in_enclave(idx) != true)
    {
        abort();
    }
}

bool set_global_data()
{
    // Initialize the read/write lock.
    init_rwlock(&lock_eid);

    // Get the saved launch token.
    // If error occures, zero the token.
    ifstream ifs(TOKEN_NAME, std::ios::binary | std::ios::in);
    if(!ifs.good())
    {
        memset(token, 0, sizeof(sgx_launch_token_t));
    }
    else
    {
        ifs.read(reinterpret_cast<char *>(&token), sizeof(sgx_launch_token_t));
        if(ifs.fail())
        {
            memset(&token, 0, sizeof(sgx_launch_token_t));
        }
    }

    // Allocate memory to save the sealed data.
    uint32_t sealed_len = sizeof(sgx_sealed_data_t) + sizeof(uint32_t);
    for(int i = 0; i < BUF_NUM; i++)
    {
        sealed_buf->sealed_buf_ptr[i] = (uint8_t *)malloc(sealed_len);
        if(sealed_buf->sealed_buf_ptr[i] == NULL)
        {
            cout << "Out of memory" << endl;
            return false;
        }
        memset(sealed_buf->sealed_buf_ptr[i], 0, sealed_len);
    }
    sealed_buf->index = 0; // index indicates which buffer contains current sealed data and which contains the backup sealed data

    return true;
}

void release_source()
{
    for(int i = 0; i < BUF_NUM; i++)
    {
        if(sealed_buf->sealed_buf_ptr[i] != NULL)
        {
            free(sealed_buf->sealed_buf_ptr[i]);
            sealed_buf->sealed_buf_ptr[i] = NULL;
        }
    }
    fini_rwlock(&lock_eid);
    return;
}

int main(int argc, char* argv[])
{
    (void)argc, (void)argv;

    posix_memalign((void **)&sealed_buf, 4096, sizeof(*sealed_buf));

    cifar::read_array_dataset(&ds, 1500, 1);

    // check the length of the image (3*32*32 pixels)
    assert(ds.image_len == 4096);

    unsigned int target_count = 0;

    // Riccardo
    setup_kernel_channel();

    uint64_t *nuke;
    posix_memalign((void **)&nuke, 4096, 512 * sizeof(uint64_t));
    nuke[0] = 102;
    int tempor = 0;;
    for(unsigned int i=0; i<ds.len; i++)
    {
        // cout << "L" << ds.labels[i] << ":" << (int)ds.images[i*4096] << "@" <<
        //     (void*)&ds.images[i*4096] << endl;
        if(ds.labels[i] == 0)
        {
            // Riccardo
            send_image_address((void*)&ds.images[i * 4096]);
            target_count++;
            tempor = i * 4096;
        }
    }

    // Riccardo
    send_image_address((void *)&(nuke[0]));

    std::cout << std::endl << target_count << " target label images" <<
        std::endl << std::endl;

    // Initialize the global data
    if(!set_global_data())
    {
        release_source();
        cout << "Enter a character before exit ..." << endl;
        getchar();
        return -1;
    }

    // Load and initialize the signed enclave
    // sealed_buf == NULL indicates it is the first time to initialize the enclave.
    for(int i=0; i<THREAD_NUM; i++)
    {
        sgx_status_t ret = load_and_initialize_enclave(&global_eid[i], NULL);
        if(ret != SGX_SUCCESS)
        {
            ret_error_support(ret);
            release_source();
            cout << "Enter a character before exit ..." << endl;
            getchar();
            return -1;
        }
    }

    // Riccardo
    send_model_address(sealed_buf);

    // Riccardo
    start_controlled_side_channel();

    // The address passed does not work
    // printf("%d\n", ds.images[tempor]);   // not detected :(
    // printf("%d\n", sealed_buf[0]);  // detected :)
    //printf("%lu\n", nuke[0]);     // detected :)
    // exit(0);

    // Create multiple threads to calculate the sum
    pthread_t trd[THREAD_NUM];
    // thread trd[THREAD_NUM];
    for (int i = 0; i< THREAD_NUM; i++)
    {
        int *arg = (int*) malloc(sizeof(*arg));
            if( arg == NULL ) {
                cout << "couldn't allocate memory" << endl;
                return -1;
            }
        *arg = i;

        cout << "Starting pthread" << endl;
        int ret = pthread_create(&trd[i], NULL, thread_func, arg);
        // trd[i] = thread(thread_func, i);
    }

    cout << "Started all pthreads" << endl;

    // kill logic
    // for (int i = 0; i < THREAD_NUM; i++)
    // {
    //     for(unsigned int j=0; j<200; j++) {
    //         cout << "waiting" << endl;
    //     }
    //     cout << "killing " << i << endl;
    //     pthread_cancel(trd[i]);
    //     // pthread_join(trd[i], NULL);
    //     // trd[i].join();
    // }

    for (int i = 0; i < THREAD_NUM; i++)
    {
        // Riccardo
        pthread_join_hijack(i);

       // cout << "Hijacked thread " << i << endl;
       // trd[i].join();
        pthread_join(trd[i], NULL);
       // cout << "Joined thread " << i << endl;
    }

    cout << "Finished!" << endl;
    //thread_func(1);

    // Release resources
    release_source();

    // Destroy the enclave
    for(int i=0; i<THREAD_NUM; i++)
    {
        sgx_destroy_enclave(global_eid[i]);
    }

    // Riccardo
    stop_controlled_side_channel();

    return 0;
}

