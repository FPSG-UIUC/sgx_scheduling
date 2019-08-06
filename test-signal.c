#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
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

// gcc -m64 -O3 test-signal.c -o test-signal -lpthread

#define MAJOR_NUM 1315

#define IOCTL_APPEND_ADDR _IOR(MAJOR_NUM, 0, char *)

#define IOCTL_PASS_SPECIAL_ADDR _IOR(MAJOR_NUM, 1, char *)

#define IOCTL_START_MONITORING _IOR(MAJOR_NUM, 2, char *)

#define IOCTL_STOP_MONITORING _IOR(MAJOR_NUM, 3, char *)

#define IOCTL_SIGNAL _IOR(MAJOR_NUM, 4, char *)

#define IOCTL_JOIN _IOR(MAJOR_NUM, 5, char *)

#define DEVICE_FILE_NAME "nuke_channel"
#define DEVICE_FILE_NAME_PATH "/home/riccardo/nukemod/nuke_channel"
#define THREAD_NUM 3

enum call_type { APPEND_ADDR,
				 PASS_SPECIAL_ADDR,
				 START_MONITORING,
				 STOP_MONITORING,
				 SIGNAL,
				 JOIN };

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
	case SIGNAL:
		ret_val = ioctl(file_desc, IOCTL_SIGNAL, message);
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

void handler(int signo, siginfo_t *info, void *extra)
{
	// Print TID to see which thread serviced the signal
    pid_t tid;
    tid = syscall(SYS_gettid);
    printf("caught signal for thread id %d \n", tid);
    //tid = syscall(SYS_tgkill, getpid(), tid);
	exit(0);
}


void set_sig_handler(void)
{
    struct sigaction action;
    action.sa_flags = SIGKILL;
    action.sa_sigaction = handler;
    if(sigaction(1, &action, NULL) == -1)
    {
        printf("some error happened\n");
        _exit(0);
    }
}

int alive = 0;
void *thread_func(void* i)
{
	// Call wait immediately after call
	set_sig_handler();
	ioctl_set_msg(file_desc, NULL, SIGNAL);
    while (alive == 0) {;}
}

int
main(int argc, char *argv[])
{
	setup_kernel_channel();

	pthread_t trd[THREAD_NUM];
    for (int i = 0; i< THREAD_NUM; i++) {
        pthread_create(&trd[i], NULL, thread_func, NULL);
    }

    printf("Started all threads\n");
	sleep(2);

	printf("Sending signal\n");
	ioctl_set_msg(file_desc, NULL, SIGNAL);

    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_join(trd[i], NULL);
    }

	printf("Joined all threads\n");
}