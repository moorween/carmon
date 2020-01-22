#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include "../lib/libs/utils.h"
#include "../lib/libs/temp.h"
#include "../lib/libs/blink.h"
#include "../lib/libs/async.h"
#include "../lib/GyverButton/GyverButton.h"
#include "../lib/BMP280/Adafruit_BMP280.h"
#include <U8g2lib.h>
#include <stdlib.h>
 
#define OLED_CS 45    // Pin 10, CS - Chip select
#define OLED_DC 48    // Pin 9 - DC digital signal
#define OLED_RESET 49 // using hardware !RESET from Arduino instead

U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g(U8G2_R2, OLED_CS, OLED_DC, OLED_RESET);
GButton butt1(11, LOW_PULL); //D11, D13
GButton butt2(13, LOW_PULL);
Blinker blinker;
Adafruit_BMP280 bmp280;
Async async;

int contrast = 0, displayWidth = 0;
long injDuty = 0, injTotal = 0, spdCounter = 0;
long injStartTime = 0;
long injPerSec = 0, spdPerMin = 0;
double distance = 0, injCounter = 0;
bool injLastState = false;
double totalFuel = 0;
int temperature = 0;

int pageStart[5] = {0}, pageEnd[5] = {0};
int pageIndex = 0;

int displayMode = 0;

bool experimental = false;

struct sensorsServiceData {
  double rawValue = 0;
  double altValue = 0;
  double lowValue = 0;
  double highValue = 0;
  double correction = 0;
  int warningCount = 0;
  bool displayed = false;
};

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
  sensorsServiceData serviceData;
};

sensor sensors[] = {
  {A15, 2, 2, "IAT", "IAT", 20.3, 0, 0, 0, false, 1000, 0},
  {40, 4, 3, "CTEMP", "C Temp", 1, 0, 0, 100, false, 0, 0},
  {A13, 1, 1, "BOOST", "Boost", 0.32, 2, 0, 1.2, true, 0, 5},
  {A9, 3, 3, "FPRESS", "Fuel P", 4.1, 1, 1.7, 5, false, 0, 10},
  // {A6, 3, 3, "ATPRESS", "AT Press", 4.1, 1, 0, 0, false, 0, 0},
  // {A6, 3, 3, "ATTEMP", "AT Temp", 42.3, 0, 0, 0, false, 0, 0},
  {0, 11, 3, "L100", "l/100km", 1, 1, 0, 0, false, 0, 0},
  {0, 7, 1, "INJ", "Inj", 10, 0, 0, 0, false, 0, 0},
  {1, 6, 2, "SPD", "Spd", 10, 0, 0, 0, false, 0, 0},
  {0, 8, 3, "DTY", "Duty", 1, 0, 0, 0, false, 0, 0},
  {0, 9, 3, "DST", "Dist", 1, 2, 0, 0, false, 0, 0},
  {0, 10, 3, "FUE", "Fuel", 1, 2, 0, 0, false, 0, 0},
  
  {A11, 5, 1, "EGT", "EGT", 343, 0, 0, 0, false, 0, 0},
};

const int SENSORS_SIZE = sizeof(sensors)/sizeof(sensor);
double avgValues[SENSORS_SIZE][10];
int avgIndex[SENSORS_SIZE];
// форсунка 6
// датчик скорости  7

