/*
  Basic Read

  Prints the thumbstick position as raw logical counts. Right and up are
  positive. The count starts at 0 wherever the stick is when begin() runs.
  Hold the stick centered, then reset the board, or use the Recenter example.

  Circuit:
  - Stick VCC to 3.3 V. Do not use 5 V.
  - Stick GND to GND
  - XA to pin 2 (external interrupt on an Uno)
  - XB to pin 4
  - YA to pin 3 (external interrupt on an Uno)
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

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed. XA and YA must be interrupt pins,"));
    Serial.println(F("or use the Polling example."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("x and y are logical counts. Center was 0 at begin()."));
}

void loop() {
  // Interrupt mode updates the counts in the background. update() is what
  // delivers onChange callbacks, and it is required in polling mode.
  stick.update();

  static unsigned long lastPrint = 0;
  const unsigned long now = millis();
  if (now - lastPrint < 50) {
    return;
  }
  lastPrint = now;

  long x = 0;
  long y = 0;
  stick.read(x, y);
  Serial.print(F("x="));
  Serial.print(x);
  Serial.print(F(" y="));
  Serial.println(y);
}
