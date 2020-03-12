#ifndef Async_H
    #define Async_H
    void once(int delay, void (*callback)(double));
#endif

#include <ArduinoSTL.h>
#include <map>

struct paramsData {
    int key;
    float value;
};

class Async {
        public:
	        Async();
	        void tick();
            void delay(int delay, void (*callback)(double, paramsData params), paramsData params = {0,0});
            void delay(int delay, bool replace, void (*callback)(double, paramsData params), paramsData params = {0,0});
            void repeat(int delay, void (*callback)(double, paramsData params), paramsData params = {0,0});
  };