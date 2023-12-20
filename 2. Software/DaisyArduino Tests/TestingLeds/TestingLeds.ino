// A basic everyday NeoPixel strip test program.

// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include "DaisyDuino.h"
#include <U8g2lib.h>


#include <Adafruit_NeoPixel.h>
#include "HC165.h"

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN 5

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 30
DaisyHardware hw;
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
static ShiftRegister165 shr;
long readValue = 100;

int numDevices = 1;

U8G2_SSD1309_128X64_NONAME2_F_4W_SW_SPI oled(U8G2_R0, /* clock=*/8,
                                             /* data=*/10, /* cs=*/7, /* dc=*/9,
                                             /* reset=*/30);

// setup() function -- runs once at startup --------------------------------

void setup() {
hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);

  // strip.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  // strip.show();             // Turn OFF all pixels ASAP
  // strip.setBrightness(20);  // Set BRIGHTNESS to about 1/5 (max = 255)

  uint32_t pin_cfg[] = { 0, 1, 2, 3 };
  shr.Init(pin_cfg, numDevices);
  oled.setFont(u8g2_font_inb16_mf);
  oled.setFontDirection(0);
  oled.setFontMode(1);
  oled.begin();
}


// loop() function -- runs repeatedly as long as board is on ---------------
char* getBin(long input) {
  char str[numDevices * 8 + 1];
  str[numDevices * 8] = '\0';
  int i = numDevices * 8;
  while (i) {
    i--;
    str[i] = (input & 1) + '0';
    input >>= 1;
  }
  return str;
}

void loop() {

  // strip.clear();
  // readValue = shr.Read();
  // int i = 0;
  // for (i = 0; i < 8; i++) {
  //   if (bitRead(readValue, i) == 1) {
  //     strip.setPixelColor(i, strip.Color(255, 0, 0));
  //   } else {
  //     strip.setPixelColor(i, strip.Color(0, 255, 0));
  //   }
  // }
  // strip.show();
  oled.clearBuffer();
  oled.drawStr(0, 0, getBin(readValue));
  oled.sendBuffer();
}
