/*
  Two Sticks

  Each module is its own object. An Uno has only two external-interrupt
  pins and each stick needs two, so both sticks use polling here. A Mega
  can run each stick in interrupt mode on pins 2, 3, 18, and 19.

  At most N64_THUMBSTICK_MAX_INSTANCES objects can be active.

  Circuit:
  - Both VCC pins to 3.3 V. Do not use 5 V. Share GND.
  - Stick A: XA 2, XB 4, YA 3, YB 5
  - Stick B: XA 6, XB 8, YA 7, YB 9

  created 27 Sep 2026
  by Jackson Teather

  This example code is in the public domain.

  https://github.com/wosevision/N64Thumbstick
*/

#include <N64Thumbstick.h>

N64Thumbstick stickA;
N64Thumbstick stickB;

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  const bool aOk =
      stickA.begin(2, 4, 3, 5, N64_THUMBSTICK_POLLING);
  const bool bOk =
      stickB.begin(6, 8, 7, 9, N64_THUMBSTICK_POLLING);
  if (!aOk || !bOk) {
    Serial.println(F("begin() failed. The eight signal pins must all differ."));
    while (true) {
      delay(1000);
    }
  }

  Serial.println(F("Two sticks, polling. A is listed first."));
}

void loop() {
  stickA.update();
  stickB.update();

  static unsigned long lastPrint = 0;
  const unsigned long now = millis();
  if (now - lastPrint < 50) {
    return;
  }
  lastPrint = now;

  Serial.print(F("A "));
  Serial.print(stickA.x());
  Serial.print(F(","));
  Serial.print(stickA.y());
  Serial.print(F("  B "));
  Serial.print(stickB.x());
  Serial.print(F(","));
  Serial.println(stickB.y());
}
