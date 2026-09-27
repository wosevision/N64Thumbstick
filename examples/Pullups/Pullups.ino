/*
  Pull-ups

  The decoder expects driven high and low levels. If Diagnostics shows a
  line that chatters while the stick is still, that line needs a pull-up.
  usePullups(true) enables the pin's internal pull-up.

  On a 5 V board the internal pull-up pulls the module toward 5 V. Use
  external pull-ups to 3.3 V there (10 k is plenty). Set ENABLE_PULLUPS
  only on a 3.3 V board.

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

// Leave false on a 5 V Arduino. True is for a 3.3 V board whose lines float.
const bool ENABLE_PULLUPS = false;

N64Thumbstick stick;

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  if (ENABLE_PULLUPS) {
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || \
    defined(ARDUINO_AVR_MEGA) || defined(ARDUINO_AVR_MEGA2560)
    Serial.println(F("This is a 5 V board. Internal pull-ups pull toward 5 V."));
    Serial.println(F("Prefer external 10 k pull-ups to 3.3 V."));
#endif
  }

  stick.usePullups(ENABLE_PULLUPS);

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }

  Serial.print(F("Internal pull-ups "));
  Serial.println(ENABLE_PULLUPS ? F("on") : F("off"));
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
