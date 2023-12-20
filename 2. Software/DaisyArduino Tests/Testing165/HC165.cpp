#include "HC165.h"
#include "Arduino.h"

void ShiftRegister165::Init(uint32_t *pin_cfg, int num_daisy_chained) {
  // Initialize Pins as outputs
  for (size_t i = 0; i < NUM_PINS; i++) {
    pin_[i] = pin_cfg[i];

    if (i == PIN_DATA) {
      pinMode(pin_[i], INPUT);

    } else {
      pinMode(pin_[i], OUTPUT);
    }
  }
  digitalWrite(pin_[PIN_CLK], LOW);
  digitalWrite(pin_[PIN_LOAD], HIGH);

  // std::fill(state_, state_ + kMaxSr165DaisyChain, 0x00);
  num_devices_ = num_daisy_chained;
  // Set to 1 device if out of range.
  if (num_devices_ == 0 || num_devices_ > kMaxSr165DaisyChain)
    num_devices_ = 1;
}


long ShiftRegister165::Read() {
  digitalWrite(pin_[PIN_CLK_ENABLE], LOW);
  digitalWrite(pin_[PIN_LOAD], HIGH);
  long value = 0;
  long i;
  for (i = 0; i < num_devices_*8; ++i) {
    digitalWrite(pin_[PIN_CLK], LOW);
    value |= digitalRead(pin_[PIN_DATA]) << i;
    digitalWrite(pin_[PIN_CLK], HIGH);
  }
  digitalWrite(pin_[PIN_CLK_ENABLE], HIGH);
  digitalWrite(pin_[PIN_LOAD], LOW);
  return value;
}
