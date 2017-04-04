#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include <wiringPi.h>

#include "sht_handler.h"


const uint8_t T_CMD   		= 0x03;   // 000  0001   1
const uint8_t RH_CMD		= 0x05;   // 000  0010   1
const uint8_t STAT_REG_W  	= 0x06;   // 000  0011   0
const uint8_t STAT_REG_R  	= 0x07;   // 000  0011   1
const uint8_t SOFT_RESET  	= 0x1e;   // 000  1111   0

// Status register writable bits
const uint8_t SR_MASK     = 0x07;

// getByte flags
const bool noACK  = false;
const bool ACK    = true;

// Temperature & humidity equation constants
const double D1  = -40.1;          // for deg C @ 5V
const double D2h =   0.01;         // for deg C, 14-bit precision
const double D2l =   0.04;         // for deg C, 12-bit precision

//  const double C1  = -4.0000;        // for V3 sensors
//  const double C2h =  0.0405;        // for V3 sensors, 12-bit precision
//  const double C3h = -2.8000E-6;     // for V3 sensors, 12-bit precision
//  const double C2l =  0.6480;        // for V3 sensors, 8-bit precision
//  const double C3l = -7.2000E-4;     // for V3 sensors, 8-bit precision
const double C1  = -2.0468;        // for V4 sensors
const double C2h =  0.0367;        // for V4 sensors, 12-bit precision
const double C3h = -1.5955E-6;     // for V4 sensors, 12-bit precision
const double C2l =  0.5872;        // for V4 sensors, 8-bit precision
const double C3l = -4.0845E-4;     // for V4 sensors, 8-bit precision

const double T1  =  0.01;          // for V3 and V4 sensors
const double T2h =  0.00008;       // for V3 and V4 sensors, 12-bit precision
const double T2l =  0.00128;       // for V3 and V4 sensors, 8-bit precision

sht_handler::sht_handler(uint8_t data_pin, uint8_t clock_pin) {
  this->data_pin = data_pin;
  this->clock_pin = clock_pin;
  this->sr = 0x00;                 
  reset();
}

uint8_t sht_handler::measure(double *t, double *rh) {
  uint16_t raw_data;
  uint8_t error;
  pinMode(data_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  if ((error = request_raw(T, &raw_data)))
    return error;
  *t = calc_t(raw_data);
  if ((error = request_raw(RH, &raw_data)))
    return error;
  *rh = calc_rh(raw_data, *t);
  return 0 ;
}

uint8_t sht_handler::request_raw(uint8_t cmd, uint16_t *result) {
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
      return S_Err_TO;              
    delay(3);
  }
  error = get_result(result);
  return error;
}

uint8_t sht_handler::get_result(uint16_t *result) {
  uint8_t val;
  val = get_byte(ACK);
  calc_crc(val, &crc);
  *result = val;
  val = get_byte(ACK);
  calc_crc(val, &crc);
  *result = (*result << 8) | val;
  val = get_byte(noACK);
  val = bitrev(val);
  if (val != crc) {
    *result = 0xFFFF;
    return S_Err_CRC;
  }
  return 0;
}

uint8_t sht_handler::write_sr(uint8_t value) {
  uint8_t error;
  value &= SR_MASK;                 // Mask off unwritable bits
  sr = value;                // Save local copy
  transmission_start();
  if ((error = put_byte(STAT_REG_W)))
    return error;
  return put_byte(value);
}

uint8_t sht_handler::read_sr(uint8_t *result) {
  uint8_t val;
  uint8_t error = 0;
  crc = bitrev(sr & SR_MASK);  // Initialize CRC calculation
  transmission_start();
  if ((error = put_byte(STAT_REG_R))) {
    *result = 0xFF;
    return error;
  }
  calc_crc(STAT_REG_R, &crc);       // Include command byte in CRC calculation
  *result = get_byte(ACK);
  calc_crc(*result, &crc);
  val = get_byte(noACK);
  val = bitrev(val);
  if (val != crc) {
    *result = 0xFF;
    error = S_Err_CRC;
  }
  return error;
}

