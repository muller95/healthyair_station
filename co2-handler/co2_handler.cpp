#include <errno.h>
#include <string.h>

#include "co2_handler.h"

uint8_t
co2_handler::calc_check(uint8_t *data)
{
	uint8_t check = 0;
	for (int i = 1; i < 8; i++)
		check += data[i];

	return ~check + 1;
}

int32_t 
co2_handler::query(const char *serial_path)
{
	int i;
	int fd, avail;
	uint8_t response[9];
	uint8_t request[] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int32_t ppm = 0;	

	error_msg = "";
	if ((fd = serialOpen(serial_path, 9600)) < 0) {
		error_msg = string("Unable to open serial device: ");
		error_msg += string(strerror(errno));
		return -1;
	}

	request[8] = this->calc_check(request);
	for (i = 0; i < 9; i++)
		response[i] = 0;

	for (i = 0; i < 9; i++)
		serialPutchar(fd, request[i]);

	while ((avail = serialDataAvail(fd)) < 9) {
		delay(100);
	}
	

	if (avail < 0) {
		error_msg = string("Unable to read serial device: ");
		error_msg += string(strerror(errno));
	}
	
	for (i = 0; i < 9; i++) 
		response[i] = serialGetchar(fd);
	ppm = (response[2] << 8) | response[3];

	if (response[8] != this->calc_check(response)) {
		error_msg = string("Check value error");
		return -1;
	}


	serialClose(fd);	

	return ppm;
}

string
co2_handler::get_last_error()
{
	return error_msg;
}
