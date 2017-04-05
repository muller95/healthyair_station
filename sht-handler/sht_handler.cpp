#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include <wiringPi.h>

#include "sht_handler.h"


const uint8_t T_CMD			= 0x03;		
const uint8_t RH_CMD		= 0x05;	
const uint8_t STAT_REG_W	= 0x06;
const uint8_t STAT_REG_R	= 0x07;
const uint8_t SOFT_RESET	= 0x1e;

const uint8_t SR_MASK			= 0x07;

const bool NO_ACK	= false;
const bool ACK		= true;

const double D1  = -40.1;			
const double D2h =	 0.01;		
const double D2l =	 0.04;	

const double C1  = -2.0468;
const double C2h =	0.0367;
const double C3h = -1.5955E-6;
const double C2l =	0.5872;		
const double C3l = -4.0845E-4;	

const double T1  =	0.01;	
const double T2h =	0.00008;
const double T2l =	0.00128;			 

const uint8_t EOK	= 0;
const uint8_t EACK	= 1;
const uint8_t ECRC	= 2; 
const uint8_t ETO	= 3;	

sht_handler::sht_handler(uint8_t data_pin, uint8_t clock_pin) {
	this->data_pin = data_pin;
	this->clock_pin = clock_pin;
	this->sr = 0x00;								 
	reset();
}

void
sht_handler::set_error_msg(uint8_t error)
{
	switch (error) {
		case EOK: {
			error_msg = "OK";
			break;
		}

		case EACK: {
			error_msg = "Fail to read ACK";
			break;
		}

		case ECRC: {
			error_msg = "Incorrect CRC";
			break;
		}

		case ETO: {
			error_msg = "SHT timeout";
			break;
		}
	}
}

string
sht_handler::get_last_error(void)
{
	return error_msg;
}

void
sht_handler::measure(double *t, double *rh) 
{
	uint16_t raw_data;
	uint8_t error;
	
	pinMode(data_pin, OUTPUT);
	pinMode(clock_pin, OUTPUT);
	
	if ((error = request_raw(T, &raw_data)))
		goto set_error;
	*t = calc_t(raw_data);

	if ((error = request_raw(RH, &raw_data)))
		goto set_error;
	*rh = calc_rh(raw_data, *t);

set_error:
	set_error_msg(error);
}

uint8_t 
sht_handler::request_raw(uint8_t cmd, uint16_t *result) 
{
	uint8_t error, i;
	
	crc = bitrev(sr & SR_MASK); 
	transmission_start();
	
	if (cmd == T)
		cmd = T_CMD;
	else
		cmd = RH_CMD;
	if ((error = put_byte(cmd)))
		return error;
	
	calc_crc(cmd, &crc);						 
	
	i = 240;
	while (digitalRead(data_pin)) {
		i--;
		if (i == 0)
			return ETO;							 
		delay(3);
	}

	error = get_result(result);
	return error;
}

uint8_t 
sht_handler::get_result(uint16_t *result) 
{
	uint8_t val;
	
	val = get_byte(ACK);
	calc_crc(val, &crc);
	*result = val;
	val = get_byte(ACK);
	calc_crc(val, &crc);
	*result = (*result << 8) | val;
	val = get_byte(NO_ACK);
	val = bitrev(val);
	
	if (val != crc) {
		*result = 0xFFFF;
		return ECRC;
	}
	
	return 0;
}

uint8_t 
sht_handler::write_sr(uint8_t value) 
{
	uint8_t error;
	
	value &= SR_MASK;		 
	sr = value;					
	
	transmission_start();
	if ((error = put_byte(STAT_REG_W)))
		return error;
	
	return put_byte(value);
}

uint8_t 
sht_handler::read_sr(uint8_t *result) {
	uint8_t val, error = 0;

	crc = bitrev(sr & SR_MASK);
	transmission_start();
	if ((error = put_byte(STAT_REG_R))) {
		*result = 0xFF;
		return error;
	}

	calc_crc(STAT_REG_R, &crc);
	*result = get_byte(ACK);
	calc_crc(*result, &crc);
	val = get_byte(NO_ACK);
	val = bitrev(val);
	
	if (val != crc) {
		*result = 0xFF;
		error = ECRC;
	}
	
	return error;
}

