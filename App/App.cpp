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
#include <thread>
#include <iostream>

#include "Enclave_u.h"
#include "sgx_urts.h"
#include "sgx_tseal.h"

#include "rwlock.h"
#include "ErrorSupport.h"

#define ENCLAVE_NAME "libenclave.signed.so"
#define TOKEN_NAME "Enclave.token"

#define THREAD_NUM 3

// Global data
sgx_enclave_id_t global_eid = 0;
sgx_launch_token_t token = {0};
rwlock_t lock_eid;
struct sealed_buf_t sealed_buf;

using namespace std;

// Ocall function
void print(const char *str)
{
    cout<<str;
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

bool increase_and_seal_data_in_enclave()
{
    size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
    sgx_status_t ret = SGX_SUCCESS;
    int retval = 0;
    sgx_enclave_id_t current_eid = 0;

    // Enter the enclave to increase and seal the secret data for 100 times.
    for(unsigned int i = 0; i<5; i++)
    {
        current_eid = global_eid;
        ret = increase_and_seal_data(current_eid, &retval, thread_id,
                &sealed_buf);

        if(ret != SGX_SUCCESS)
        {
            ret_error_support(ret);
            return false;
        }
        else if(retval != 0)
        {
            return false;
        }
    }
    return true;
}


void thread_func(int idx)
{
    if(increase_and_seal_data_in_enclave() != true)
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
        sealed_buf.sealed_buf_ptr[i] = (uint8_t *)malloc(sealed_len);
        if(sealed_buf.sealed_buf_ptr[i] == NULL)
        {
            cout << "Out of memory" << endl;
            return false;
        }
        memset(sealed_buf.sealed_buf_ptr[i], 0, sealed_len);
    }
    sealed_buf.index = 0; // index indicates which buffer contains current sealed data and which contains the backup sealed data

    return true;
}

void release_source()
{
    for(int i = 0; i < BUF_NUM; i++)
    {
        if(sealed_buf.sealed_buf_ptr[i] != NULL)
        {
            free(sealed_buf.sealed_buf_ptr[i]);
            sealed_buf.sealed_buf_ptr[i] = NULL;
        }
    }
    fini_rwlock(&lock_eid);
    return;
}

int main(int argc, char* argv[])
{
    (void)argc, (void)argv;


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
    sgx_status_t ret = load_and_initialize_enclave(&global_eid , NULL);
    if(ret != SGX_SUCCESS)
    {
        ret_error_support(ret);
        release_source();
        cout << "Enter a character before exit ..." << endl;
        getchar();
        return -1;
    }

    // Create multiple threads to calculate the sum
    thread trd[THREAD_NUM];
    for (int i = 0; i< THREAD_NUM; i++)
    {
        trd[i] = thread(thread_func, i+1);
    }
    for (int i = 0; i < THREAD_NUM; i++)
    {
        trd[i].join();
    }

    // Release resources
    release_source();

    // Destroy the enclave
    sgx_destroy_enclave(global_eid);

    cout << "Enter a character before exit ..." << endl;
    getchar();
    return 0;
}

