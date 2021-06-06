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

#include "settings.h"
#include "icons.h"
#include "fonts.h"

#include <DHT10.h>
DHT10 DHT;

struct {
    String Version = "1.1.0";
    String ProjectID = "P231";
    String URL = "https://make.quwj.com/project/231";
    String Author = "NXEZ.com";
} AppInfo;

String clientId = "CUBE-";

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
    display->drawString(64 + x, 2 + y, date_str);
    display->setFont(ArialMT_Plain_24);

    sprintf(time_str, "%02d:%02d:%02d", now.hour(),now.minute(),now.second());
    display->drawString(64 + x, 19 + y, time_str);
}

void drawTempHumi(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y){
    char temp_str[36];

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 0 + y, "- Indoor Sensor -");

    if(TEMPERATURE_SENSOR_TYPE == "DHT10"){
        sprintf(temp_str, "%d C   %d %%", (int)DHT.temperature, (int)DHT.humidity);
    }
    else if(TEMPERATURE_SENSOR_TYPE == "DHT22"){
        sprintf(temp_str, "%d C   %d %%", (int)temperature, (int)humidity);
    }
    else{
        Serial.println("unknown TEMPERATURE_SENSOR_TYPE");
    }

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
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
    display->drawString(5, 52, time_str);

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(84, 52, temp_str);

    int8_t quality = getWifiQuality();
    for (int8_t i = 0; i < 4; i++){
        for (int8_t j = 0; j < 2 * (i + 1); j++){
            if (quality > i * 25 || j == 0){
                display->setPixel(120 + 2 * i, 61 - j);
            }
        }
    }

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawHorizontalLine(0, 51, 128);
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

void setup(){
    WiFi.mode(WIFI_STA);
    Serial.begin(115200);
    Serial.println();

    clientId += String(ESP.getChipId(), HEX);
    clientId.toUpperCase();
    Serial.println("clientId: " + (String)(clientId.c_str()));

    if(TEMPERATURE_SENSOR_TYPE == "DHT10"){
        DHT.begin();
        Serial.print("DHT10 library version: ");
        Serial.println(DHT10_VERSION);
    }

    // initialize dispaly
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.flipScreenVertically();
    drawSplash(&display, "- NXEZ CUBE -\nwww.nxez.com");

    pinMode(D6, OUTPUT);
    tone(D6, 800);
    delay(100);
    tone(D6, 0);
    delay(100);
    tone(D6, 600);
    delay(100);
    tone(D6, 0);
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

    updateData(&display);
    updateDataDHT();

    ticker.attach(UPDATE_INTERVAL_SECS, setReadyForUpdate);
    ticker2.attach(UPDATE_INTERVAL_SECS_DHT, setReadyForDHTUpdate);
}

void loop(){
    now = baseTime.operator+(TimeSpan((millis()-millisTimeUpdated)/1000));

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