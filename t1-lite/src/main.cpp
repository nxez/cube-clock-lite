/**
Copyright (c) 2021 by NXEZ.com
See more at https://www.nxez.com
*/
#include <FS.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
//#include <SSD1306Wire.h>
#include <SH1106.h>
#include <OLEDDisplayUi.h>
#include <Wire.h>
#include <RTClib.h>
#include <EasyNTPClient.h>
#include <SimpleDHT.h>
#include <time.h>
#include <WiFiUdp.h>
#include <DoubleResetDetect.h>

#include "DSEG7Classic-BoldFont.h"
#include "settings.h"
#include "icons.h"
#include "fonts.h"

#include <DHT10.h>
DHT10 DHT;

struct {
    String Version = "1.0.0";
    String ProjectID = "P405-Lite";
    String URL = "https://make.quwj.com/project/405";
    String Author = "NXEZ.com";
} AppInfo;

String clientId = "CUBE-";

bool updating = false;
int frameIndex = 0;
int enableAutoTransition = 1;
bool screenOn = true;
int batteryRemaining = 0;

int tasterPin[] = {D6, D5, D4};
int timeoutTaster[] = {0, 0, 0, 0};
bool pushed[] = {false, false, false, false};
int blockTimeTaster[] = {0, 0, 0, 0};
bool blockTaster[] = {false, false, false, false};
bool blockTaster2[] = {false, false, false, false};
bool tasterState[3];

#define DHT_PIN D5
//SimpleDHT11 dht11(DHT_PIN);
SimpleDHT22 dht(DHT_PIN);
byte temperature = 0;
byte humidity = 0;

WiFiUDP ntpUDP;
EasyNTPClient ntpClient(ntpUDP, NTP_SERVERS, 3600 * UTC_OFFSET);
unsigned long millisTimeUpdated = millis(); 

bool readyForUpdate = false;
bool readyForDHTUpdate = false;

String lastUpdate = "--";

Ticker ticker;
Ticker ticker2;
DateTime baseTime;
DateTime now;

char monName[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char wdayName[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

//declaring prototypes
void updateData(OLEDDisplay *display);
void updateDataDHT();
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawTempHumi(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state);
int8_t getWifiQuality();
int8_t getbatteryRemaining();
int checkTaster(int nr);

// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = {drawDateTime, drawTempHumi};
int numberOfFrames = 2;

OverlayCallback overlays[] = {drawHeaderOverlay};
int numberOfOverlays = 1;

void drawSplash(OLEDDisplay *display, String label){
    display->clear();
    display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64, 32, label);
    display->display();
}

void setReadyForUpdate() {
    readyForUpdate = true;
}

void setReadyForDHTUpdate() {
    readyForDHTUpdate = true;
}

void drawProgress(OLEDDisplay *display, int percentage, String label){
    display->clear();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64, 10, label);
    display->drawProgressBar(10, 28, 108, 12, percentage);
    display->display();
}

void updateData(OLEDDisplay *display){
    drawProgress(display, 10, "Updating time..");
    DateTime dt = DateTime(ntpClient.getUnixTime());
    baseTime = DateTime(dt.unixtime());
    millisTimeUpdated = millis();

    readyForUpdate = false;
    drawProgress(display, 100, "Done!");
    delay(1000);
}