void injStateChange() {
  bool state = digitalRead(2);

  if (state) {
    injCounter ++;
    if (injStartTime) {
      injDuty = micros() - injStartTime;
      injTotal += injDuty / 1000;
    }
  } else {
    injStartTime = micros();
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
 if (!bmp280.begin()) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  EEPROM.get(100, distance);
  EEPROM.get(200, injTotal);

  int pageIndex = 0, slots = 6;
  for(unsigned int i = 0; i < SENSORS_SIZE; i++) {
    switch (sensors[i].type) {
        case 7:
          Serial.println("Attach INJ");
          attachInterrupt(sensors[i].port, injStateChange, CHANGE);
          break;
        case 6:
          Serial.println("Attach SPD");
          attachInterrupt(sensors[i].port, spdRise, RISING);
          break;
        case 1:
          // float val = analogRead(sensors[i].port);
          // async.delay(500, [](double del, paramsData params) {
          //   int i = params.key;
          //   if (injCounter == 0) {
          //     double volt = voltVal(params.value);
          //     double correction = mapVal(volt);
          //     sensors[i].serviceData.correction = correction;
          //     EEPROM.put(0, correction);
          //     Serial.println("MAP CORRECTION ONLINE");
          //     Serial.println(sensors[i].serviceData.correction);
          //   } else {
          //     EEPROM.get(0, sensors[i].serviceData.correction);
          //     Serial.println("MAP CORRECTION EEPROM");
          //     Serial.println(sensors[i].serviceData.correction);
          //   }
          // }, {i, val});
          async.repeat(900000, [](double del, paramsData params) { // one in 15 minutes
            int i = params.key;
            double correction = bmp280.readPressure() / 133.322;
            sensors[i].serviceData.correction = correction;
            Serial.print("MAP correction: ");
            Serial.println(correction);
          }, {i, 0});
          
          break;
    }

    if (sensors[i].large) {
      slots --;
    } else {
      pageEnd[pageIndex] ++;
    }
    slots --;

    if (slots < 1 && i < SENSORS_SIZE - 1) {
      pageIndex ++;
      pageStart[pageIndex] = i + 1;
      pageEnd[pageIndex] = i;
      slots = 6;
    }
  }
  pageEnd[pageIndex] = SENSORS_SIZE - 1;

  blinker.blink();
  u8g.begin();
  u8g.setFont(u8g2_font_5x8_mf);
  u8g.setFontPosTop();
  u8g.setContrast(0);

  displayWidth = u8g.getDisplayWidth();

  async.repeat(5000, [](double del, paramsData p) {
   if (spdCounter > 0) {
      EEPROM.put(100, distance);
    }
    if (injCounter > 0) {
      EEPROM.put(200, injTotal);
    }
  });
}

void loop()
{
  if (butt1.isSingle()) {
    pageIndex = (pageStart[pageIndex + 1] > 0) ? pageIndex + 1 : 0;  
  }

  if (butt1.isHolded()) {
    displayMode = displayMode < 2 ? displayMode + 1 : 0;
    Serial.println(displayMode);
  }

  if (butt2.isClick()) {
    blinker.blink(3, YELLOW);
    experimental = !experimental;
  }

  if (butt2.isHolded()) {
    distance = 0;
    injTotal = 0;
  }

  once(500, [](double interval)  {  
    double dst = ((double)spdCounter / 200) * 2.100;
    distance += dst / 1000;
    injPerSec = injCounter * (1000 / interval);
    spdPerMin = dst * (60000 / interval); //1500 = 60km/h
    //  Serial.print(interval);
    // Serial.print("   ");
    // Serial.print(injPerSec);
    // Serial.print("   ");
    // Serial.print((1000 / interval));
    // Serial.print("   ");
    // Serial.println(injCounter);
    injCounter = 0;
    spdCounter = 0;
   
    temperature = detectTemperature();
  });

  once(200, [](double time) {
    for(unsigned int i = 0; i < SENSORS_SIZE; i++) {
        float val = analogRead(sensors[i].port);
        double volt = voltVal(val);

        if (sensors[i].averageSize > 0) {
          
          if (experimental) {
            double avgRaw = 0;
            for (int b = 0; b < sensors[i].averageSize; b ++) {
              avgRaw += analogRead(sensors[i].port);
            } 

            volt = voltVal(avgRaw / sensors[i].averageSize);

            once(10000, [](double time) {
              blinker.blink(1, YELLOW, 1);
            });
          }
          
          avgValues[i][avgIndex[i]] = volt;

          if (avgIndex[i] <= sensors[i].averageSize) {
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
          if (sensors[i].type == 3) {
            Serial.println(avgValue);
            Serial.println(avgSize);
          }
          volt = avgValue / avgSize;
        }

        switch (sensors[i].type) {
          case 7:
            sensors[i].value = injPerSec * 2 * 60;
            sensors[i].serviceData.rawValue = injPerSec;
            sensors[i].serviceData.altValue = injDuty;
            break;
          case 6:
            sensors[i].value = spdPerMin * 0.06;
            sensors[i].serviceData.rawValue = spdPerMin;
            break;
          case 8:
            sensors[i].value = injDuty;
            sensors[i].serviceData.rawValue = injDuty;
            break;
          case 9:
            sensors[i].value = distance;
            sensors[i].serviceData.rawValue = distance;
            break;
          case 10:
            totalFuel = fuelRate(injTotal);
          
            sensors[i].value = totalFuel;
            sensors[i].serviceData.rawValue = injTotal;
            break;
          case 11:
            sensors[i].value = (100 / distance) * totalFuel; 
            sensors[i].serviceData.rawValue = 0;
            break;
          case 3:
            sensors[i].value = pressVal(volt);
            sensors[i].serviceData.rawValue = volt;
            break;
          case 4:
            sensors[i].value = temperature;
            sensors[i].serviceData.rawValue = temperature;
            break;
          case 5:
            sensors[i].value = getEGT(voltVal(val) * 1000);
            sensors[i].serviceData.rawValue = volt;
            break;
          case 1: //GM Map
          // Serial.println("MAP");
          // Serial.println(mapVal(volt));
          // Serial.println(sensors[i].serviceData.correction);
            sensors[i].value = (mapVal(volt) - sensors[i].serviceData.correction) / 1000;
            sensors[i].serviceData.rawValue = volt;
            break;
          case 2: // IAT
            float res = resVal(volt, sensors[i].resistanceRef);
            sensors[i].value = getIat(res);
            sensors[i].serviceData.rawValue = res;
            break;
        }

        if (sensors[i].serviceData.lowValue == 0 || 
          sensors[i].serviceData.lowValue > sensors[i].value) {
          sensors[i].serviceData.lowValue = sensors[i].value;
        }
        if (sensors[i].serviceData.highValue == 0 || 
          sensors[i].serviceData.highValue < sensors[i].value) {
          sensors[i].serviceData.highValue = sensors[i].value;
        }

        bool warning = false;
        if (sensors[i].maxValue != 0 && sensors[i].value > sensors[i].maxValue) {
          warning = true;
        }
        if (sensors[i].minValue != 0 && sensors[i].value < sensors[i].minValue) {
          warning = true;
        }

        if (warning) {
          sensors[i].serviceData.warningCount = 3;
          // blinker.blink(3, MAROON);
        }

        switch (displayMode) {
          case 1:
            sensors[i].value = sensors[i].serviceData.highValue;
            break;
          case 2:
            sensors[i].value = sensors[i].serviceData.lowValue;
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

  once(300, [](double interval) {
    int displayX = 0;
    int displayY = 0;

    // u8g.clearBuffer();
    u8g.firstPage();
    do {
      for(unsigned int i = pageStart[pageIndex]; (i <= pageEnd[pageIndex] && i < SENSORS_SIZE); i++) {
          int yPlus = 32;

          char tmp_string[256];
          dtostrf(sensors[i].value, 2, sensors[i].decimals, tmp_string);  
          if (sensors[i].value < 0) {
            if (tmp_string[1] == '0') {
              tmp_string[1] = "";
            }
          }

          bool drawTitle = true;
          if (sensors[i].serviceData.warningCount > 0) {
            if (sensors[i].serviceData.displayed) {
              drawTitle = false;
              sensors[i].serviceData.warningCount --;
            }
          }
          sensors[i].serviceData.displayed = drawTitle;
           
          if (sensors[i].large) {
            u8g.setFont(u8g2_font_ncenR12_tf);
            drawTitle && u8g.drawStr(displayX + 10, displayY, sensors[i].title);
            u8g.setFont(u8g2_font_osr35_tn); //u8g2_font_fub20_tn
            u8g.drawStr(displayX - 15, displayY + 20, tmp_string);
            yPlus = 64;
          } else {
            if (displayX > 180) {
              u8g.setFont(u8g2_font_5x8_mf);
              drawTitle && u8g.drawStr(displayWidth - u8g.getStrWidth(sensors[i].title), displayY, sensors[i].title);
              u8g.setFont(u8g2_font_helvR18_tn); //u8g2_font_fub20_tn
              u8g.drawStr(displayWidth - u8g.getStrWidth(tmp_string), displayY + 8, tmp_string);
            } else {
              u8g.setFont(u8g2_font_5x8_mf);
              drawTitle && u8g.drawStr(displayX, displayY, sensors[i].title);
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
      u8g.drawLine(75, 5, 75, 59);
      u8g.drawLine(181, 5, 181, 59);

      switch (displayMode) {
        case 1:
          u8g.drawLine(181, 5, 174, 13);
          u8g.drawLine(181, 5, 188, 13);
          u8g.drawLine(75, 5, 68, 13);
          u8g.drawLine(75, 5, 82, 13);
          break;
        case 2:
          u8g.drawLine(181, 59, 174, 51);
          u8g.drawLine(181, 59, 188, 51);
          u8g.drawLine(75, 59, 68, 51);
          u8g.drawLine(75, 59, 82, 51);
          break;
      }
      
    } while ( u8g.nextPage() );
   
    // u8g.drawLine(65, 5, 65, 59);
    // u8g.drawLine(191, 5, 191, 59);
    // u8g.sendBuffer();
  });

  async.tick();
  butt1.tick();
  butt2.tick();
  blinker.tick();
}