uint8_t 
sht_handler::reset(void) {
	uint8_t i;
	
	sr = 0x00;								 
	digitalWrite(data_pin, HIGH); 
	pinMode(data_pin, OUTPUT);
	PULSE_LONG;
	
	for (i = 0; i < 9; i++) {
		digitalWrite(clock_pin, HIGH);
		PULSE_LONG;
		digitalWrite(clock_pin, LOW);
		PULSE_LONG;
	}

	transmission_start();
	return put_byte(SOFT_RESET);
}

uint8_t sht_handler::put_byte(uint8_t value) {
	uint8_t mask, i, error = 0;

	pinMode(data_pin, OUTPUT);				
	mask = 0x80;										 

	for (i = 8; i > 0; i--) {
		digitalWrite(data_pin, value & mask);
		PULSE_SHORT;
		digitalWrite(clock_pin, HIGH);
		PULSE_LONG;
		digitalWrite(clock_pin, LOW);
		PULSE_SHORT;
		mask >>= 1;									 
	}

	pinMode(data_pin, INPUT);				
#ifdef DATA_PU
	digitalWrite(data_pin, DATA_PU);
#endif
	digitalWrite(clock_pin, HIGH); 
	PULSE_LONG;

	if (digitalRead(data_pin))		
		error = EACK;
	PULSE_SHORT;
	digitalWrite(clock_pin, LOW); 

	return error;
}

uint8_t sht_handler::get_byte(bool ack) {
	uint8_t i, result = 0;
	for (i = 8; i > 0; i--) {
		result <<= 1;									
		digitalWrite(clock_pin, HIGH);	
		PULSE_SHORT;
		result |= digitalRead(data_pin);
		digitalWrite(clock_pin, LOW);
		PULSE_SHORT;
	}
	pinMode(data_pin, OUTPUT);
	digitalWrite(data_pin, !ack);	
	PULSE_SHORT;
	digitalWrite(clock_pin, HIGH);
	PULSE_LONG;
	digitalWrite(clock_pin, LOW);
	PULSE_SHORT;
	pinMode(data_pin, INPUT);	
#ifdef DATA_PU
	digitalWrite(data_pin, DATA_PU);
#endif
	return result;
}

void sht_handler::transmission_start(void) {
	digitalWrite(data_pin, HIGH);
	pinMode(data_pin, OUTPUT);
	PULSE_SHORT;
	digitalWrite(clock_pin, HIGH);
	PULSE_SHORT;
	digitalWrite(data_pin, LOW);
	PULSE_SHORT;
	digitalWrite(clock_pin, LOW);
	PULSE_LONG;
	digitalWrite(clock_pin, HIGH);
	PULSE_SHORT;
	digitalWrite(data_pin, HIGH);
	PULSE_SHORT;
	digitalWrite(clock_pin, LOW);
	PULSE_SHORT;
}

double sht_handler::calc_t(uint16_t raw_data) {
	if (sr & LOW_RES)
		return D1 + D2l * (double) raw_data;
	else
		return D1 + D2h * (double) raw_data;
}

double sht_handler::calc_rh(uint16_t raw_data, double t) {
	double rh;

	if (sr & LOW_RES) {
		rh = C1 + C2l * raw_data + C3l * raw_data * raw_data;
		rh = (t - 25.0) * (T1 + T2l * raw_data) + rh;
	} else {
		rh = C1 + C2h * raw_data + C3h * raw_data * raw_data;
		rh = (t - 25.0) * (T1 + T2h * raw_data) + rh;
	}
	if (rh > 100.0) 
		rh = 100.0;
	if (rh < 0.1) 
		rh = 0.1;
	return rh;
}

void sht_handler::calc_crc(uint8_t value, uint8_t *crc) {
	const uint8_t POLY = 0x31;
	uint8_t i;
	
	*crc ^= value;
	for (i = 8; i > 0; i--) {
		if (*crc & 0x80)
			*crc = (*crc << 1) ^ POLY;
		else
			*crc = (*crc << 1);
	}
}

uint8_t sht_handler::bitrev(uint8_t value) {
	uint8_t i, result = 0;
	for (i = 8; i > 0; i--) {
		result = (result << 1) | (value & 0x01);
		value >>= 1;
	}
	return result;
}
