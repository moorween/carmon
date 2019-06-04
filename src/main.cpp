#include <SPI.h>
#include <Wire.h>
#include "TinyGPS++.h"
#include <Arduino.h>
#include "../lib/utils.cpp"
// #include "U8glib.h"
#include <U8g2lib.h>
#include <stdlib.h>

#define OLED_CS 45    // Pin 10, CS - Chip select
#define OLED_DC 48    // Pin 9 - DC digital signal
#define OLED_RESET 49 // using hardware !RESET from Arduino instead

U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g(U8G2_R2, OLED_CS, OLED_DC, OLED_RESET);

struct sensor
{
  int port;
  int type; // 1 = GM- Map, 2 = GM IAT sensor, 3 = pressure sensor, 4 = DS18b20
  int displayPosition;
  char serialMark[14];
  char title[14];
  double value;
  int decimals;
  double minValue;
  double maxValue;
  boolean large;
  int resistanceRef;
  int averageInterval;
};

sensor sensors[] = {
  {A4, 2, 2, "IAT", "IAT", 20.3, 0, 0, 0, false, 10000},
  {A5, 3, 3, "FPRESS", "Fuel Press", 4.1, 2, 0, 0, false, 0},
  {A3, 1, 1, "BOOST", "Boost", 0.32, 2, 0, 0, true, 0},
  {A6, 3, 3, "ATPRESS", "AT Press", 4.1, 1, 0, 0, false, 0},
  {A6, 3, 3, "ATTEMP", "AT Temp", 42.3, 0, 0, 0, false, 0}
};

void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600);

  u8g.begin();
  u8g.setFont(u8g2_font_5x8_mf);
  u8g.setFontPosTop();
  u8g.setColorIndex(1);
}

void loop()
{
  int displayX = 0;
  int displayY = 0;
  u8g.clearBuffer();

  for(signed int i = 0; i < sizeof(sensors)/sizeof(sensor); i++) {
      int yPlus = 32;
      float val = analogRead(sensors[i].port);

      switch (sensors[i].type) {
        case 1: //GM Map
          sensors[i].value = mapVal(voltVal(val)) / 1000;
          break;
        case 2: // IAT
          sensors[i].value = getIat(resVal(voltVal(val), sensors[i].resistanceRef));
          break;
        case 3:
          break;
      }
      
      // sensors[i].value = u8g.getDisplayWidth();

      char tmp_string[128];
      dtostrf(sensors[i].value, 2, sensors[i].decimals, tmp_string);  
      
      if (sensors[i].value < 0) {
        if (tmp_string[1] == '0') {
          tmp_string[1] = "";
        }
      }

      if (sensors[i].large) {
        u8g.setFont(u8g2_font_celibatemonk_tr);
        u8g.drawStr(displayX + 10, displayY, sensors[i].title);
        u8g.setFont(u8g2_font_osr35_tn); //u8g2_font_fub20_tn
        u8g.drawStr(displayX - 15, displayY + 20, tmp_string);
        yPlus = 64;
      } else {

        if (displayX > 180) {
          u8g.setFont(u8g2_font_5x8_mf);
          u8g.drawStr(u8g.getDisplayWidth() - u8g.getStrWidth(sensors[i].title), displayY, sensors[i].title);
          u8g.setFont(u8g2_font_helvR18_tn); //u8g2_font_fub20_tn
          u8g.drawStr(u8g.getDisplayWidth() - u8g.getStrWidth(tmp_string), displayY + 8, tmp_string);
        } else {
          u8g.setFont(u8g2_font_5x8_mf);
          u8g.drawStr(displayX, displayY, sensors[i].title);
          u8g.setFont(u8g2_font_helvR18_tn); //u8g2_font_fub20_tn
          u8g.drawStr(displayX, displayY + 8, tmp_string);
        }
      }

      if ((displayY + yPlus) >= 64) {
        displayY = 0;
        displayX += 94;
      } else {
        displayY += yPlus;
      }
  }
  u8g.drawLine(65, 5, 65, 59);
  u8g.drawLine(191, 5, 191, 59);
  u8g.sendBuffer();
}