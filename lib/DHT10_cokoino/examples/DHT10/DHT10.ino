// AUTHOR: mo cokoino
// VERSION: 0.0.1
// PURPOSE: Demo for DHT10 I2C humidity & temperature sensor
// URL: https://github.com/Cokoino/DHT10-lilbrary

#include <DHT10.h>
DHT10 DHT;
void setup(){
  DHT.begin();
  Serial.begin(115200);
  Serial.print("DHT10 library version: ");
  Serial.println(DHT10_VERSION);
  delay(2000);
  Serial.println("Humidity(%), Temperature(C)");
}
void loop(){
  int status = DHT.read();
  if(status == DHT10_OK){
    Serial.print(DHT.humidity);
    Serial.print(",\t");
    Serial.println(DHT.temperature);  
  }
  delay(2000);   //recommend delay 2 second 
}
