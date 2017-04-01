#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#include "Sensirion.h"

const uint8_t dataPin  =  5;
const uint8_t clockPin =  4;

float temperature;
float humidity;
float dewpoint;


Sensirion tempSensor = Sensirion(dataPin, clockPin);

int32_t
k30_query()
{
	int i;
	int fd, avail;
	int resp[7];
	char request[] = {0xFE, 0x4, 0x0, 0x3, 0x0, 0x1, 0xD5, 0xC5};
	int32_t ppm = 0;	
	if ((fd = serialOpen("/dev/ttyS1", 9600)) < 0) {
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1;
	}

	for (i = 0; i < 7; i++)
		resp[i] = 0;

	while ((avail = serialDataAvail(fd)) < 7) {
		for (i = 0; i < 8; i++)
			serialPutchar(fd, request[i]);
		delay(100);
	}
	

	if (avail < 0)
		fprintf (stderr, "%s\n", strerror (errno)) ;
	
	for (i = 0; i < 7; i++) 
		resp[i] = serialGetchar(fd);
	ppm = (resp[3] << 8) | resp[4];


	serialClose(fd);	

	return ppm;
}

int 
main()
{
	if (wiringPiSetup() != 0)
		return 1;
	
//	pinMode(dataPin, OUTPUT);
//	pinMode(clockPin, OUTPUT);
	
	
	while (1) {
		int32_t ppm = k30_query();
		tempSensor.measure(&temperature, &humidity, &dewpoint);
		printf("%f %f %d\n", temperature, humidity, ppm);
		fflush(stdout);
		delay(1000);
	}

	return 0;
}
