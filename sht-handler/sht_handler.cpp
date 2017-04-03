/* ========================================================================== */
/*  Sensirion.cpp - Library for Sensirion SHT1x & SHT7x family temperature    */
/*    and humidity sensors                                                    */
/*  Created by Markus Schatzl, November 28, 2008                              */
/*  Released into the public domain                                           */
/*                                                                            */
/*  Revised (v1.1) by Carl Jackson, August 4, 2010                            */
/*  Rewritten (v2.0) by Carl Jackson, December 10, 2010                       */
/*    See README.txt file for details                                         */
/* ========================================================================== */


/******************************************************************************
 * Includes
 ******************************************************************************/


  // AVR LibC Includes
  #include <stddef.h>
  #include <stdint.h>
  #include <math.h>

  // Wiring Core Includes
  #include <wiringPi.h>


#include "sht_handler.h"


/******************************************************************************
 * Definitions
 ******************************************************************************/

// Sensirion command definitions:      adr command r/w
const uint8_t MEAS_TEMP   = 0x03;   // 000  0001   1
const uint8_t MEAS_HUMI   = 0x05;   // 000  0010   1
const uint8_t STAT_REG_W  = 0x06;   // 000  0011   0
const uint8_t STAT_REG_R  = 0x07;   // 000  0011   1
const uint8_t SOFT_RESET  = 0x1e;   // 000  1111   0

// Status register writable bits
const uint8_t SR_MASK     = 0x07;

// getByte flags
const bool noACK  = false;
const bool ACK    = true;

// Temperature & humidity equation constants
  const float D1  = -40.1;          // for deg C @ 5V
  const float D2h =   0.01;         // for deg C, 14-bit precision
  const float D2l =   0.04;         // for deg C, 12-bit precision

//  const float C1  = -4.0000;        // for V3 sensors
//  const float C2h =  0.0405;        // for V3 sensors, 12-bit precision
//  const float C3h = -2.8000E-6;     // for V3 sensors, 12-bit precision
//  const float C2l =  0.6480;        // for V3 sensors, 8-bit precision
//  const float C3l = -7.2000E-4;     // for V3 sensors, 8-bit precision
  const float C1  = -2.0468;        // for V4 sensors
  const float C2h =  0.0367;        // for V4 sensors, 12-bit precision
  const float C3h = -1.5955E-6;     // for V4 sensors, 12-bit precision
  const float C2l =  0.5872;        // for V4 sensors, 8-bit precision
  const float C3l = -4.0845E-4;     // for V4 sensors, 8-bit precision

  const float T1  =  0.01;          // for V3 and V4 sensors
  const float T2h =  0.00008;       // for V3 and V4 sensors, 12-bit precision
  const float T2l =  0.00128;       // for V3 and V4 sensors, 8-bit precision


/******************************************************************************
 * Constructors
 ******************************************************************************/

sht_handler::sht_handler(uint8_t data_pin, uint8_t clock_pin) {
  // Initialize private storage for library functions
  this->data_pin = data_pin;
  this->clock_pin = clock_pin;
  this->_presult = NULL;                  // No pending measurement
  this->_stat_reg = 0x00;                 // Sensor status register default state

  // Initialize CLK signal direction
  // Note: All functions exit with CLK low and DAT in input mode
//  pinMode(clock_pin, OUTPUT);

  // Return sensor to default state
  resetConnection();                // Reset communication link with sensor
  put_byte(SOFT_RESET);              // Send soft reset command
}


/******************************************************************************
 * User functions
 ******************************************************************************/

