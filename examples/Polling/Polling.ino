/*
  Polling

  Counts edges from loop() instead of from interrupts, so XA and YA can be
  any digital pins. update() has to run often: a fast swipe is a few
  milliseconds per edge, and delay() will miss some of them.

  Circuit:
  - Stick VCC to 3.3 V. Do not use 5 V.
  - Stick GND to GND
  - XA to pin 6
  - XB to pin 7
  - YA to pin 8
  - YB to pin 9

  created 27 Sep 2026
  by Jackson Teather

  This example code is in the public domain.

  https://github.com/wosevision/N64Thumbstick
*/

#include <N64Thumbstick.h>

const uint8_t PIN_XA = 6;
const uint8_t PIN_XB = 7;
const uint8_t PIN_YA = 8;
const uint8_t PIN_YB = 9;

N64Thumbstick stick;

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB, N64_THUMBSTICK_POLLING)) {
    Serial.println(F("begin() failed. Check that the four pins are distinct."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("Polling. Keep loop() free of delay()."));
}

void loop() {
  stick.update();

  static unsigned long lastPrint = 0;
  const unsigned long now = millis();
  if (now - lastPrint < 50) {
    return;
  }
  lastPrint = now;

  Serial.print(F("x="));
  Serial.print(stick.x());
  Serial.print(F(" y="));
  Serial.println(stick.y());
}
