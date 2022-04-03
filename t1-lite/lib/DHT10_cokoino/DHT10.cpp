// FILE: DHT10.cpp
// AUTHOR: mo cokoino
// PURPOSE: I2C library for DHT10 for Arduino.
// VERSION: 0.0.1
// HISTORY: See DHT10.cpp
// URL: https://github.com/Cokoino/DHT10-lilbrary

#include <Wire.h>
#include "DHT10.h"

#define DHT10_ADDRESS  ((uint8_t)0x38)

////////////////////////////////////////////////////////////////////
void DHT10::begin(){
  Wire.begin();
}
////////////////////////////////////////////////////////////////////.
int8_t DHT10::read(){
  //trigger measurement data
  delay(50);
  Wire.beginTransmission(DHT10_ADDRESS);
  Wire.write(0xac);
  Wire.write(0x33);
  Wire.write(0x00);
  int8_t status = Wire.endTransmission();
  if(status !=DHT10_OK) return DHT10_CONNECT_ERROR;
  
  //check busy                  
  char ct = 0;
  uint8_t st;
  do{
    Wire.requestFrom(DHT10_ADDRESS, 1);
    st = Wire.read();
    delay(4);
    if(ct++>25) return DHT10_BUSY;
    }while((st & 0x80)==0x80);
  
  //check CAL 
  if((st & 0x08)!=0x08){ 
    Wire.beginTransmission(DHT10_ADDRESS);
    Wire.write(0xe1);     //configrate DHT10
    Wire.write(0x08);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(50);
    Wire.beginTransmission(DHT10_ADDRESS); 
    Wire.write(0xba);    //soft reset DHT10
    Wire.endTransmission();
    delay(200);
    return DHT10_CAL_ERROR;
  }
  
  //read humidity and temperature
  Wire.requestFrom(DHT10_ADDRESS, 6);
  st = Wire.read();
  for (int i = 0; i < 5; i++){
    data[i] = Wire.read();
  }
  
  //processing data
  temperature_humidity();
  return DHT10_OK;
}
////////////////////////////////////////////////////////////////////
void DHT10::temperature_humidity(void){
  humidity = ((float)data[0])/256;
  humidity = humidity + ((float)data[1])/64/1024;
  humidity = humidity + ((float)data[2])/16/1024/1024;
  humidity = humidity*100;
  temperature = ((float)(data[2]%16))/16;
  temperature = temperature + ((float)data[3])/4096;
  temperature = temperature + ((float)data[4])/1024/1024;
  temperature = temperature*200 - 50;
}
// end of file
