#include "temp.h"

OneWire ds(40);

int detectTemperature() {
  byte data[2];
  ds.reset();
  ds.write(0xCC);
  ds.write(0xBE);
  data[0] = ds.read();
  data[1] = ds.read();

  // Формируем значение
  int temperature = (data[1] << 8) + data[0]; 
  temperature = temperature >> 4;
  ds.reset();
  ds.write(0xCC);
  ds.write(0x44);
  return temperature;
}