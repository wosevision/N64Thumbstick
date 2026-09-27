/*
  Directions

  left(), right(), up(), and down() are true when the shaped axis passes
  the direction threshold. They follow the deadzone, so a reading of 0 is
  never a direction. Diagonals report two directions at once.

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

void printDirections() {
  bool any = false;
  if (stick.left()) {
    Serial.print(F("left "));
    any = true;
  }
  if (stick.right()) {
    Serial.print(F("right "));
    any = true;
  }
  if (stick.up()) {
    Serial.print(F("up "));
    any = true;
  }
  if (stick.down()) {
    Serial.print(F("down "));
    any = true;
  }
  if (!any) {
    Serial.print(F("center"));
  }
  Serial.println();
}

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  stick.setDirectionThreshold(N64_THUMBSTICK_DEFAULT_DIRECTION_THRESHOLD);

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }

  Serial.print(F("Threshold "));
  Serial.print(stick.directionThreshold());
  Serial.println(F(" counts."));
}

void loop() {
  stick.update();

  static bool lastLeft = false;
  static bool lastRight = false;
  static bool lastUp = false;
  static bool lastDown = false;

  const bool nowLeft = stick.left();
  const bool nowRight = stick.right();
  const bool nowUp = stick.up();
  const bool nowDown = stick.down();
  if (nowLeft == lastLeft && nowRight == lastRight && nowUp == lastUp &&
      nowDown == lastDown) {
    return;
  }

  lastLeft = nowLeft;
  lastRight = nowRight;
  lastUp = nowUp;
  lastDown = nowDown;
  printDirections();
}
