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

  An ATtiny84 or ATtiny85 has no USB controller. This sketch bit-bangs
  low-speed USB with V-USB (the files in usbdrv/, GPL v2 or v3, not
  public domain). The rest of the sketch is public domain. The chip
  runs at 5 V from USB VBUS. The stick stays at 3.3 V. A 3.3 V high
  still reads as high on a 5 V input (the threshold is 0.6 times VCC).
  Neither part is rated for the USB clocks below 4.5 V.

  The two chips share the mouse report and the poll loop. They do not
  share a clock or a pinout. The 84 has no PLL, so it needs a crystal.
  The 85's PLL can make 16.5 MHz, and a crystal there would land on the
  USB pins. The 85 also has six I/O pins, so the fourth stick wire is
  reset.

  USB wiring, low speed, from the V-USB notes. The D+ and D- pins are
  named in the chip sections below.
  - D+ through 68 ohms. D- through 68 ohms.
  - 3.6 V zener from each of those AVR pins to GND, cathode at the pin.
  - 1.5k from D- to 5 V.
  - 1M from D+ to GND.
  - USB VBUS to the chip's VCC, USB GND to GND.

  ATtiny84 (ATTinyCore). Tools > Clock > 16 MHz (external). A 12 or 20
  MHz crystal also times V-USB if the menu matches it. Crystal on PB0
  and PB1, about 18 pF from each leg to GND. Burn Bootloader once with
  the crystal fitted, then upload with ISP. Without the crystal the chip
  has no clock and ISP will not answer until it is connected. D+ is PA0
  and D- is PA1, a pin change on PA0, so INT0 and RESET stay free.
  Clockwise pin numbers (Tools > Pin Mapping, the default) are D+ 0 and
  D- 1. Counterclockwise they are 10 and 9. The stick is polled: XA PA2,
  XB PA3, YA PA7, YB PB2 (clockwise: 2, 3, 7, and 8). ISP stays on
  RESET, PA4, PA5, and PA6.

  ATtiny85 (ATTinyCore). Tools > Clock > 16.5 MHz (PLL, tweaked). That
  menu nudges OSCCAL because V-USB's 16.5 MHz timing was written for
  this PLL, and it does not use a crystal. Burn Bootloader once so the
  PLL fuse takes. D- is PB3 (pin 3) and D+ is PB4 (pin 4), the same pair
  a Digispark already wires to the plug. The stick is polled: XA pin 0
  (PB0), XB pin 1 (PB1), YA pin 2 (PB2), YB pin 5 (PB5). Pin 5 is reset
  until RSTDISBL is programmed (high fuse bit 7 cleared). That board
  menu leaves the bit set, and clearing it disables ISP until a
  high-voltage programmer restores reset.

  There is no hardware UART, and the software serial pins collide with
  the USB pins, so this build does not print. Call usbPoll() every loop.
  A delay() long enough to miss a USB frame will make the host drop the
  device. The timer interrupt can delay a bit sample, so a report is
  sometimes skipped. The host retries enumeration.

  Circuit, other boards:
  - Stick VCC to 3.3 V. Do not use 5 V.
  - Stick GND to GND
  - XA to pin 2
  - XB to pin 4
  - YA to pin 3
  - YB to pin 5

  created 27 Sep 2026
  by Jackson Teather

  This example code is in the public domain, except usbdrv/.

  https://github.com/wosevision/N64Thumbstick
*/

#include <N64Thumbstick.h>

// Set to 1 after installing the Mouse library. Leave at 0 otherwise.
// The ATtiny84 and ATtiny85 builds do not use this flag. They always
// include V-USB.
#ifndef N64_EXAMPLE_USE_MOUSE
#define N64_EXAMPLE_USE_MOUSE 0
#endif

// USBCON exists on the 32U4 (Leonardo, Micro) and not on an Uno.
#if N64_EXAMPLE_USE_MOUSE && defined(USBCON)
#include <Mouse.h>
#define N64_EXAMPLE_HAS_MOUSE 1
#elif defined(__AVR_ATtinyX4__) || defined(__AVR_ATtinyX5__)
#include "vusb_tiny.h"
#define N64_EXAMPLE_HAS_VUSB 1
#endif