// All-in-one (blocking): Returns temperature, humidity, & dewpoint
uint8_t sht_handler::measure(float *temp, float *humi) {
  uint16_t raw_data;
  uint8_t error;
  pinMode(data_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  if ((error = measTemp(&raw_data)))
    return error;
  *temp = calc_t(raw_data);
  if ((error = measHumi(&raw_data)))
    return error;
  *humi = calc_rh(raw_data, *temp);
  return 0 ;
}

// Initiate measurement.  If blocking, wait for result
uint8_t sht_handler::meas(uint8_t cmd, uint16_t *result, bool block) {
  uint8_t error, i;
  _crc = bitrev(_stat_reg & SR_MASK);  // Initialize CRC calculation
  startTransmission();
  if (cmd == TEMP)
    cmd = MEAS_TEMP;
  else
    cmd = MEAS_HUMI;
  if ((error = put_byte(cmd)))
    return error;
  calc_crc(cmd, &_crc);              // Include command byte in CRC calculation
  // If non-blocking, save pointer to result and return
  if (!block) {
    _presult = result;
    return 0;
  }
  // Otherwise, wait for measurement to complete with 720ms timeout
  i = 240;
  while (digitalRead(data_pin)) {
    i--;
    if (i == 0)
      return S_Err_TO;              // Error: Timeout
    delay(3);
  }
  error = get_result(result);
  return error;
}

// Check if non-blocking measurement has completed
// Non-zero return indicates complete (with or without error)
uint8_t sht_handler::measRdy(void) {
  uint8_t error = 0;
  if (_presult == NULL)             // Already done?
    return S_Meas_Rdy;
  if (digitalRead(data_pin) != 0)   // Measurement ready yet?
    return 0;
  error = get_result(_presult);
  _presult = NULL;
  if (error)
    return error;                   // Only possible error is S_Err_CRC
  return S_Meas_Rdy;
}

// Get measurement result from sensor (plus CRC, if enabled)
uint8_t sht_handler::get_result(uint16_t *result) {
  uint8_t val;
  val = get_byte(ACK);
  calc_crc(val, &_crc);
  *result = val;
  val = get_byte(ACK);
  calc_crc(val, &_crc);
  *result = (*result << 8) | val;
  val = get_byte(noACK);
  val = bitrev(val);
  if (val != _crc) {
    *result = 0xFFFF;
    return S_Err_CRC;
  }
  return 0;
}

// Write status register
uint8_t sht_handler::writeSR(uint8_t value) {
  uint8_t error;
  value &= SR_MASK;                 // Mask off unwritable bits
  _stat_reg = value;                // Save local copy
  startTransmission();
  if ((error = put_byte(STAT_REG_W)))
    return error;
  return put_byte(value);
}

// Read status register
uint8_t sht_handler::readSR(uint8_t *result) {
  uint8_t val;
  uint8_t error = 0;
  _crc = bitrev(_stat_reg & SR_MASK);  // Initialize CRC calculation
  startTransmission();
  if ((error = put_byte(STAT_REG_R))) {
    *result = 0xFF;
    return error;
  }
  calc_crc(STAT_REG_R, &_crc);       // Include command byte in CRC calculation
  *result = get_byte(ACK);
  calc_crc(*result, &_crc);
  val = get_byte(noACK);
  val = bitrev(val);
  if (val != _crc) {
    *result = 0xFF;
    error = S_Err_CRC;
  }
  return error;
}

// Public reset function
// Note: Soft reset returns sensor status register to default values
uint8_t sht_handler::reset(void) {
  _stat_reg = 0x00;                 // Sensor status register default state
  resetConnection();                // Reset communication link with sensor
  return put_byte(SOFT_RESET);       // Send soft reset command & return status
}


/******************************************************************************
 * sht_handler data communication
 ******************************************************************************/

// Write byte to sensor and check for acknowledge
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


/******************************************************************************
 * sht_handler signaling
 ******************************************************************************/

// Generate sht_handler-specific transmission start sequence
// This is where sht_handler does not conform to the I2C standard and is
// the main reason why the AVR TWI hardware support can not be used.
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______
void sht_handler::startTransmission(void) {
  digitalWrite(data_pin, HIGH);  // Set data register high before turning on
  pinMode(data_pin, OUTPUT);     // output driver (avoid possible low pulse)
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
  // Unnecessary here since putByte always follows startTransmission
//  pinMode(data_pin, INPUT);
}

// Communication link reset
// At least 9 SCK cycles with DATA=1, followed by transmission start sequence
//      ______________________________________________________         ________
// DATA:                                                      |_______|
//          _    _    _    _    _    _    _    _    _        ___     ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
void sht_handler::resetConnection(void) {
  uint8_t i;
  digitalWrite(data_pin, HIGH);  // Set data register high before turning on
  pinMode(data_pin, OUTPUT);     // output driver (avoid possible low pulse)
  PULSE_LONG;
  for (i = 0; i < 9; i++) {
    digitalWrite(clock_pin, HIGH);
    PULSE_LONG;
    digitalWrite(clock_pin, LOW);
    PULSE_LONG;
  }
  startTransmission();
}


/******************************************************************************
 * Helper Functions
 ******************************************************************************/

// Calculates temperature in degrees C from raw sensor data
float sht_handler::calc_t(uint16_t raw_data) {
  if (_stat_reg & LOW_RES)
    return D1 + D2l * (float) raw_data;
  else
    return D1 + D2h * (float) raw_data;
}

// Calculates relative humidity from raw sensor data
//   (with temperature compensation)
float sht_handler::calc_rh(uint16_t raw_data, float temp) {
  float humi;
  if (_stat_reg & LOW_RES) {
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
