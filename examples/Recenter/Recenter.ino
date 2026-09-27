/*
  Recenter

  The module reports movement, not an absolute angle, so the zero point is
  wherever the stick was at begin(). zero() makes the current position read
  as 0, 0. setOrigin(0, 0) restores the frame from the last begin().

  Open the Serial Monitor and send a line.
  - z  capture the current position as zero
  - r  restore the begin() frame

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

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("Center the stick and send z. Send r to undo."));
}

void loop() {
  stick.update();

  if (Serial.available() > 0) {
    const char command = Serial.read();
    if (command == 'z' || command == 'Z') {
      stick.zero();
      Serial.println(F("Origin set to the current position."));
    } else if (command == 'r' || command == 'R') {
      stick.setOrigin(0, 0);
      Serial.println(F("Origin restored to the begin() position."));
    }
  }

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
