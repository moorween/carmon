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
#include "TimerOne.h"

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

double distanceNew = 0, fuelNew = 0;
long injNew = 0;

bool injLastState = false;
double totalFuel = 0;
int temperature = 0;

int pageStart[5] = {0}, pageEnd[5] = {0};
int pageIndex = 0;

int displayMode = 0;

bool engineRun = false;
bool experimental = false;

long fuelFlow[40];
int flowPos = 0;

int startReady = 0;
long startTime = 0;
double time_60 = 0, time_100 = 0;

struct sensorsServiceData {
  char charValue;
  double rawValue = 0;
  double altValue = 0;
  double lowValue = 0;
  double highValue = 0;
  double correction = 0;
  int warningCount = 0;
  long warningTime = 0;
  bool displayed = false;
  bool warning = false;
  bool panic = false;  
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
  int panicDelay;
  boolean large;
  int resistanceRef;
  int averageSize; // 10 is MAX
  int refTo;
  sensorsServiceData serviceData;
};

sensor sensors[] = {
  // {A15, 2, 2, "IAT", "IAT", 20.3, 0, 0, 0, 0, false, 1000, 0},
  {A9, 3, 3, "FPRESS", "Fuel P", 4.1, 1, 2.8, 3.5, 1000, false, 0, 10, 2},
  {A5, 15, 1, "VOLT", "Volt", 12.0, 1, 12, 14.7, 1000, false, 0, 5, 0},
  {A13, 1, 1, "BOOST", "Boost", 0.32, 2, 0, 1.2, 1000, true, 0, 5},
  {0, 12, 3, "L100N", "l/100km", 1, 1, 0, 0, 0, false, 0, 0},
  {40, 4, 3, "CTEMP", "C Temp", 1, 0, 0, 0, 10000, false, 0, 0},
  {0, 13, 3, "L5N", "l/100km(5km)", 1, 1, 0, 0, 0, false, 0, 0},
  // {A6, 3, 3, "ATPRESS", "AT Press", 4.1, 1, 0, 0, false, 0, 0},
  // {A6, 3, 3, "ATTEMP", "AT Temp", 42.3, 0, 0, 0, false, 0, 0},
  {0, 7, 1, "INJ", "Inj", 10, 0, 0, 0, 0, false, 0, 0},
  {1, 6, 2, "SPD", "Spd", 10, 0, 0, 0, 0, false, 0, 0},
  {0, 8, 3, "DTY", "Duty", 1, 0, 0, 0, 0, false, 0, 0},
  {0, 9, 3, "DST", "Dist", 1, 2, 0, 0, 0, false, 0, 0},
  {0, 10, 3, "FUE", "Fuel", 1, 2, 0, 0, 0, false, 0, 0},
  {0, 11, 3, "L100", "l/100km", 1, 1, 0, 0, 0, false, 0, 0},
  // {A11, 5, 1, "EGT", "EGT", 343, 0, 0, 0, 0, false, 0, 0},

  {0, 40, 3, "0_60", "0-60", 1, 2, 0, 0, 0, false, 0, 0},
  {0, 41, 3, "0_100", "0-100", 1, 2, 0, 0, 0, false, 0, 0},
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
    engineRun = true;
    if (injStartTime) {
      injDuty = micros() - injStartTime;
      injTotal += injDuty / 1000;
      injNew += injDuty / 1000;
    }
  } else {
    injStartTime = micros();
  }
}

void spdRise() {
  if (startReady >= 5) {
    startTime = millis();
    time_60 = 0;
    time_100 = 0;
    startReady = 0;
  }
  spdCounter ++;
}

void timerIsr() {
  butt1.tick();
  butt2.tick();
}

void readFuelFlow() {
  EEPROM.get(512, injNew); // 4 bytes
  EEPROM.get(516, distanceNew); // 4 bytes
  EEPROM.get(520, flowPos); // 2 bytes
  EEPROM.get(522, fuelFlow);
}

