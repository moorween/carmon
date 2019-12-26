#include "async.h"
#include <Arduino.h>

std::map <int, unsigned long> timings;

typedef void (*callback)(double, paramsData params); 

struct timingParams {
  long delay;
  long unsigned mls;
  bool once;
  paramsData params;
};

std::map <callback, timingParams> _timings;

void once(int delay, void (*callback)(double)) {
  if (millis() - timings[delay] >= delay)
  {
    callback(millis() - timings[delay]);
    timings[delay] = millis();
  }
}

Async::Async() {
}

void Async::repeat(int delay, void (*cb)(double, paramsData params), paramsData params) {
  _timings[cb] = {delay, millis(), false, params};
}

void Async::delay(int delay, void (*cb)(double, paramsData params), paramsData params) {
  _timings[cb] = {delay, millis(), true, params};
}

void Async::tick() {
  std::map<callback, timingParams>::iterator it = _timings.begin();
 
    while (it != _timings.end()) {
      if (millis() - it->second.mls >= it->second.delay) {
        it->first(millis() - it->second.mls, it->second.params);
        if (it->second.once) {
          _timings.erase(it->first);
        } else {
          it->second.mls = millis();
        }
      }
      it ++;
    }
}
