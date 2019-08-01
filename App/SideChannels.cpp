/*
 * ioctl_set_msg is the interface to the kernel module
 * @file_desc is the file descriptor for the char device
 * @message is the buffer holding the message to transfer
 * @type is the type of ioctl
 */

#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include "SideChannels.h"

static void ioctl_set_msg(int file_desc, char *message, enum call_type type)
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

	// Send nuke_addr to ioctl device in the kernel
	ioctl_set_msg(file_desc, msg, IOCTL_APPEND_ADDR);
}

int send_model_address(void *addr)
{
	char buffer[80], *msg = NULL;

	// Write the start of nuke_addr into the buffer
	sprintf(buffer, "%p", addr);
	msg = buffer;

	// Send nuke_addr to ioctl device in the kernel
	ioctl_set_msg(file_desc, msg, IOCTL_PASS_SPECIAL_ADDR);
}

int start_controlled_side_channel(void)
{

	// Send nuke_addr to ioctl device in the kernel
	ioctl_set_msg(file_desc, NULL, IOCTL_START_MONITORING);
}

int stop_controlled_side_channel(void)
{

	// Send nuke_addr to ioctl device in the kernel
	ioctl_set_msg(file_desc, NULL, IOCTL_STOP_MONITORING);
}

int pause_thread_until_good_batch(void)
{

	// Send nuke_addr to ioctl device in the kernel
	ioctl_set_msg(file_desc, NULL, IOCTL_WAIT);
}