void updateDataDHT(){
    //DHT BEGIN
    if(TEMPERATURE_SENSOR_TYPE == "DHT10"){
        Serial.print("Reading DHT10...");
        int status = DHT.read();
        if(status == DHT10_OK){
            //Serial.print(DHT.humidity);
            //Serial.print(",\t");
            //Serial.println(DHT.temperature);  
            Serial.println("OK");
        }
        delay(2000);   //recommend delay 2 second 
    }
    else if(TEMPERATURE_SENSOR_TYPE == "DHT22"){
        // read without samples.
        int err = SimpleDHTErrSuccess;
        Serial.print("Reading DHT22...");
        if ((err = dht.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
            Serial.print("Read DHT22 failed, err=");
            Serial.println(err);
            //delay(1000);
            return;
        }
        Serial.println("OK");
    }
    else{
        Serial.println("unknown TEMPERATURE_SENSOR_TYPE");
    }
    //DHT END

    readyForDHTUpdate = false;
    batteryRemaining = getbatteryRemaining();
}

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y){
    char date_str[20];
    char time_str[11];

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    snprintf_P(date_str,
    sizeof(date_str),
    PSTR("%03s %03s %02u %04u"),
    wdayName[now.dayOfTheWeek()], monName[now.month() - 1],
    now.day(), now.year());
    int textWidth = display->getStringWidth(date_str);
    display->drawString(64 + x, 2 + y, date_str);
    display->setFont(DSEG7_Classic_Bold_21);

    sprintf(time_str, "%02d:%02d:%02d", now.hour(),now.minute(),now.second());
    display->drawString(64 + x, 19 + y, time_str);
}

void drawTempHumi(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y){
    char temp_str[36];

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 0 + y, "- Indoor Sensor -");

    if(TEMPERATURE_SENSOR_TYPE == "DHT10"){
        sprintf(temp_str, "%d C   %d P", (int)DHT.temperature, (int)DHT.humidity);
    }
    else if(TEMPERATURE_SENSOR_TYPE == "DHT22"){
        sprintf(temp_str, "%d C   %d P", (int)temperature, (int)humidity);
    }
    else{
        Serial.println("unknown TEMPERATURE_SENSOR_TYPE");
    }

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(DSEG7_Classic_Bold_14);
    display->drawString(x+64, y+16, temp_str);

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(x+62, y+35, "TEMPER   HUMIDI");
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state){
    char time_str[11];
    char temp_str[20];

    if(TEMPERATURE_SENSOR_TYPE == "DHT10"){
        sprintf(temp_str, "%d°C H:%d%%", (int)DHT.temperature, (int)DHT.humidity);
    }
    else if(TEMPERATURE_SENSOR_TYPE == "DHT22"){
        sprintf(temp_str, "%d°C H:%d%%", (int)temperature, (int)humidity);
    }
    else{
        Serial.println("unknown TEMPERATURE_SENSOR_TYPE");
    }

    display->setFont(ArialMT_Plain_10);

    #ifdef STYLE_24HR
    sprintf(time_str, "%02d:%02d\n", now.hour(), now.minute());
    #else
    int hour = (now.hour() + 11) % 12 + 1; // take care of noon and midnight
    //sprintf(time_str, "%2d:%02d:%02d%s\n", hour, now.minute(), now.second(), now.hour() >= 12 ? "pm" : "am");
    sprintf(time_str, "%2d:%02d%s\n", hour, now.minute(), now.hour() >= 12 ? "pm" : "am");
    #endif

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    //display->drawString(5, 52, time_str);
    display->drawString(13, 52, time_str);

    
    /*
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    //String temp = "20°C H:99%";
    //display->drawString(101, 52, temp);
    display->drawString(84, 52, temp_str);
    */

    int offset_x = -120;
    int8_t quality = getWifiQuality();
    for (int8_t i = 0; i < 4; i++){
        for (int8_t j = 0; j < 2 * (i + 1); j++){
            if (quality > i * 25 || j == 0){
                display->setPixel(120 + 2 * i + offset_x, 61 - j);
            }
        }
    }

    //battery
    offset_x = 116;
    //Serial.println(batteryRemaining);
    for (int8_t i = 0; i < batteryRemaining; i++){
        for (int8_t j = 0; j < 7; j++){
            display->setPixel(i * 2 + offset_x, 61 - j);
            display->setPixel(i * 2 + 1 + offset_x, 61 - j);
        }
    }

    for (int8_t i = 0; i < 5; i++){
        display->setPixel(i * 2 + offset_x, 61 - 0);
        display->setPixel(i * 2 + 1 + offset_x, 61 - 0);

        display->setPixel(i * 2 + offset_x, 61 - 6);
        display->setPixel(i * 2 + 1 + offset_x, 61 - 6);
    }

    for (int8_t j = 0; j < 7; j++){
        display->setPixel(0 + offset_x, 61 - j);
        display->setPixel(5 * 2 + offset_x, 61 - j);
    }

    display->setPixel(5 * 2 + 1 + offset_x, 61 - 4);
    display->setPixel(5 * 2 + 1 + offset_x, 61 - 3);
    display->setPixel(5 * 2 + 1 + offset_x, 61 - 2);

    //end battery

    if(!enableAutoTransition){
        display->drawString(98, 52, "LK");
    }
    else{
        display->drawString(98, 52, "  ");
    }

    display->drawHorizontalLine(0, 51, 128);
}