uint8_t sht_handler::reset(void) {
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
  uint8_t mask, i;
  uint8_t error = 0;
  pinMode(data_pin, OUTPUT);        // Set data line to output mode
  mask = 0x80;                      // Bit mask to transmit MSB first
  for (i = 8; i > 0; i--) {
    digitalWrite(data_pin, value & mask);
    PULSE_SHORT;
    digitalWrite(clock_pin, HIGH);  // Generate clock pulse
    PULSE_LONG;
    digitalWrite(clock_pin, LOW);
    PULSE_SHORT;
    mask >>= 1;                     // Shift mask for next data bit
  }
  pinMode(data_pin, INPUT);         // Return data line to input mode
#ifdef DATA_PU
  digitalWrite(data_pin, DATA_PU);  // Restore internal pullup state
#endif
  digitalWrite(clock_pin, HIGH);    // Clock #9 for ACK
  PULSE_LONG;
  if (digitalRead(data_pin))        // Verify ACK ('0') received from sensor
    error = S_Err_NoACK;
  PULSE_SHORT;
  digitalWrite(clock_pin, LOW);     // Finish with clock in low state
  return error;
}

// Read byte from sensor and send acknowledge if "ack" is true
uint8_t sht_handler::get_byte(bool ack) {
  uint8_t i;
  uint8_t result = 0;
  for (i = 8; i > 0; i--) {
    result <<= 1;                   // Shift received bits towards MSB
    digitalWrite(clock_pin, HIGH);  // Generate clock pulse
    PULSE_SHORT;
    result |= digitalRead(data_pin);  // Merge next bit into LSB position
    digitalWrite(clock_pin, LOW);
    PULSE_SHORT;
  }
  pinMode(data_pin, OUTPUT);
  digitalWrite(data_pin, !ack);     // Assert ACK ('0') if ack == 1
  PULSE_SHORT;
  digitalWrite(clock_pin, HIGH);    // Clock #9 for ACK / noACK
  PULSE_LONG;
  digitalWrite(clock_pin, LOW);     // Finish with clock in low state
  PULSE_SHORT;
  pinMode(data_pin, INPUT);         // Return data line to input mode
#ifdef DATA_PU
  digitalWrite(data_pin, DATA_PU);  // Restore internal pullup state
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

// Calculates relative humidity from raw sensor data
//   (with temperature compensation)
double sht_handler::calc_rh(uint16_t raw_data, double temp) {
  double humi;
  if (sr & LOW_RES) {
    humi = C1 + C2l * raw_data + C3l * raw_data * raw_data;
    humi = (temp - 25.0) * (T1 + T2l * raw_data) + humi;
  } else {
    humi = C1 + C2h * raw_data + C3h * raw_data * raw_data;
    humi = (temp - 25.0) * (T1 + T2h * raw_data) + humi;
  }
  if (humi > 100.0) humi = 100.0;
  if (humi < 0.1) humi = 0.1;
  return humi;
}

// Calculate CRC for a single byte
void sht_handler::calc_crc(uint8_t value, uint8_t *crc) {
  const uint8_t POLY = 0x31;   // Polynomial: x**8 + x**5 + x**4 + 1
  uint8_t i;
  *crc ^= value;
  for (i = 8; i > 0; i--) {
    if (*crc & 0x80)
      *crc = (*crc << 1) ^ POLY;
    else
      *crc = (*crc << 1);
  }
}

// Bit-reverse a byte (for CRC calculations)
uint8_t sht_handler::bitrev(uint8_t value) {
  uint8_t i;
  uint8_t result = 0;
  for (i = 8; i > 0; i--) {
    result = (result << 1) | (value & 0x01);
    value >>= 1;
  }
  return result;
}
