#include "blink.h"
#include "./async.h"
#include <Arduino.h>
#include "../microLED/microLED.h"

#include <ArduinoSTL.h>
#include <map>
#include <iterator>

LEDdata leds[1];
microLED led(leds, 1, 39); 

struct blinkParams
{
  int count;
  int iteration = 1;
  bool direction = true; 
};

std::map <COLORS, blinkParams> queue;

Blinker::Blinker() {
}

void Blinker::tick() {
  once(2, [](double delayed) {
    std::map<COLORS, blinkParams>::iterator it = queue.begin();
 
    while (it != queue.end()) {
      led.setColor(0, it->first);
      led.setBrightness(it->second.iteration / 2);
      led.show();
      
      if (it->second.direction) {
        if (it->second.iteration < 128) {
          it->second.iteration ++;
        } else {
          it->second.direction = false;
        }
      } else {
        if (it->second.iteration > 1) {
          it->second.iteration --;
        } else {
          it->second.direction = true;
          it->second.count --;
          if (it->second.count == 0) {
            queue.erase(it->first);
          }
        }
      }
      it ++;
    }
  });
}

void Blinker::blink(int count, COLORS color) {
  queue[color].count = count;
}

void Blinker::blink() {
    Blinker::blink(1, ORANGE);
}