void setup(){
    pinMode(D4, INPUT_PULLUP);
    pinMode(D5, INPUT_PULLUP);
    pinMode(D6, INPUT_PULLUP);

    if(TEMPERATURE_SENSOR_TYPE == "DHT10"){
        DHT.begin();
        delay(500);
    }

    WiFi.mode(WIFI_STA);
    Serial.begin(115200);
    Serial.println();

    clientId += String(ESP.getChipId(), HEX);
    clientId.toUpperCase();
    Serial.println("clientId: " + (String)(clientId.c_str()));

    // initialize dispaly
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.flipScreenVertically();
    drawSplash(&display, "- NXEZ CUBE -\nwww.nxez.com");

    pinMode(D0, OUTPUT);
    tone(D0, 800);
    delay(100);
    tone(D0, 0);
    delay(100);
    tone(D0, 600);
    delay(100);
    tone(D0, 0);
    delay(1500);
    
    drawSplash(&display, "Cube Clock\nver: " + AppInfo.Version +"\n(" + AppInfo.ProjectID + ")");
    delay(1500);

    drawSplash(&display, "Connecting to WiFi..");

    WiFi.begin(WIFI_SSID, WIFI_PWD);

    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }

    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
   
    ui.setTargetFPS(30);
    // Setup frame display time to 10 sec
    ui.setTimePerFrame(10000);

    // Hack until disableIndicator works:
    // Set an empty symbol
    ui.setActiveSymbol(emptySymbol);
    ui.setInactiveSymbol(emptySymbol);

    ui.disableIndicator();
    ui.setFrameAnimation(SLIDE_LEFT);

    // Add frames
    ui.setFrames(frames, numberOfFrames);
    ui.setOverlays(overlays, numberOfOverlays);

    if(enableAutoTransition){
        ui.enableAutoTransition();
    }
    else{
        ui.disableAutoTransition();
    }

    ui.switchToFrame(frameIndex);

    updateData(&display);
    updateDataDHT();

    ticker.attach(UPDATE_INTERVAL_SECS, setReadyForUpdate);
    ticker2.attach(UPDATE_INTERVAL_SECS_DHT, setReadyForDHTUpdate);
}

void loop(){
    now = baseTime.operator+(TimeSpan((millis()-millisTimeUpdated)/1000));

	checkTaster(0);
	checkTaster(1);
	checkTaster(2);

    if(readyForDHTUpdate){
        updateDataDHT();
    }

    if (readyForUpdate && ui.getUiState()->frameState == FIXED) {
        updateData(&display);
    }

    int remainingTimeBudget = ui.update();

    if (remainingTimeBudget > 0) {
        // You can do some work here
        // Don't do stuff if you are below your
        // time budget.
        delay(remainingTimeBudget);
    }
}

