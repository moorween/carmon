#include "async.h"
#include <Arduino.h>
#include <ArduinoSTL.h>
#include <map>

std::map <int, unsigned long> timings;
void once(int delay, void (*callback)(double)) {
  if (millis() - timings[delay] >= delay)
  {
    callback(millis() - timings[delay]);
    timings[delay] = millis();
  }
}