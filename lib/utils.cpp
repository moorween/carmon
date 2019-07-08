#include "../../EEPROM/src/EEPROM.h"
const float voltRef = 1.1;

float iatCal[18][2] = {
    130, 89.3,
    120, 112.7,
    110, 144.2,
    100, 186.6,
    90, 243.2,
    80, 322.5,
    70, 435.7,
    60, 595.5,
    50, 834,
    40, 1175,
    30, 1707,
    20, 2500,
    10, 3792,
    0, 5896,
    -10, 9397,
    -20, 15462,
    -30, 26114,
    -40, 45313};

float dataArr[143][2] = {
    -50, -1.889,
    -40, -1.527,
    -30, -1.156,
    -20, -0.777,
    -10, -0.392,
    0, 0,
    10, 0.397,
    20, 0.798,
    30, 1.203,
    40, 1.611,
    50, 2.022,
    60, 2.436,
    70, 2.85,
    80, 3.266,
    90, 3.681,
    100, 4.095,
    110, 4.508,
    120, 4.919,
    130, 5.327,
    140, 5.733,
    150, 6.137,
    160, 6.539,
    170, 6.939,
    180, 7.338,
    190, 7.737,
    200, 8.137,
    210, 8.537,
    220, 8.938,
    230, 9.341,
    240, 9.745,
    250, 10.151,
    260, 10.56,
    270, 10.969,
    280, 11.381,
    290, 11.793,
    300, 12.207,
    310, 12.623,
    320, 13.039,
    330, 13.456,
    340, 13.874,
    350, 14.292,
    360, 14.712,
    370, 15.132,
    380, 15.552,
    390, 15.974,
    400, 16.395,
    410, 16.818,
    420, 17.241,
    430, 17.664,
    440, 18.088,
    450, 18.513,
    460, 18.938,
    470, 19.363,
    480, 19.788,
    490, 20.214,
    500, 20.64,
    510, 21.066,
    520, 21.493,
    530, 21.919,
    540, 22.346,
    550, 22.772,
    560, 23.198,
    570, 23.624,
    580, 24.05,
    590, 24.476,
    600, 24.902,
    610, 25.327,
    620, 25.751,
    630, 26.176,
    640, 26.599,
    650, 27.022,
    660, 27.445,
    670, 27.867,
    680, 28.288,
    690, 28.709,
    700, 29.128,
    710, 29.547,
    720, 29.965,
    730, 30.383,
    740, 30.799,
    750, 31.214,
    760, 31.629,
    770, 32.042,
    780, 32.455,
    790, 32.866,
    800, 33.277,
    810, 33.686,
    820, 34.095,
    830, 34.502,
    840, 34.909,
    850, 35.314,
    860, 35.718,
    870, 36.121,
    880, 36.524,
    890, 36.925,
    900, 37.325,
    910, 37.724,
    920, 38.122,
    930, 38.519,
    940, 38.915,
    950, 39.31,
    960, 39.703,
    970, 40.096,
    980, 40.488,
    990, 40.879,
    1000, 41.269,
    1010, 41.657,
    1020, 42.045,
    1030, 42.432,
    1040, 42.817,
    1050, 43.202,
    1060, 43.585,
    1070, 43.968,
    1080, 44.349,
    1090, 44.729,
    1100, 45.108,
    1110, 45.486,
    1120, 45.863,
    1130, 46.238,
    1140, 46.612,
    1150, 46.985,
    1160, 47.356,
    1170, 47.726,
    1180, 48.095,
    1190, 48.462,
    1200, 48.828,
    1210, 49.192,
    1220, 49.555,
    1230, 49.916,
    1240, 50.276,
    1250, 50.633,
    1260, 50.99,
    1270, 51.344,
    1280, 51.697,
    1290, 52.049,
    1300, 52.398,
    1310, 52.747,
    1320, 53.093,
    1330, 53.439,
    1340, 53.782,
    1350, 54.125,
    1360, 54.466,
    1370, 54.807
};

int prevIatIdx;
int prevIdx;
float pRef = 0;

float pressVal(float value) {
  return ((value / voltRef) - 0.1) * 10;
}

float mapVal(float value)
{
  const float Umin = 0.4;
  const float Umax = 4.65;
  const float Pmin = 200;
  const float Pmax = 3000;

  float p = (Pmax - Pmin) * (value - Umin) / (Umax - Umin) + Umin;
  if (pRef == 0)
  {
    pRef = EEPROM.read(0) || p;
    EEPROM.write(0, p);
  }
  if (millis() > 10000) {
    EEPROM.write(0, 0);
  }

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
  if (ohm > iatCal[0][1] && ohm < iatCal[17][1])
  {
    float unit;
    for (int i = prevIatIdx; i < 18; i++)
    {
      if (iatCal[i][1] > ohm)
      {
        prevIatIdx = i;
        unit = (iatCal[i][1] - iatCal[i - 1][1]) / 10;
        return iatCal[i][0] + (iatCal[i][1] - ohm) / unit;
      }
    }
    prevIatIdx = 0;
    return getIat(ohm);
  } else {
    return 0;
  }
}

float getEGT(float val)
{
  if (val > dataArr[0][1] && val < dataArr[142][1])
  {
    float unit;
    for (int i = prevIdx; i < 143; i++)
    {
      if (dataArr[i][1] > val)
      {
        prevIdx = i;
        unit = (dataArr[i][1] - dataArr[i + 1][1]) / 10;
        return dataArr[i][0] + (dataArr[i][1] - val) / unit;
      }
    }
    prevIdx = 0;
    return getEGT(val);
  } else {
    return 0;
  }
}