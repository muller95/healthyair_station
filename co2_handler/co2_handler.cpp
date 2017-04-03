#include <errno.h>
#include <string.h>

#include "co2_handler.h"

int32_t 
co2_handler::query(const char *serial_path)
{
	int i;
	int fd, avail;
	int resp[7];
	char request[] = {0xFE, 0x4, 0x0, 0x3, 0x0, 0x1, 0xD5, 0xC5};
	int32_t ppm = 0;	

	error_msg = "";
	if ((fd = serialOpen(serial_path, 9600)) < 0) {
		error_msg = string("Unable to open serial device: ");
		error_msg += string(strerror(errno));
		return 1;
	}

	for (i = 0; i < 7; i++)
		resp[i] = 0;

/*	while ((avail = serialDataAvail(fd)) < 7) {
		for (i = 0; i < 8; i++)
			serialPutchar(fd, request[i]);
		delay(100);
	}*/
		
	for (i = 0; i < 8; i++)
		serialPutchar(fd, request[i]);
	while ((avail = serialDataAvail(fd)) < 7) {
		delay(100);
	}
	

	if (avail < 0) {
		error_msg = string("Unable to read serial device: ");
		error_msg += string(strerror(errno));
	}
	
	for (i = 0; i < 7; i++) 
		resp[i] = serialGetchar(fd);
	ppm = (resp[3] << 8) | resp[4];


	serialClose(fd);	

	return ppm;
}

string
co2_handler::get_last_error()
{
	return error_msg;
}
