#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#include "sht_handler.h"


int 
main()
{
	const uint8_t data_pin  =  5, clock_pin =  4;

	float temperature;
	float humidity;
	
	sht_handler sht = sht_handler(data_pin, clock_pin);
	if (wiringPiSetup() != 0)
		return 1;
	
	
	while (1) {
		sht.measure(&temperature, &humidity);
		printf("%f %f\n", temperature, humidity);
		fflush(stdout);
		delay(1000);
	}

	return 0;
}
