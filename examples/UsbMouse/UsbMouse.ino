/*
  USB Mouse

  A held direction nudges the pointer. Screen Y grows downward, so the
  stick's positive Y is negated. The deadzone keeps a resting stick from
  drifting the cursor.

  On a Leonardo or Micro, install the Mouse library from Library Manager
  and set N64_EXAMPLE_USE_MOUSE to 1 below. The pointer then follows the
  stick. Left at 0, the sketch compiles on any board and prints why the
  pointer will not move. The include has to be a normal #include, not a
  probe, or the IDE will not add the Mouse library to the build.

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

// Set to 1 after installing the Mouse library. Leave at 0 otherwise.
#ifndef N64_EXAMPLE_USE_MOUSE
#define N64_EXAMPLE_USE_MOUSE 0
#endif

// USBCON exists on the 32U4 (Leonardo, Micro) and not on an Uno.
#if N64_EXAMPLE_USE_MOUSE && defined(USBCON)
#include <Mouse.h>
#define N64_EXAMPLE_HAS_MOUSE 1
#endif

const uint8_t PIN_XA = 2;
const uint8_t PIN_XB = 4;
const uint8_t PIN_YA = 3;
const uint8_t PIN_YB = 5;
const int kSpeed = 8;

N64Thumbstick stick;

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

#if defined(N64_EXAMPLE_HAS_MOUSE)
  stick.setDeadzone(2);
  Mouse.begin();
  Serial.println(F("Stick X/Y nudges the pointer."));
#elif N64_EXAMPLE_USE_MOUSE
  Serial.println(F("N64_EXAMPLE_USE_MOUSE is set, but this board has no USB mouse."));
#elif defined(USBCON)
  Serial.println(F("Install the Mouse library, then set N64_EXAMPLE_USE_MOUSE to 1."));
#else
  Serial.println(F("This board has no USB mouse support."));
  Serial.println(F("Use a Leonardo or Micro, install Mouse, and set the flag."));
#endif

  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB)) {
    Serial.println(F("begin() failed."));
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  stick.update();

#if defined(N64_EXAMPLE_HAS_MOUSE)
  int dx = static_cast<int>(stick.xNormalized() * kSpeed);
  int dy = static_cast<int>(-stick.yNormalized() * kSpeed);
  if (dx > 127) {
    dx = 127;
  }
  if (dx < -128) {
    dx = -128;
  }
  if (dy > 127) {
    dy = 127;
  }
  if (dy < -128) {
    dy = -128;
  }
  if (dx != 0 || dy != 0) {
    Mouse.move(static_cast<signed char>(dx), static_cast<signed char>(dy));
  }
  delay(15);
#else
  delay(1000);
#endif
}
