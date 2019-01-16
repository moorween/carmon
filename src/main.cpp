#include <SPI.h>
#include <Wire.h>
#include "TinyGPS++.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP8266_SSD1322.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include "../lib/utils.cpp"

#define OLED_CS     45  // Pin 10, CS - Chip select
#define OLED_DC     48   // Pin 9 - DC digital signal
#define OLED_RESET  49   // using hardware !RESET from Arduino instead

//hardware SPI - only way to go. Can get 110 FPS
ESP8266_SSD1322 display(OLED_DC, OLED_RESET, OLED_CS);
TinyGPSPlus gpsParser;

void setup()   {      
  Serial.begin(9600);
  Serial2.begin(9600);
  display.begin(true);

  display.clearDisplay();
  display.setFont(&FreeSerif9pt7b);
  display.setTextColor(WHITE);

}


void loop() {
   while(Serial2.available() > 0) {
        char temp = Serial2.read();
//        Serial.write(temp);
        gpsParser.encode(temp);
    }

    String lat  = "Unknown         ";
    String lng  = "location        ";
    if(gpsParser.location.isValid()) {
        lat = "Lat: " + String(gpsParser.location.lat(), 10);
        lng = "Lng: " + String(gpsParser.location.lng(), 10);
        display.setCursor(128,12);
        display.println(lat + lng);
        Serial.println(lat + lng);
    } else {
//      Serial.println("Location invalid");
    }
  display.setCursor(128,12);
        display.println(lat + lng);
  float val1 = analogRead(A5);
  float p1 = mapVal(voltVal(val1));
  
  p1 = p1 / 1000;
  display.clearDisplay();
  display.setCursor(0,12);
  display.setFont(&FreeSerif9pt7b);
  display.println("Boost");
  display.setCursor(0,30);
  display.setFont(&FreeSerif12pt7b);
  display.println(p1);
 
  float val2 = analogRead(A9);
  
  float res = resVal(voltVal(val2), 10000);
  //val2 = (5 / val2) -1;
  //val2 = 10000 * val2;
  display.setCursor(0,44);
  display.setFont(&FreeSerif9pt7b);
  display.println("Air");
  display.setCursor(0,60);
  display.setFont(&FreeSerif12pt7b);
  display.println(getIat(res));
  display.display();
  delay(500);
}