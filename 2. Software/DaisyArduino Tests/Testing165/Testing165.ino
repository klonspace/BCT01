/*
  DigitalReadSerial

  Reads a digital input on pin 2, prints the result to the Serial Monitor

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/DigitalReadSerial
*/
#include "HC165.h"


static ShiftRegister165 shr;
long readValue = 0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // make the pushbutton's pin an input:
  uint32_t pin_cfg[] = { 0, 1, 2, 3 };
  shr.Init(pin_cfg, 2);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input pin:
  readValue = shr.Read();
  
  // print out the state of the button:
  Serial.println(readValue, BIN);
  Serial.println(bitRead(readValue, 0));
  delay(100);  // delay in between reads for stability
}