void setup()
{
  Serial.begin(9600);
  Serial3.begin(115000);
  Serial3.println("AT+NAMECarMon");
  Timer1.initialize(10000);           // установка таймера на каждые 10000 микросекунд (== 10 мс)
  Timer1.attachInterrupt(timerIsr);

  if (!bmp280.begin()) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  readFuelFlow();
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
        
          async.delay(500, [](double del, paramsData params) {
            int i = params.key;
          
            async.repeat(900000, [](double del, paramsData params) { // once in 15 minutes
              int i = params.key;

              double atmPress = bmp280.readPressure() / 133.322;
              double correction = 0;

              if (!engineRun) {
                double val = analogRead(sensors[i].port);
                double volt = voltVal(val);
                correction = mapVal(volt) - atmPress;

                EEPROM.put(0, correction);
                Serial.println("MAP CORRECTION ONLINE");
                Serial.println(correction);
              } else {
                EEPROM.get(0, correction);
                Serial.println("MAP CORRECTION EEPROM");
                Serial.println(correction);
              }
            
              sensors[i].serviceData.correction = atmPress + correction;
              Serial.print("MAP atmosphere pressure: ");
              Serial.println(atmPress);
            }, {i, 0});
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

  async.repeat(1000, [](double del, paramsData p) {
    if (spdCounter < 3) {
      startReady += 1;
    }
  });

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
  }

  if (butt2.isClick()) {
    blinker.blink(3, YELLOW);
    experimental = !experimental;

    // for (int i = 0; i < 15; i ++) {
    //   fuelFlow[i] = 28500;
    // }
    // distanceNew = 0.39;
    // injNew = 286;
    // flowPos = 15;
    // EEPROM.put(512, injNew); // 4 bytes
    // EEPROM.put(516, distanceNew); // 4 bytes
    // EEPROM.put(520, flowPos); // 2 bytes
    // EEPROM.put(522, fuelFlow);
  }

  if (butt2.isHolded()) { // reset counters
    if (distanceNew == 0) {
      // distance = 0;
      // injTotal = 0;
    }   
    
    distanceNew = 0;
    injNew = 0;
    flowPos = 0;
    
    EEPROM.put(512, injNew); // 4 bytes
    EEPROM.put(516, distanceNew); // 4 bytes
    EEPROM.put(520, flowPos); // 2 bytes

    memset(fuelFlow,0,sizeof(fuelFlow));
    EEPROM.put(522, fuelFlow);
  }

  once(1000, [](double interval)  {  
    temperature = detectTemperature();
  });

  once(500, [](double interval)  {  
    double dst = ((double)spdCounter / 4) * 2.100;
    distance += dst / 1000;
    injPerSec = injCounter * (1000 / interval);
    spdPerMin = dst * (60000 / interval); //1500 = 60km/h

    double spdPerHour = spdPerMin * 0.06;

    if (time_60 == 0 && spdPerHour >= 60) {
      time_60 = (millis() - startTime) / 1000;
    }

    if (time_100 == 0 && spdPerHour >= 100) {
      time_100 = (millis() - startTime) / 1000;
    }

    injCounter = 0;
    spdCounter = 0;
   
    distanceNew += dst / 1000;
    if (distanceNew >= 5) {
      flowPos = flowPos < 39 ? flowPos + 1 : 0; 

      fuelFlow[flowPos] = injNew;
      distanceNew = distanceNew - 5;

      EEPROM.put(520, flowPos); // 2 bytes
      EEPROM.put(522 + (flowPos * 4), injNew);
      injNew = 0;
    }

    if (dst > 0) {
      EEPROM.put(512, injNew); // 4 bytes
      EEPROM.put(516, distanceNew); // 4 bytes
    }
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
    
          volt = avgValue / avgSize;
        }

        switch (sensors[i].type) {
          case 3:
            {
              sensors[i].value = pressVal(volt);
              sensors[i].serviceData.rawValue = volt;
            }
            break;
          case 4:
            {
              sensors[i].value = temperature;
              sensors[i].serviceData.rawValue = temperature;
            }
            break;
          case 5:
            {
              sensors[i].value = getEGT(voltVal(val) * 1000);
              sensors[i].serviceData.rawValue = volt;
            }
            break;
          case 1: //GM Map
            {
              sensors[i].value = (mapVal(volt) - sensors[i].serviceData.correction) / 1000;
              sensors[i].serviceData.rawValue = volt;
            }
            break;
          case 2: // IAT
            {
              float res = resVal(volt, sensors[i].resistanceRef);
              sensors[i].value = getIat(res);
              sensors[i].serviceData.rawValue = res;
            }
            break;
          case 12:
            {
              double distanceSumm = distanceNew;
              long injSumm = injNew;

              for (int a = 0; a < 40; a ++) {
                if (fuelFlow[a] > 0) {
                  injSumm += fuelFlow[a];
                  distanceSumm += 5;
                }
              }

              double fuelSumm = fuelRate(injSumm);
                
              sensors[i].value = (100 / distanceSumm) * fuelSumm; 
              sensors[i].serviceData.rawValue = 0;
            }
            break;
          case 13:
            {
              double distanceSumm = distanceNew;
              long injSumm = injNew; 
              
              // 5 km
              if (fuelFlow[flowPos] > 0) { 
                  injSumm += fuelFlow[flowPos];
                  distanceSumm += 5;
              }

              double fuelSumm = fuelRate(injSumm);
                
              sensors[i].value = (100 / distanceSumm) * fuelSumm; 
              sensors[i].serviceData.rawValue = 0;
            }
            break;
          case 7:
            {
              sensors[i].value = injPerSec * 2 * 60;
              sensors[i].serviceData.rawValue = injPerSec;
              sensors[i].serviceData.altValue = injDuty;
            }
            break;
          case 6:
            {
              sensors[i].value = spdPerMin * 0.06;
              sensors[i].serviceData.rawValue = spdPerMin;
            }
            break;
          case 8:
            {
              sensors[i].value = injDuty;
              sensors[i].serviceData.rawValue = injDuty;
            }
            break;
          case 9:
            {
              sensors[i].value = distance;
              sensors[i].serviceData.rawValue = distance;
            }
            break;
          case 10:
            {
              totalFuel = fuelRate(injTotal);
          
              sensors[i].value = totalFuel;
              sensors[i].serviceData.rawValue = injTotal;
            }
            break;
          case 11:
            {
              totalFuel = fuelRate(injTotal);
              sensors[i].value = (100 / distance) * totalFuel; 
              sensors[i].serviceData.rawValue = 0;
            }
            break;
          case 15:
            {
              sensors[i].value = volt / 0.2130; // R1 = 56kOm, R2 = 14.7 kOm
              sensors[i].serviceData.rawValue = volt;
            }
            break;
          case 40:
            {
              sensors[i].value = time_60;
              sensors[i].serviceData.rawValue = 0;
            }
            break;
          case 41:
            {
              sensors[i].value = time_100;
              sensors[i].serviceData.rawValue = 0;
            }
            break;
        }

        if (engineRun) {
            if (sensors[i].serviceData.lowValue == 0 || 
              sensors[i].serviceData.lowValue > sensors[i].value) {
              sensors[i].serviceData.lowValue = sensors[i].value;
            }
            if (sensors[i].serviceData.highValue == 0 || 
              sensors[i].serviceData.highValue < sensors[i].value) {
              sensors[i].serviceData.highValue = sensors[i].value;
            }

        }
        
        bool warning = false;
        double value = sensors[i].value;

        if (sensors[i].refTo) {
          value = value - sensors[sensors[i].refTo].value;
        }

        if (sensors[i].maxValue != 0 && value > sensors[i].maxValue) {
          warning = true;
        }
        if (sensors[i].minValue != 0 && value < sensors[i].minValue) {
          warning = true;
        }

        sensors[i].serviceData.warning = warning;

        if (warning) {
          sensors[i].serviceData.warningCount = 3;
          sensors[i].serviceData.warningTime += time;
          if (engineRun) {

            if (sensors[i].panicDelay > 0 && sensors[i].serviceData.warningTime > sensors[i].panicDelay) {
              blinker.blink(3, MAROON);
            } else {
              blinker.blink(3, YELLOW);
            }
            
          }
        } else {
          sensors[i].serviceData.warningTime = 0;
        }

        switch (displayMode) {
          case 1:
            sensors[i].value = sensors[i].serviceData.highValue;
            break;
          case 2:
            sensors[i].value = sensors[i].serviceData.lowValue;
            break;
        }

        // char tmp_string[256];
        // dtostrf(sensors[i].value, 2, sensors[i].decimals, tmp_string);  
        // if (sensors[i].value < 0) {
        //   if (tmp_string[1] == '0') {
        //     tmp_string[1] = "";
        //   };
        // }

        // sensors[i].serviceData.charValue = *tmp_string;
        // Serial3.print();
        // Serial3.print("/");
        // Serial3.print(sensors[i].serviceData.rawValue);
        // Serial3.print("/");
        // Serial3.print(sensors[i].value);
        // Serial3.println(String(sensors[i].serialMark) + "/" + tmp_string + "END");
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
  blinker.tick();
}