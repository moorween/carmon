#include <Adafruit_GFX.h>
#include <ESP8266_SSD1322.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>

#define OLED_CS 45    // Pin 10, CS - Chip select
#define OLED_DC 48    // Pin 9 - DC digital signal
#define OLED_RESET 49 // using hardware !RESET from Arduino instead

//hardware SPI - only way to go. Can get 110 FPS
ESP8266_SSD1322 displ(OLED_DC, OLED_RESET, OLED_CS);

GFXfont font;

void beginDisplay() {
    displ.begin(true);
}

void clearDisplay()
{
    displ.clearDisplay();
}

void drawText(int16_t x, int16_t y, const String &str, const GFXfont *f)
{
    font = *f;
    displ.setCursor(x, y);
    // displ.setFont(&font);
    displ.println(str);
}



void display() {
    displ.display();
}