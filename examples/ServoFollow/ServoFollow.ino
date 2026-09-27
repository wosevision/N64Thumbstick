/*
  Servo Follow

  Maps the X axis from -1..1 onto a hobby-servo pulse (about 1000 to 2000
  microseconds, repeated every 20 milliseconds). The pulse is generated in
  loop() with micros(), so this sketch does not need the Servo library and
  does not block long enough to drop edges.

  The stick stays on 3.3 V. Power the servo from 5 V, or from its own
  supply, and tie the grounds together. A small deadzone keeps a resting
  stick from buzzing the servo. See the Deadzone example.

  Circuit:
  - Stick VCC to 3.3 V. Do not use 5 V.
  - Stick GND to GND, common with the servo ground
  - XA to pin 2
  - XB to pin 4
  - YA to pin 3
  - YB to pin 5
  - Servo signal to pin 9

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
const uint8_t PIN_SERVO = 9;
const unsigned long kFrameUs = 20000;

N64Thumbstick stick;

unsigned long frameStart = 0;
unsigned long pulseUs = 1500;
bool pulseHigh = false;

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

  pinMode(PIN_SERVO, OUTPUT);
  digitalWrite(PIN_SERVO, LOW);
  stick.setDeadzone(2);

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }

  frameStart = micros();
  Serial.println(F("Servo follows X. Center is about 1500 microseconds."));
}

void loop() {
  stick.update();

  const unsigned long now = micros();
  if (!pulseHigh && (now - frameStart) >= kFrameUs) {
    const float x = stick.xNormalized();
    long mapped = 1500L + static_cast<long>(x * 500.0f);
    if (mapped < 1000L) {
      mapped = 1000L;
    }
    if (mapped > 2000L) {
      mapped = 2000L;
    }
    pulseUs = static_cast<unsigned long>(mapped);
    frameStart = now;
    digitalWrite(PIN_SERVO, HIGH);
    pulseHigh = true;
  } else if (pulseHigh && (now - frameStart) >= pulseUs) {
    digitalWrite(PIN_SERVO, LOW);
    pulseHigh = false;
  }
}
