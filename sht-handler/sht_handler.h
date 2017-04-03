#ifndef SHT_HANDLER_H
#define SHT_HANDLER_H

#include <stdint.h>


// Enable ('1') or disable ('0') internal pullup on DATA line
// Commenting out this #define saves code space but leaves internal pullup
//   state undefined (ie, depends on last bit transmitted)
#define DATA_PU 1

// Clock pulse timing macros
// Lengthening these may assist communication over long wires
#define PULSE_LONG  delayMicroseconds(3)
#define PULSE_SHORT delayMicroseconds(1)

// Useful macros
#define measTemp(result)  meas(TEMP, result, BLOCK)
#define measHumi(result)  meas(HUMI, result, BLOCK)

// User constants
const uint8_t TEMP     =     0;
const uint8_t HUMI     =     1;
const bool    BLOCK    =  true;
const bool    NONBLOCK = false;

// Status register bit definitions
const uint8_t LOW_RES  =  0x01;  // 12-bit Temp / 8-bit RH (vs. 14 / 12)
const uint8_t NORELOAD =  0x02;  // No reload of calibrarion data
const uint8_t HEAT_ON  =  0x04;  // Built-in heater on
const uint8_t BATT_LOW =  0x40;  // VDD < 2.47V

// Function return code definitions
const uint8_t S_Err_NoACK  = 1;  // ACK expected but not received
const uint8_t S_Err_CRC    = 2;  // CRC failure
const uint8_t S_Err_TO     = 3;  // Timeout
const uint8_t S_Meas_Rdy   = 4;  // Measurement ready

class sht_handler
{
  private:
    uint8_t data_pin;
    uint8_t clock_pin;
    uint16_t *_presult;
    uint8_t _stat_reg;
    uint8_t _crc;
    uint8_t get_result(uint16_t *result);
    uint8_t put_byte(uint8_t value);
    uint8_t get_byte(bool ack);
    void startTransmission(void);
    void resetConnection(void);
    void calc_crc(uint8_t value, uint8_t *crc);
    uint8_t bitrev(uint8_t value);

  public:
    sht_handler(uint8_t data_pin, uint8_t clock_pin);
    uint8_t measure(float *temp, float *humi);   
    uint8_t meas(uint8_t cmd, uint16_t *result, bool block);
    uint8_t measRdy(void);
    uint8_t writeSR(uint8_t value);
    uint8_t readSR(uint8_t *result);
    uint8_t reset(void);
    float calc_t(uint16_t raw_data);
    float calc_rh(uint16_t raw_data, float temp);
};

#endif 
