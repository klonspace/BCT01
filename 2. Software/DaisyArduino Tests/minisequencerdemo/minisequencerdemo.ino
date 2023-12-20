// Title: Oled
// Description: Bounces a character around the screen.
// Hardware: Daisy Patch
// Author: Ben Sergentanis

#include "DaisyDuino.h"
#include <U8g2lib.h>
#include "HC165.h"


static ShiftRegister165 shr;
long readValue = 0;

DaisyHardware hw;

// the magic incantation
U8G2_SSD1309_128X64_NONAME2_F_4W_SW_SPI oled(U8G2_R0, /* clock=*/8,
                                             /* data=*/10, /* cs=*/7, /* dc=*/9,
                                             /* reset=*/30);
#define LED_PIN 5
#define LED_COUNT 30
// Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int numRegisters = 2;
char str[] = "button0";

// int buttonRefs[] = {
//   8,
//   9,
//   10,
//   11,
//   0,
//   1,
//   2,
//   3,
//   15,
//   14,
//   13,
//   12,
//   7,
//   6,
//   5,
//   4,
// };

int buttonRefs[] = {
  0,1,2,3,8,9,10,11,7,6,5,4,15,14,13,12
};

long currState = 0;

int y = 0;

size_t num_channels;

static WhiteNoise nse;

float samplerate = 48000.0;

float steplength = 0.1;

float currTime = 0.0;


void MyCallback(float **in, float **out, size_t size) {
  float sig;
  for (size_t i = 0; i < size; i++) {
    sig = nse.Process();

    for (size_t chn = 0; chn < num_channels; chn++) {
      out[chn][i] = sig * 0.05 * bitRead(currState,  (int)fmod(currTime / steplength, 16.0)) * (1 - (fmod(currTime, steplength) / steplength));
    }
  }
  currTime += (float)size / samplerate;
}


void setup() {
  float sample_rate;
  // Initialize for Daisy pod at 48kHz
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  num_channels = hw.num_channels;
  sample_rate = DAISY.get_samplerate();

  nse.Init();

  DAISY.begin(MyCallback);

  Serial.begin(9600);

  oled.setFont(u8g2_font_micro_tr);
  oled.setFontDirection(0);
  oled.setFontMode(1);
  oled.begin();

  uint32_t pin_cfg[] = { 0, 1, 2, 3 };
  shr.Init(pin_cfg, numRegisters);
}

void processState(long read, long *prev, long *curr) {
  int i = 0;
  int currBit = 0;
  for (i = 0; i < numRegisters * 8; i++) {
    currBit = bitRead(read, (numRegisters*8 - 1) -buttonRefs[i]);
    if (currBit != bitRead(*prev, i)) {
      bitWrite(*prev, i, currBit);
      if (bitRead(*prev, i) == 1) {
        bitWrite(*curr, i, abs(bitRead(*curr, i) - 1));
      }
    }
  }
}

void loop() {
  processState(shr.Read(), &readValue, &currState);
  oled.clearBuffer();
  y = 15;
  Serial.println(readValue, BIN);
  int i = 0;
  int x = 0;
  for (i = 0; i < numRegisters * 8; i++) {
    x = 10 + (i % 8) * 15;
    y = 15 + floor(i / 8) * 15;
    oled.drawCircle(x, y, 6);
    if (bitRead(currState, i) == 1) {
      oled.drawDisc(x, y, 3);
    } else {
    }
  }
  oled.sendBuffer();
  // Serial.println(str);
  delay(100);
}