#include <SPI.h>
#include <Wire.h>
// #include "TinyGPS++.h"
#include <Arduino.h>
#include "../lib/utils.cpp"
// #include "U8glib.h"
#include <U8g2lib.h>
#include <stdlib.h>
#include <OneWire.h>

#define OLED_CS 45    // Pin 10, CS - Chip select
#define OLED_DC 48    // Pin 9 - DC digital signal
#define OLED_RESET 49 // using hardware !RESET from Arduino instead

U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g(U8G2_R2, OLED_CS, OLED_DC, OLED_RESET);
OneWire ds(40);

int contrast = 0;
int temperature = 0; // Глобальная переменная для хранения значение температуры с датчика DS18B20

long lastUpdateTime = 0; // Переменная для хранения времени последнего считывания с датчика
const int TEMP_UPDATE_TIME = 1000; // Определяем пе

struct sensor
{
  int port;
  int type; // 1 = GM- Map, 2 = GM IAT sensor, 3 = pressure sensor, 4 = DS18b20, 5 = EGT
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
  {A15, 2, 2, "IAT", "IAT", 20.3, 0, 0, 0, false, 1000},
  {A11, 5, 1, "EGT", "EGT", 343, 0, 0, 0, false, 0},
  {A13, 1, 1, "BOOST", "Boost", 0.32, 2, 0, 0, true, 0},
  {A13, 3, 3, "FPRESS", "Fuel P", 4.1, 1, 0, 0, false, 0},
  // {A6, 3, 3, "ATPRESS", "AT Press", 4.1, 1, 0, 0, false, 0},
  // {A6, 3, 3, "ATTEMP", "AT Temp", 42.3, 0, 0, 0, false, 0},
  {40, 4, 3, "CTEMP", "C Temp", 4.1, 0, 0, 0, false, 0},
};

int detectTemperature() {
  byte data[2];
  ds.reset();
  ds.write(0xCC);
  ds.write(0x44);

  if (millis() - lastUpdateTime > TEMP_UPDATE_TIME)
  {
    lastUpdateTime = millis();
    ds.reset();
    ds.write(0xCC);
    ds.write(0xBE);
    data[0] = ds.read();
    data[1] = ds.read();

    // Формируем значение
    temperature = (data[1] << 8) + data[0]; temperature = temperature >> 4;
  }
  return temperature;
}

void setup()
{
  // analogReference(INTERNAL1V1);
  Serial.begin(9600);
  Serial2.begin(9600);

  u8g.begin();
  u8g.setFont(u8g2_font_5x8_mf);
  u8g.setFontPosTop();
  u8g.setContrast(0);

  // u8g.setColorIndex(1);
}

void loop()
{
  
  int displayX = 0;
  int displayY = 0;
  u8g.clearBuffer();
  
  for(signed int i = 0; i < sizeof(sensors)/sizeof(sensor); i++) {
      int yPlus = 32;
      float val = analogRead(sensors[i].port);
      val = analogRead(sensors[i].port);

      switch (sensors[i].type) {
        case 1: //GM Map
        // sensors[i].value = val;
          sensors[i].value = mapVal(voltVal(val)) / 1000;
          Serial.println(sensors[i].value);
          break;
        case 2: // IAT
          sensors[i].value = getIat(resVal(voltVal(val), sensors[i].resistanceRef));
          // sensors[i].value = resVal(voltVal(val), sensors[i].resistanceRef);
          // sensors[i].value = voltVal(val);
          break;
        case 3:
          sensors[i].value = pressVal(voltVal(val));
          break;
        case 4:
          sensors[i].value = temperature;
          break;
        case 5:
          sensors[i].value = voltVal(val) ;//getEGT(voltVal(val) / 1000);
          break;
      }
      // sensors[i].value = val;
      // sensors[i].value = u8g.getDisplayWidth();

      char tmp_string[128];
      dtostrf(sensors[i].value, 2, sensors[i].decimals, tmp_string);  
      // dtostrf(sensors[i].value, 2, 4, tmp_string);  
      if (sensors[i].value < 0) {
        if (tmp_string[1] == '0') {
          tmp_string[1] = "a";
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
      delay(10);
      if (contrast < 255) {
        contrast += 15;
        u8g.setContrast(contrast);
      }
  }
  u8g.drawLine(65, 5, 65, 59);
  u8g.drawLine(191, 5, 191, 59);
  u8g.sendBuffer();
  delay(100);
  detectTemperature();
}