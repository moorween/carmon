#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include "../lib/utils.cpp"
#include <U8g2lib.h>
#include <stdlib.h>
#include <OneWire.h>
#include <ArduinoSTL.h>
#include <map>


#define OLED_CS 45    // Pin 10, CS - Chip select
#define OLED_DC 48    // Pin 9 - DC digital signal
#define OLED_RESET 49 // using hardware !RESET from Arduino instead

U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g(U8G2_R2, OLED_CS, OLED_DC, OLED_RESET);
OneWire ds(40);

int contrast = 0, displayWidth = 0;
int temperature = 0; // Глобальная переменная для хранения значение температуры с датчика DS18B20

long injDuty = 0, injCounter = 0, spdCounter = 0;
long injStartTime = 0;
long injPerMin = 0, spdPerMin = 0;
bool injLastState = false;

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
  int averageSize; // 10 is MAX
  double rawValue;
  double altValue;
};

sensor sensors[] = {
  {0, 7, 1, "INJ", "Inj", 10, 0, 0, 0, false, 0, 5, 0, 0},
  {1, 6, 2, "SPD", "Spd", 10, 0, 0, 0, false, 0, 5, 0, 0},
  // {A15, 2, 2, "IAT", "IAT", 20.3, 0, 0, 0, false, 1000, 0, 0, 0},
  // {A11, 5, 1, "EGT", "EGT", 343, 0, 0, 0, false, 0, 0, 0, 0},
  {A13, 1, 1, "BOOST", "Boost", 0.32, 2, 0, 0, true, 0, 10, 0, 0},
  {A9, 3, 3, "FPRESS", "Fuel P", 4.1, 1, 0, 0, false, 0, 5, 0, 0},
  // {A6, 3, 3, "ATPRESS", "AT Press", 4.1, 1, 0, 0, false, 0, 0},
  // {A6, 3, 3, "ATTEMP", "AT Temp", 42.3, 0, 0, 0, false, 0, 0},
  // {40, 4, 3, "CTEMP", "C Temp", 1, 0, 0, 0, false, 0, 0, 0, 0},
  {0, 8, 3, "DTY", "Duty", 1, 0, 0, 0, false, 0, 0, 0, 0},
};

const int SENSORS_SIZE = sizeof(sensors)/sizeof(sensor);
double avgValues[SENSORS_SIZE][10];
int avgIndex[SENSORS_SIZE];
// форсунка 6
// датчик скорости  7

int detectTemperature() {
  byte data[2];
  ds.reset();
  ds.write(0xCC);
  ds.write(0xBE);
  data[0] = ds.read();
  data[1] = ds.read();

  // Формируем значение
  temperature = (data[1] << 8) + data[0]; temperature = temperature >> 4;
  ds.reset();
  ds.write(0xCC);
  ds.write(0x44);
  return temperature;
}

std::map <int, int> timings;
void once(int delay, void (*callback)(void)) {
  if (millis() - timings[delay] >= delay)
  {
    timings[delay] = millis();
    callback();
  }
}

void injStateChange() {
  bool state = digitalRead(2);

  if (state) {
    injCounter ++;
    if (injStartTime) {
      injDuty = millis() - injStartTime;
    }
  } else {
    injStartTime = millis();
  }
}

void spdRise() {
  spdCounter ++;
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

  u8g.begin();
  u8g.setFont(u8g2_font_5x8_mf);
  u8g.setFontPosTop();
  u8g.setContrast(0);

  displayWidth = u8g.getDisplayWidth();
}

void loop()
{
  once(1000, []()  {  
    injPerMin = injCounter * 60;
    spdPerMin = spdCounter * 60;
    injCounter = 0;
    spdCounter = 0;

    detectTemperature();
  });

  once(100, []() {
    for(unsigned int i = 0; i < sizeof(sensors)/sizeof(sensor); i++) {
        float val = analogRead(sensors[i].port);
        double volt = voltVal(val);

        if (sensors[i].averageSize > 0) {
          avgValues[i][avgIndex[i]] = volt;

          if (avgIndex[i] < sensors[i].averageSize) {
            avgIndex[i] ++;
          } else {
            avgIndex[i] = 0;
          }

          double avgValue = 0;
          int avgSize = 0;
          for (int a = 0; a < sensors[i].averageSize; a ++) {
            if (avgValues[i][a] != 0) {
              avgValue += avgValues[i][a];
              avgSize ++;
            }
          }

          volt = avgValue / avgSize;
        }

        switch (sensors[i].type) {
          case 6:
            sensors[i].value = injPerMin;
            sensors[i].rawValue = injPerMin;
            sensors[i].altValue = injDuty;
            break;
          case 7:
            sensors[i].value = spdPerMin;
            sensors[i].rawValue = spdPerMin;
            break;
          case 8:
            sensors[i].value = injDuty;
            sensors[i].rawValue = injDuty;
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

        // Serial3.print(sensors[i].serialMark);
        // Serial3.print("/");
        // Serial3.print(sensors[i].rawValue);
        // Serial3.print("/");
        // Serial3.print(sensors[i].value);
        // Serial3.println("END");
    }
  });

  once(300, []() {
    int displayX = 0;
    int displayY = 0;
    long start = millis();

    // u8g.clearBuffer();
    u8g.firstPage();
    do {
      for(unsigned int i = 0; i < sizeof(sensors)/sizeof(sensor); i++) {
          int yPlus = 32;

          char tmp_string[256];
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
              u8g.drawStr(displayWidth - u8g.getStrWidth(sensors[i].title), displayY, sensors[i].title);
              u8g.setFont(u8g2_font_helvR18_tn); //u8g2_font_fub20_tn
              u8g.drawStr(displayWidth - u8g.getStrWidth(tmp_string), displayY + 8, tmp_string);
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
          
          if (contrast < 255) {
            contrast += 15;
            u8g.setContrast(contrast);
          }
      }
      u8g.drawLine(65, 5, 65, 59);
      u8g.drawLine(191, 5, 191, 59);
    } while ( u8g.nextPage() );
   
    // u8g.drawLine(65, 5, 65, 59);
    // u8g.drawLine(191, 5, 191, 59);
    // u8g.sendBuffer();
    Serial.println(millis() - start);
  });
}