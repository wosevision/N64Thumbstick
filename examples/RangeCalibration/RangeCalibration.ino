/*
  Range Calibration

  Center the stick when the sketch asks, then push it out to the gate in
  every direction. calibrateEnd() stores the largest distance from the
  origin on each axis as the full scale used by the normalized readings.

  This sketch uses interrupt mode, so the pause in setup() does not drop
  edges. zero() after that pause throws those edges away.

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
const unsigned long kSettleMs = 2500;
const unsigned long kSweepMs = 8000;

N64Thumbstick stick;
unsigned long sweepStart = 0;
bool calibrated = false;

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

  Serial.println(F("Center the stick and hold it there."));
  delay(kSettleMs);
  stick.zero();
  stick.calibrateBegin();
  sweepStart = millis();
  Serial.println(F("Push the stick to every edge for 8 seconds."));
}

void loop() {
  stick.update();
  stick.calibrateUpdate();

  if (!calibrated && (millis() - sweepStart) >= kSweepMs) {
    calibrated = true;
    if (stick.calibrateEnd()) {
      Serial.print(F("Full scale x="));
      Serial.print(stick.fullScaleX());
      Serial.print(F(" y="));
      Serial.println(stick.fullScaleY());
    } else {
      Serial.println(F("No movement detected. Full scale unchanged."));
    }
  }

  if (!calibrated) {
    return;
  }

  static unsigned long lastPrint = 0;
  const unsigned long now = millis();
  if (now - lastPrint < 50) {
    return;
  }
  lastPrint = now;

  Serial.print(F("x="));
  Serial.print(stick.xNormalized(), 3);
  Serial.print(F(" y="));
  Serial.println(stick.yNormalized(), 3);
}
