#ifndef DHT10_H
#define DHT10_H
//
// FILE: DHT10.h
// AUTHOR: mo cokoino
// PURPOSE: DHT_I2C library for Arduino .
// VERSION: 0.0.1
// URL: https://github.com/Cokoino/DHT10-lilbrary

#include "Wire.h"
#include "Arduino.h"

#define DHT10_VERSION "0.0.1"
#define DHT10_OK      0
#define DHT10_CONNECT_ERROR  -1
#define DHT10_BUSY           -2
#define DHT10_CAL_ERROR      -3

class DHT10
{
public:
  void begin();
  int8_t read();
  void temperature_humidity();
  float humidity;
  float temperature;
private:
  uint8_t data[5];
};

#endif
// END OF FILE
