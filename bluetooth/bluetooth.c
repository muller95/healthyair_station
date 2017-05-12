#include <wiringPi.h>
#include <wiringSerial.h>

#include "bluetooth.h"

//enum BL_ERRNO bl_errno = BL_ESUCCESS;


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
