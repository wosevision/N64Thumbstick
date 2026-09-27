/*
  Invert and Swap

  A stick mounted upside down or turned in the shell reads backward or with
  the axes exchanged. invert and swap change later readings. They do not
  rewrite counts already stored. Set them before zero().

  If one axis runs backward, swapping that axis's two wires does the same
  thing as invert, because the decoder compares A with B.

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

const bool INVERT_X = true;
const bool INVERT_Y = false;
const bool SWAP_AXES = false;

N64Thumbstick stick;

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  stick.invertX(INVERT_X);
  stick.invertY(INVERT_Y);
  stick.swapAxes(SWAP_AXES);

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }

  // The stick should be centered now if this zero is the one you want.
  stick.zero();
  Serial.println(F("X is inverted. Right should still increase x."));
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