#if defined(__AVR_ATtinyX4__)
const uint8_t PIN_XA = PIN_PA2;
const uint8_t PIN_XB = PIN_PA3;
const uint8_t PIN_YA = PIN_PA7;
const uint8_t PIN_YB = PIN_PB2;
const N64ThumbstickMode STICK_MODE = N64_THUMBSTICK_POLLING;
static_assert(PIN_XA != PIN_PA0 && PIN_XB != PIN_PA0 && PIN_YA != PIN_PA0 &&
                  PIN_YB != PIN_PA0,
              "stick pin is USB D+");
static_assert(PIN_XA != PIN_PA1 && PIN_XB != PIN_PA1 && PIN_YA != PIN_PA1 &&
                  PIN_YB != PIN_PA1,
              "stick pin is USB D-");
static_assert(PIN_XA != PIN_PB0 && PIN_XB != PIN_PB0 && PIN_YA != PIN_PB0 &&
                  PIN_YB != PIN_PB0 && PIN_XA != PIN_PB1 && PIN_XB != PIN_PB1 &&
                  PIN_YA != PIN_PB1 && PIN_YB != PIN_PB1,
              "stick pin is a crystal pin");
#elif defined(__AVR_ATtinyX5__)
const uint8_t PIN_XA = 0;
const uint8_t PIN_XB = 1;
const uint8_t PIN_YA = 2;
const uint8_t PIN_YB = 5;
const N64ThumbstickMode STICK_MODE = N64_THUMBSTICK_POLLING;
static_assert(PIN_XA != 3 && PIN_XB != 3 && PIN_YA != 3 && PIN_YB != 3,
              "stick pin is USB D-");
static_assert(PIN_XA != 4 && PIN_XB != 4 && PIN_YA != 4 && PIN_YB != 4,
              "stick pin is USB D+");
#else
const uint8_t PIN_XA = 2;
const uint8_t PIN_XB = 4;
const uint8_t PIN_YA = 3;
const uint8_t PIN_YB = 5;
const N64ThumbstickMode STICK_MODE = N64_THUMBSTICK_INTERRUPT;
#endif

const int kSpeed = 8;

N64Thumbstick stick;

void pointerDelta(int& dx, int& dy) {
  dx = static_cast<int>(stick.xNormalized() * kSpeed);
  dy = static_cast<int>(-stick.yNormalized() * kSpeed);
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
}

void setup() {
#if !defined(N64_EXAMPLE_HAS_VUSB)
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif

#if defined(N64_EXAMPLE_HAS_MOUSE)
  Serial.println(F("Stick X/Y nudges the pointer."));
#elif N64_EXAMPLE_USE_MOUSE
  Serial.println(F("N64_EXAMPLE_USE_MOUSE is set, but this board has no USB mouse."));
#elif defined(USBCON)
  Serial.println(F("Install the Mouse library, then set N64_EXAMPLE_USE_MOUSE to 1."));
#else
  Serial.println(F("This board has no USB mouse support."));
  Serial.println(F("Use a Leonardo or Micro, install Mouse, and set the flag."));
#endif
#endif

#if defined(N64_EXAMPLE_HAS_VUSB)
  n64UsbMouseBegin();
#elif defined(N64_EXAMPLE_HAS_MOUSE)
  Mouse.begin();
#endif

  stick.setDeadzone(2);
  if (!stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB, STICK_MODE)) {
#if !defined(N64_EXAMPLE_HAS_VUSB)
    Serial.println(F("begin() failed."));
#endif
    while (true) {
#if defined(N64_EXAMPLE_HAS_VUSB)
      n64UsbMousePoll();
#else
      delay(1000);
#endif
    }
  }
}

void loop() {
  stick.update();

#if defined(N64_EXAMPLE_HAS_VUSB)
  n64UsbMousePoll();
  static unsigned long lastMove = 0;
  const unsigned long now = millis();
  if (static_cast<unsigned long>(now - lastMove) >= 15) {
    lastMove = now;
    int dx = 0;
    int dy = 0;
    pointerDelta(dx, dy);
    if (dx != 0 || dy != 0) {
      n64UsbMouseMove(static_cast<int8_t>(dx), static_cast<int8_t>(dy));
    }
  }
#elif defined(N64_EXAMPLE_HAS_MOUSE)
  int dx = 0;
  int dy = 0;
  pointerDelta(dx, dy);
  if (dx != 0 || dy != 0) {
    Mouse.move(static_cast<signed char>(dx), static_cast<signed char>(dy));
  }
  delay(15);
#else
  delay(1000);
#endif
}
