/*
  N64 Ordinates

  The controller chip reports each A-edge as 4 signed units and saturates
  at the int8 limits. Games such as Super Mario 64 misbehave past about
  +/-80, so this sketch clamps there. setN64Clamp(127) matches the chip.

  Serial.print on an int8_t writes a character. Cast to int.

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

  stick.setUnitsPerEdge(N64_THUMBSTICK_DEFAULT_UNITS_PER_EDGE);
  stick.setN64Clamp(80);

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }

  Serial.print(stick.unitsPerEdge());
  Serial.print(F(" console units per edge, clamp +/-"));
  Serial.println(stick.n64Clamp());
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
  Serial.print(static_cast<int>(stick.xN64()));
  Serial.print(F(" y="));
  Serial.println(static_cast<int>(stick.yN64()));
}
