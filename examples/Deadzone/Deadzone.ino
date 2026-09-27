/*
  Deadzone

  Counts inside the window read as 0. Past the window, the width is
  subtracted, so the reading is 0 at the edge and grows without a jump.
  A center window of about 7 console units is the most the N64 programming
  manual allows, and the console uses 4 units per edge, so 2 raw counts is
  a reasonable match.

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

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  stick.setDeadzone(2);

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }

  Serial.print(F("Deadzone "));
  Serial.print(stick.deadzone());
  Serial.println(F(" counts."));
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
