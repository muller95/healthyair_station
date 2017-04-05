#include <iostream>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#include "sht_handler.h"

using namespace std;

int 
main()
{
	const uint8_t data_pin  =  5, clock_pin =  4;
	double t, rh;
	
	sht_handler sht = sht_handler(data_pin, clock_pin);
	if (wiringPiSetup() != 0) {
		cout << "ERR" << endl << "Error setuping wiring Pi" << endl;
		return 1;
	}
	
	
	sht.measure(&t, &rh);

	if (sht.get_last_error() == "")
		cout << "OK" << endl << t << " " << rh << endl;
	else 
		cout << "ERR" << endl << sht.get_last_error() << endl;

	return 0;
}
