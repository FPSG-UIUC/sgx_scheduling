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
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

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
#define SIG_RICCARDO 44

enum call_type { APPEND_ADDR,
				 PASS_SPECIAL_ADDR,
				 START_MONITORING,
				 STOP_MONITORING,
				 SIGNAL,
				 JOIN };

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

//void handler(int signo, siginfo_t *info, void *extra)
void handler(int signo, siginfo_t *info, void *unused)
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
	struct sigaction act;
 
	memset (&act, '\0', sizeof(act));
 
	/* Use the sa_sigaction field because the handles has two additional parameters */
	act.sa_sigaction = handler;
 
	/* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
	act.sa_flags = SA_SIGINFO;
 
	if (sigaction(SIG_RICCARDO, &act, NULL) < 0) {
		perror ("sigaction");
		exit(0);
	}

    printf("registered handler\n");
}

int alive = 0;
void *thread_func(void* i)
{
	// Call wait immediately after call
	ioctl(file_desc, IOCTL_SIGNAL, NULL);
    while (alive == 0) {;}
}

int
main(int argc, char *argv[])
{
	setup_kernel_channel();
    set_sig_handler();

	pthread_t trd[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&trd[i], NULL, thread_func, NULL);
    }

    printf("Started all threads\n");
	sleep(2);

	printf("Sending signal\n");
	ioctl(file_desc, IOCTL_SIGNAL, NULL);
    printf("Sent signal\n");

    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_join(trd[i], NULL);
    }

	printf("Joined all threads\n");
}
