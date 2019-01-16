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
  -40, 45313
};

int prevIatIdx;
float pRef = 0;

float mapVal(float value){
  const float Umin = 0.4;
  const float Umax = 4.65;
  const float Pmin = 200;
  const float Pmax = 3000;

  float p = (Pmax - Pmin) * (value - Umin) / (Umax - Umin) + Umin;
  if (pRef == 0) {
    pRef = p;
  }
  return p - pRef;
}

float resVal(float value, int resRef) {
  return (value * resRef) / (5 - value);
}

float voltVal(float value) {
  return (value / 1024.0)  * 5.0;
}

float getIat(float ohm) {
  float unit;
  for (int i = prevIatIdx; i < 18; i++) {
    if (iatCal[i][1] > ohm) {
      prevIatIdx = i;
      unit = (iatCal[i][1] - iatCal[i - 1][1]) / 10;
      return iatCal[i][0] + (iatCal[i][1] - ohm) / unit;
    }
  }
  prevIatIdx = 0;
  return getIat(ohm);
}
