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

long lastUpdateTime = 0, lastUpdTime = 0; // Переменная для хранения времени последнего считывания с датчика
const int TEMP_UPDATE_TIME = 1000; // Определяем пе

int injDuty = 0, injCounter = 0, spdCounter = 0;
bool injLastState = false;
long injLastUpdate = 0;
int injPerMin = 0, spdPerMin = 0;


struct sensor
{
  int port;
  int type; // 1 = GM- Map, 2 = GM IAT sensor, 3 = pressure sensor, 4 = DS18b20, 5 = EGT, 6 = Speed, 7 = Injector
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
  double rawValue;
};

sensor sensors[] = {
  {0, 7, 1, "INJ", "Inj", 10, 0, 0, 0, false, 0, 1000, 0},
  {1, 6, 2, "SPD", "Spd", 10, 0, 0, 0, false, 0, 1000, 0},
  // {A15, 2, 2, "IAT", "IAT", 20.3, 0, 0, 0, false, 1000, 0, 0},
  // {A11, 5, 1, "EGT", "EGT", 343, 0, 0, 0, false, 0, 0, 0},
  {A13, 1, 1, "BOOST", "Boost", 0.32, 2, 0, 0, true, 0, 0, 0},
  {A9, 3, 3, "FPRESS", "Fuel P", 4.1, 1, 0, 0, false, 0, 0, 0},
  // {A6, 3, 3, "ATPRESS", "AT Press", 4.1, 1, 0, 0, false, 0},
  // {A6, 3, 3, "ATTEMP", "AT Temp", 42.3, 0, 0, 0, false, 0},
  {40, 4, 3, "CTEMP", "C Temp", 1, 0, 0, 0, false, 0, 0, 0},
};
// форсунка 6
// датчик скорости  7
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

struct delays
{
  int index;
  int delay = 0;
};
int timings[];

void once(delays timeout, void (*callback)(void)) {
  if (millis() - timings[timeout.index] >= timeout.delay)
  {
    timings[timeout.index] = millis();
    callback();
  }
}

void injStateChange() {
  injCounter ++;
  Serial.println("INJ ++");
  bool state = digitalRead(2);
  if (state) {
    
  }

}

void spdRise() {
  spdCounter ++;
  Serial.println("SPD ++");
}

void setup()
{
  Serial.begin(9600);
  Serial3.begin(9600);
  Serial3.println("AT+NAMECarMon");

  for(signed int i = 0; i < sizeof(sensors)/sizeof(sensor); i++) {
    switch (sensors[i].type) {
        case 6:
        Serial.println("Attach INJ");
          attachInterrupt(sensors[i].port, injStateChange, CHANGE);
          break;
        case 7:
        Serial.println("Attach SPD");
          attachInterrupt(sensors[i].port, spdRise, RISING);
          break;
    }
  }
  // analogReference(INTERNAL1V1);
  
  u8g.begin();
  u8g.setFont(u8g2_font_5x8_mf);
  u8g.setFontPosTop();
  u8g.setContrast(0);

  // u8g.setColorIndex(1);
}

void loop()
{
  
  once((delays) {1, 1000}, injStateChange);

  int displayX = 0;
  int displayY = 0;
  u8g.clearBuffer();
  
  if (millis() - lastUpdTime > 1000)
  {
    lastUpdTime = millis();

    injPerMin = injCounter * 60;
    spdPerMin = spdCounter * 60;
    injCounter = 0;
    spdCounter = 0;
  }

  for(signed int i = 0; i < sizeof(sensors)/sizeof(sensor); i++) {
      int yPlus = 32;
      float val = analogRead(sensors[i].port);
      double volt = voltVal(val);

      switch (sensors[i].type) {
        case 6:
          sensors[i].value = injPerMin;
          sensors[i].rawValue = injPerMin;
          break;
        case 7:
          sensors[i].value = spdPerMin;
          sensors[i].rawValue = spdPerMin;
          break;
        case 3:
          sensors[i].value = pressVal(volt);
          sensors[i].rawValue = volt;
          break;
        case 4:
          sensors[i].value = temperature;
          sensors[i].rawValue = temperature;
          break;
        case 5:
          sensors[i].value = getEGT(voltVal(val) * 1000);
          sensors[i].rawValue = volt;
          break;
        case 1: //GM Map
          sensors[i].value = mapVal(volt) / 1000;
          sensors[i].rawValue = volt;
          break;
        case 2: // IAT
          float res = resVal(volt, sensors[i].resistanceRef);
          sensors[i].value = getIat(res);
          sensors[i].rawValue = res;
      
          break;
      }

      char tmp_string[128];
      dtostrf(sensors[i].value, 2, sensors[i].decimals, tmp_string);  
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

      Serial3.print(sensors[i].serialMark);
      Serial3.print("/");
      Serial3.print(sensors[i].rawValue);
      Serial3.print("/");
      Serial3.print(sensors[i].value);
      Serial3.println("END");

      if ((displayY + yPlus) >= 64) {
        displayY = 0;
        displayX += 94;
      } else {
        displayY += yPlus;
      }
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