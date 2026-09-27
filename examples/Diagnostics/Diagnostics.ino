/*
  Diagnostics

  Prints the architecture, which pins can interrupt, and the live level of
  each signal next to the raw and logical counts. Use it when a new module
  is wired and an axis runs backward or sits still.

  Move one axis slowly. Both wires of that pair should change. If the count
  goes the wrong way, swap those two wires or call invertX() / invertY().
  If begin() cannot attach interrupts, the sketch falls back to polling.

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

void printPinSupport(uint8_t pin) {
  Serial.print(F("Pin "));
  Serial.print(pin);
  Serial.print(F(" interrupt: "));
  Serial.println(N64Thumbstick::pinSupportsInterrupt(pin) ? F("yes") : F("no"));
}

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  Serial.print(F("Architecture: "));
  Serial.println(N64Thumbstick::architecture());
  if (N64Thumbstick::limitedInterruptPins()) {
    Serial.println(F("External interrupts are limited to specific pins."));
  } else {
    Serial.println(F("Most digital pins on this board can interrupt."));
  }
  printPinSupport(2);
  printPinSupport(6);

  bool ok = stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB, N64_THUMBSTICK_INTERRUPT);
  if (!ok) {
    Serial.println(F("Interrupt mode unavailable. Falling back to polling."));
    ok = stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB, N64_THUMBSTICK_POLLING);
  }
  if (!ok) {
    Serial.println(F("begin() failed. Check that the four pins are distinct."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("raw counts, then logical x y, then XA XB YA YB."));
}

void loop() {
  stick.update();

  static unsigned long lastPrint = 0;
  const unsigned long now = millis();
  if (now - lastPrint < 100) {
    return;
  }
  lastPrint = now;

  Serial.print(stick.xRaw());
  Serial.print(F(","));
  Serial.print(stick.yRaw());
  Serial.print(F("  "));
  Serial.print(stick.x());
  Serial.print(F(","));
  Serial.print(stick.y());
  Serial.print(F("  "));
  Serial.print(digitalRead(PIN_XA) == HIGH ? F("H") : F("L"));
  Serial.print(digitalRead(PIN_XB) == HIGH ? F("H") : F("L"));
  Serial.print(digitalRead(PIN_YA) == HIGH ? F("H") : F("L"));
  Serial.println(digitalRead(PIN_YB) == HIGH ? F("H") : F("L"));
}
