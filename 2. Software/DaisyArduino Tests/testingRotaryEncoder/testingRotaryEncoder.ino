#include "DaisyDuino.h"
#include <U8g2lib.h>

DaisyHardware hw;
U8G2_SSD1309_128X64_NONAME2_F_4W_SW_SPI oled(U8G2_R0, /* clock=*/8,
                                             /* data=*/10, /* cs=*/7, /* dc=*/9,
                                             /* reset=*/30);

long brightness = 0;
long increment = 10;
const int pin_A = 15;  // pin 12
const int pin_B = 16;  // pin 11
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_B_prev = 0;

bool aWasLast = false;



void setup() {
  // declare pin 9 to be an output:
  hw = DAISY.init(DAISY_PATCH, AUDIO_SR_48K);
  Serial.begin(9600);

  oled.setFont(u8g2_font_micro_tr);
  oled.setFontDirection(0);
  oled.setFontMode(1);
  oled.begin();

  pinMode(pin_A, INPUT_PULLUP);
  pinMode(pin_B, INPUT_PULLUP);
}

void loop() {
  encoder_A = digitalRead(pin_A);
  encoder_B = digitalRead(pin_B);
  if (encoder_B != encoder_B_prev) {
    encoder_B_prev = encoder_B;
    if (encoder_B == 1 && encoder_A == 1 || encoder_B == 0 && encoder_A == 0) {
      brightness += increment;
    } else {
      brightness -= increment;
    }
    brightness += 360;
    brightness = brightness % 360;
    oled.clearBuffer();
    oled.drawBox(10, 10, (int)(100.0 * (float)brightness / 360.0), 20);
    oled.drawFrame(10, 10, 100, 20);
    oled.sendBuffer();
  }
}