int checkTaster(int nr){
	tasterState[0] = digitalRead(tasterPin[0]);
	tasterState[1] = digitalRead(tasterPin[1]);
	tasterState[2] = digitalRead(tasterPin[2]);

	switch (nr)	{
	case 0:
		if (tasterState[0] == LOW && !pushed[nr] && !blockTaster2[nr] && tasterState[1] && tasterState[2]){
			pushed[nr] = true;
			timeoutTaster[nr] = millis();
		}
		break;
	case 1:
		if (tasterState[1] == LOW && !pushed[nr] && !blockTaster2[nr] && tasterState[0] && tasterState[2]){
			pushed[nr] = true;
			timeoutTaster[nr] = millis();
		}
		break;
	case 2:
		if (tasterState[2] == LOW && !pushed[nr] && !blockTaster2[nr] && tasterState[0] && tasterState[1]){
			pushed[nr] = true;
			timeoutTaster[nr] = millis();
		}
		break;
	case 3:
		if (tasterState[0] == LOW && tasterState[2] == LOW && !pushed[nr] && !blockTaster2[nr] && tasterState[1]){
			pushed[nr] = true;
			timeoutTaster[nr] = millis();
		}
		break;
	}

	if (pushed[nr] && (millis() - timeoutTaster[nr] < 2000) && tasterState[nr] == HIGH){
		if (!blockTaster2[nr]){
			switch (nr){
			case 0:
				Serial.println("LEFT: normaler Tastendruck");
                //transitionToFrame("previous");
                ui.previousFrame();
				break;
			case 1:
				Serial.println("MID: normaler Tastendruck");
                //updateWeatherCurrent();
                screenOn = true;
                display.displayOn();

                enableAutoTransition = !enableAutoTransition;

                if(enableAutoTransition){
                    ui.enableAutoTransition();
                }
                else{
                    ui.disableAutoTransition();
                }

				break;
			case 2:
				Serial.println("RIGHT: normaler Tastendruck");
                //transitionToFrame("next");
                ui.nextFrame();
				break;
			}

			pushed[nr] = false;
			return 1;
		}
	}

	if (pushed[nr] && (millis() - timeoutTaster[nr] > 2000)){
		if (!blockTaster2[nr]){
			switch (nr){
			case 0:
				Serial.println("LEFT: langer Tastendruck");
				break;
			case 1:
				Serial.println("MID: langer Tastendruck");
                screenOn = !screenOn;
                if(screenOn){
                    display.displayOn();
                }
                else{
                    display.displayOff();
                }

				break;
			case 2:
				Serial.println("RIGHT: langer Tastendruck");
				break;
			case 3:
				break;
			}
			blockTaster[nr] = true;
			blockTaster2[nr] = true;
			pushed[nr] = false;
			return 2;
		}
	}
	if (nr == 3){
		if (blockTaster[nr] && tasterState[0] == HIGH && tasterState[2] == HIGH){
			blockTaster[nr] = false;
			blockTimeTaster[nr] = millis();
		}
	}
	else
	{
		if (blockTaster[nr] && tasterState[nr] == HIGH){
			blockTaster[nr] = false;
			blockTimeTaster[nr] = millis();
		}
	}

	if (!blockTaster[nr] && (millis() - blockTimeTaster[nr] > 500)){
		blockTaster2[nr] = false;
	}
	return 0;
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality(){
    int32_t dbm = WiFi.RSSI();
    if (dbm <= -100){
        return 0;
    }
    else if (dbm >= -50){
        return 100;
    }
    else{
        return 2 * (dbm + 100);
    }
}

int8_t getbatteryRemaining(){
    int vola = 0;
    vola = analogRead(A0);
    double vol = ((vola/1023.0)*(110+442))/110.0;

    //Serial.println(vola);
    Serial.printf("vol: %f\n", ((vola/1023.0)*(110+442))/110.0);

    //int32_t percent = 0;

    if (vol <= 3.2){
        return 0;
    }

    if (vol >= 4.2){
        return 5;
    }
    
    if (vol >= 4.0){
        return 4;
    }
    
    if (vol >= 3.8){
        return 4;
    }
    
    if (vol >= 3.6){
        return 3;
    }
    
    if (vol >= 3.4){
        return 2;
    }
    
    if (vol >= 3.3){
        return 1;
    }

    return 0;
}