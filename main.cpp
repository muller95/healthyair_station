#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>


#define MAXTIMINGS 85
#define DHT_PIN 7

int fd;
static int dht22_dat[5] = {0,0,0,0,0};

static uint8_t sizecvt(const int read)
{
    /* digitalRead() and friends from wiringpi are defined as returning a value
       < 256. However, they are returned as int() types. This is a safety function */

    if (read > 255 || read < 0)
    {
        printf("Invalid data from wiringPi library\n");
        exit(EXIT_FAILURE);
    }
    return (uint8_t)read;
}

static int read_dht22_dat(int iPin, int* piHumidity, int* piTemp)
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

    // pull pin down for 18 milliseconds
    pinMode(iPin, OUTPUT);
    digitalWrite(iPin, LOW);
    delay(18);

    // then pull it up for 40 microseconds
    digitalWrite(iPin, HIGH);
    delayMicroseconds(40);

	// prepare to read the pin
    pinMode(iPin, INPUT);

    // detect change and read data
    for ( i=0; i< MAXTIMINGS; i++)
	{
        counter = 0;
        while (sizecvt(digitalRead(iPin)) == laststate)
		{
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
			{
                break;
            }
        }
        laststate = sizecvt(digitalRead(iPin));

        if (counter == 255) break;

        // ignore first 3 transitions
        if ((i >= 4) && (i%2 == 0))
		{
            // shove each bit into the storage bytes
            dht22_dat[j/8] <<= 1;
            if (counter > 16)
                dht22_dat[j/8] |= 1;
            j++;
        }
    }

    // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
    // print it out if data is good
    if ((j >= 40) && (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) )
	{
		*piHumidity = dht22_dat[0] * 256 + dht22_dat[1];
		*piTemp = (dht22_dat[2] & 0x7F)* 256 + dht22_dat[3];
        if ((dht22_dat[2] & 0x80) != 0)
			*piTemp *= -1;

		return 1;
    }
    else
    {
        return 0;
    }
}

void loop()
{
	int i, avail, result, iHumidity, iTemp;
	int32_t ppm;	
	int resp[7];
	char request[] = {0xFE, 0x4, 0x0, 0x3, 0x0, 0x1, 0xD5, 0xC5};


	

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
	
	while ((result = read_dht22_dat(DHT_PIN, &iHumidity, &iTemp)) != 1);

	printf("%.2f %.2f %d\n", (float)iTemp / 10.0f, (float)iHumidity / 10.0f, ppm);
	fflush(stdout);
	delay(5000);
}

int main()
{	
	if (wiringPiSetup() != 0) {
		fprintf(stderr, "Setup err: %s\n", strerror(errno));
	}
	
	if ((fd = serialOpen("/dev/ttyS1", 9600)) < 0) {
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1;
	}

	delay(5000);

	while (1) 
		loop();
	return 0;
}
