#ifndef CO2_HANDLER_H
#define CO2_HANDLER_H

#include <iostream>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringSerial.h>

using namespace std;

class co2_handler {
public:
	int32_t query(const char *serial_path);
	string get_last_error();

private:
	string error_msg;
};

#endif
