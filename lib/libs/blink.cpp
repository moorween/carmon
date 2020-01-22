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
  int brightnes;
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
      int divider = 2;
      led.setColor(0, it->first);
      led.setBrightness((it->second.iteration * divider) / 10 * it->second.brightnes);
      led.show();
      
      if (it->second.direction) {
        if (it->second.iteration < (255 / divider)) {
          it->second.iteration ++;
        } else {
          it->second.direction = false;
        }
      } else {
        if (it->second.iteration > 0) {
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

void Blinker::blink(int count, COLORS color, int brightnes = 10) {
  queue[color].count = count;
  queue[color].brightnes = brightnes;
}

// void Blinker::blink(int count, COLORS color) {
//   queue[color].count = count;
// }

void Blinker::blink() {
    Blinker::blink(1, ORANGE);
}