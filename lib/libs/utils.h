#include "EEPROM.h"
#include <Arduino.h>
float pressVal(float value);
float mapVal(float value);
float resVal(float value, int resRef);
float voltVal(float value);
float getIat(float ohm);
float getEGT(float val);
double fuelRate(long injTotal);