/*
  On Change

  onChange runs from update(), after the shaped position moves. It does not
  run from the interrupt, so Serial is safe here. Do not call update() from
  the callback. The first update() only records a baseline.

  Circuit:
  - Stick VCC to 3.3 V. Do not use 5 V.
  - Stick GND to GND
  - XA to pin 2
  - XB to pin 4
  - YA to pin 3
  - YB to pin 5

  created 27 Sep 2026
  by Jackson Teather

  This example code is in the public domain.

  https://github.com/wosevision/N64Thumbstick
*/

#include <N64Thumbstick.h>

const uint8_t PIN_XA = 2;
const uint8_t PIN_XB = 4;
const uint8_t PIN_YA = 3;
const uint8_t PIN_YB = 5;

N64Thumbstick stick;

void stickMoved(long x, long y) {
  Serial.print(F("moved x="));
  Serial.print(x);
  Serial.print(F(" y="));
  Serial.println(y);
}

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  stick.onChange(stickMoved);

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("Move the stick. Lines appear only when the count changes."));
}

void loop() {
  stick.update();
}
