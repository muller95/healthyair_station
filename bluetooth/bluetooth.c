#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#include "bluetooth.h"

#define TIMEOUT_CYCLES 	100
#define MAX_BUF_SIZE 	4096
#define DELAY_TIME		100

enum BL_ERRNO bl_errno = BL_ESUCCESS;

int
bl_init(char *serial) 
{
	int fd;

	if (wiringPiSetup() < 0) {
		bl_errno = BL_EWPINIT;
		return -1;
	}

	if ((fd = serialOpen(serial, 9600)) < 0) {
		bl_errno = BL_ESEROPEN;
		return -1;
	}

	return fd;
}

int
bl_ok(int fd)
{
	char cmd[] = "AT", response[MAX_BUF_SIZE];
	
	for (int i = 0; i < strlen(cmd); i++)
		serialPutchar(fd, cmd[i]);


	bzero(response, MAX_BUF_SIZE);
	for (int i = 0, cycles = 0; cycles < TIMEOUT_CYCLES; cycles++) {
		delay(DELAY_TIME);
		while (serialDataAvail(fd) > 0)
			response[i++] = serialGetchar(fd);
	

		if (strcmp(response, "OK") == 0)
			return 0;
	}

	if (strlen(response) > 0)
		bl_errno = BL_EAT;
	else
		bl_errno = BL_ETIMEOUT;

	return -1;
}

int
bl_name(int fd, char *name)
{
	char cmd[MAX_BUF_SIZE], response[MAX_BUF_SIZE];

	if (strlen(name) > 20) {
		bl_errno = BL_EINVAL;
		return -1;
	}
	
	
	bzero(cmd, MAX_BUF_SIZE);
	sprintf(cmd, "AT+NAME%s", name);
	for (int i = 0; i < strlen(cmd); i++)
		serialPutchar(fd, cmd[i]);

	bzero(response, MAX_BUF_SIZE);
	for (int i = 0, cycles = 0; cycles < TIMEOUT_CYCLES; cycles++) {
		delay(DELAY_TIME);
		while (serialDataAvail(fd) > 0)
			response[i++] = serialGetchar(fd);
	

		if (strcmp(response, "OKsetname") == 0)
			return 0;
		printf("%s\n", response);
	}
	
	if (strlen(response) > 0)
		bl_errno = BL_EAT;
	else
		bl_errno = BL_ETIMEOUT;

	return -1;
}
