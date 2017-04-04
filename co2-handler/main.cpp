#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>

#include "co2_handler.h"

int 
main()
{
	co2_handler co2;
	if (wiringPiSetup() != 0)
		return 1;
		
	int32_t ppm = co2.query("/dev/ttyS1");
	if (co2.get_last_error() == "")		
		cout << "OK" << endl << ppm << endl;
	else
		cout << "ERR" << endl << co2.get_last_error() << endl;

	return 0;
}
