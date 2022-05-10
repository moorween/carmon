#include "utils.h"
#include "sensorsDefinitions.h"

const float voltRef = 2.56;

int prevIatIdx;
int prevIdx;
float pRef = 0;

float pressVal(float value) {
  return (((value / voltRef) - 0.1) / 0.8) * 10;
}

float mapVal(float value)
{
  const float Umin = 0.4;
  const float Umax = 4.65;
  const float Pmin = 200;
  const float Pmax = 3000;

  float p = (Pmax - Pmin) * (value - Umin) / (Umax - Umin) + Umin;
  // if (pRef == 0)
  // {
  //   EEPROM.get(0, pRef);
  //   if (pRef == 0) {
  //     pRef = p;
  //   }
  //   EEPROM.put(0, p);
  //   Serial.print("Map Correction: ");
  //   Serial.println(pRef);
  //   Serial.print("EEPROM write: ");
  //   Serial.println(p);
  // }

  // if (millis() > 10000) {
  //   EEPROM.put(0, 0);
  // }

  return p - pRef;
}

float resVal(float value, int resRef)
{
  return (value * resRef) / (voltRef - value);
}

float voltVal(float value)
{
  return (value / 1024.0) * voltRef;
}

float getIat(float ohm)
{
  if (ohm > iatArr[0][1] && ohm < iatArr[17][1])
  {
    float unit;
    for (int i = prevIatIdx; i < 18; i++)
    {
      if (iatArr[i][1] > ohm)
      {
        prevIatIdx = i;
        unit = (iatArr[i][1] - iatArr[i - 1][1]) / 10;
        return iatArr[i][0] + (iatArr[i][1] - ohm) / unit;
      }
    }
    prevIatIdx = 0;
    return getIat(ohm);
  } else {
    return 999;
  }
}

float getEGT(float val)
{
  if (val > egtArr[0][1] && val < egtArr[142][1])
  {
    float unit;
    for (int i = prevIdx; i < 143; i++)
    {
      if (egtArr[i][1] > val)
      {
        prevIdx = i;
        unit = (egtArr[i][1] - egtArr[i + 1][1]) / 10;
        return egtArr[i][0] + (egtArr[i][1] - val) / unit;
      }
    }
    prevIdx = 0;
    return getEGT(val);
  } else {
    return 999;
  }
}

double fuelRate(long injTotal) {
  return (((((double)injTotal * 6) / 1000) / 60) * 390) / 1000;
}