/*
 * N64Thumbstick
 * Copyright (C) 2026 Jackson Teather <jacksonteather@hotmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Read a Nintendo 64 optical thumbstick module (the 6-pin stick, not the
 * rest of the controller).
 *
 * Each axis is a quadrature pair. XA and YA are the edge sources. XB and YB
 * are sampled on each edge of A to decide the sign. The position is the
 * running sum of those steps. It starts at 0 wherever the stick happens to
 * be when begin() runs, which is how the NUS-CNT chip treats the module.
 * Center the stick and call zero() when you want the relaxed position to
 * read as 0.
 *
 * Power the module at 3.3 V. Do not tie VCC to 5 V.
 *
 * Keep the object alive for the whole run, usually as a global. Call begin()
 * from setup(). The object cannot be copied. At most
 * N64_THUMBSTICK_MAX_INSTANCES objects can be active at once.
 *
 * Hardware notes: extras/Wiring.md and extras/Quadrature.md.
 * https://n64brew.dev/wiki/Controller#Analog_Stick
 */

#ifndef N64_THUMBSTICK_H
#define N64_THUMBSTICK_H

#include <Arduino.h>

#include "internal/AxisDecoder.h"

// Approximate raw A-edges from the physical center to the gate, from
// measurements of about 18 steps per side. Calibrate a worn stick.
constexpr long N64_THUMBSTICK_DEFAULT_FULL_SCALE = 18;

// The console reports 4 ordinate units per A-edge, then saturates.
constexpr int N64_THUMBSTICK_DEFAULT_UNITS_PER_EDGE = 4;

// int8 saturation used by the controller IC. Games are calmer near +/-80.
constexpr int N64_THUMBSTICK_DEFAULT_CLAMP = 127;

// Raw counts, after the deadzone, before a direction reads as pressed.
constexpr long N64_THUMBSTICK_DEFAULT_DIRECTION_THRESHOLD = 6;

constexpr uint8_t N64_THUMBSTICK_MAX_INSTANCES = 4;

enum N64ThumbstickMode {
  N64_THUMBSTICK_INTERRUPT = 0,
  N64_THUMBSTICK_POLLING = 1
};

// Called from update(), not from the interrupt. Arguments match x() and y().
typedef void (*N64ThumbstickCallback)(long x, long y);

class N64Thumbstick {
public:
  N64Thumbstick();
  ~N64Thumbstick();

  N64Thumbstick(const N64Thumbstick&) = delete;
  N64Thumbstick& operator=(const N64Thumbstick&) = delete;
  N64Thumbstick(N64Thumbstick&&) = delete;
  N64Thumbstick& operator=(N64Thumbstick&&) = delete;

  // Attach the four signals. XA and YA must support external interrupts
  // when mode is N64_THUMBSTICK_INTERRUPT; on an Uno those are pins 2 and 3.
  // Polling mode works on any digital pin. Call update() every pass of
  // loop() in polling mode, and whenever an onChange callback is set.
  // Returns false if the pins are invalid, already used, not interrupt
  // capable for interrupt mode, or the instance limit is full.
  // begin() resets the counts and the origin. Invert, deadzone, scale,
  // clamp, pull-ups, and the callback are left as you set them.
  bool begin(uint8_t pinXa, uint8_t pinXb, uint8_t pinYa, uint8_t pinYb,
             N64ThumbstickMode mode = N64_THUMBSTICK_INTERRUPT);

  // Poll edges in polling mode, then deliver onChange if the shaped
  // position moved. Safe to call in interrupt mode.
  void update();

  // Detach interrupts and release the pins. The last counts stay readable.
  void end();

  // Logical counts: origin, invert, and swap applied, then the deadzone.
  // Right and up are positive unless inverted.
  long x() const;
  long y() const;

  // Both axes from one interrupt-safe snapshot. Same shaping as x() and y().
  void read(long& xOut, long& yOut) const;

  // Hardware counts since begin(), with no origin, invert, swap, or deadzone.
  long xRaw() const;
  long yRaw() const;

  // Console-style ordinates: shaped counts times unitsPerEdge, clamped.
  // Print these with an int cast. Printing an int8_t writes a character.
  int8_t xN64() const;
  int8_t yN64() const;

  // Shaped counts divided by (full scale - deadzone), clamped to -1..1.
  float xNormalized() const;
  float yNormalized() const;

  // Raw edges per second for that logical axis, from the gap between the
  // last two A-edges. Returns 0 once the stick has stopped.
  float xVelocity() const;
  float yVelocity() const;

  // True when the shaped axis has passed the direction threshold.
  bool left() const;
  bool right() const;
  bool up() const;
  bool down() const;

  // The current physical position becomes logical 0, 0.
  void zero();

  // Absolute offsets subtracted after invert and swap, measured from the
  // position at the last begin(). setOrigin(0, 0) restores that frame.
  // zero() is the call that recenters on wherever the stick is now.
  void setOrigin(long x, long y);

  // Flip a logical axis. Takes effect on later reads; stored counts stay.
  // Set this before zero() if a mounted stick points the wrong way.
  void invertX(bool invert);
  void invertY(bool invert);

  // Exchange the logical axes. Set this before zero().
  void swapAxes(bool swap);

  // Counts inside +/-deadzone read as 0. Outside it, the deadzone width is
  // subtracted so the value is continuous at the edge of the window.
  // Directions use the value after this window. Default is 0.
  void setDeadzone(long counts);
  long deadzone() const;

  void setDirectionThreshold(long counts);
  long directionThreshold() const;

  // Ignored when units < 1. Values above 127 are clamped to 127.
  void setUnitsPerEdge(int units);
  int unitsPerEdge() const;

  // Clamp limit for xN64() and yN64(), from 1 to 127.
  void setN64Clamp(int limit);
  int n64Clamp() const;

  // Full-scale raw throw used by the normalized readings. Non-positive
  // values are ignored. The one-argument form sets both axes.
  void setFullScale(long counts);
  void setFullScale(long countsX, long countsY);
  long fullScaleX() const;
  long fullScaleY() const;

  // Learn full scale from the largest |logical| count seen between
  // calibrateBegin() and calibrateEnd(). Center and zero() first so the
  // center is 0. Returns false from calibrateEnd() if nothing moved.
  void calibrateBegin();
  void calibrateUpdate();
  bool calibrateEnd();

  // Replace the callback, or pass nullptr to clear it. The callback runs
  // from update() and must not call update() again.
  void onChange(N64ThumbstickCallback callback);

  // Internal pull-ups on the four signal pins. Call before or after begin().
  // On a 5 V board this pulls the module's outputs toward 5 V. Prefer
  // external pull-ups to 3.3 V there. Leave this off unless a line floats.
  void usePullups(bool enabled);

  // Build architecture name ("avr", "esp32", ...), or "generic".
  static const char* architecture();

  // True on cores where external interrupts exist only on specific pins.
  // pinSupportsInterrupt() is the per-pin check begin() actually uses.
  static bool limitedInterruptPins();

  // True when this core reports an external interrupt on the pin.
  static bool pinSupportsInterrupt(uint8_t pin);

private:
  static N64Thumbstick* _slots[N64_THUMBSTICK_MAX_INSTANCES];
  static void dispatch(uint8_t slot, uint8_t axis);
  static void isrX0();
  static void isrY0();
  static void isrX1();
  static void isrY1();
  static void isrX2();
  static void isrY2();
  static void isrX3();
  static void isrY3();

  int8_t claimSlot();
  void dropSlot();
  bool pinsAreValid(uint8_t pinXa, uint8_t pinXb, uint8_t pinYa,
                    uint8_t pinYb) const;
  bool pinsConflict() const;
  bool attachInterrupts();
  void detachInterrupts();
  void applyPinModes();
  void snapshot(long& hardwareX, long& hardwareY) const;
  long preX(long hardwareX, long hardwareY) const;
  long preY(long hardwareX, long hardwareY) const;
  long logicalX() const;
  long logicalY() const;
  long shape(long logical) const;
  int8_t toN64(long shaped) const;
  float normalize(long shaped, long fullScale) const;

  N64ThumbstickInternal::AxisDecoder _axisX;
  N64ThumbstickInternal::AxisDecoder _axisY;
  N64ThumbstickCallback _callback;
  uint8_t _pinXa;
  uint8_t _pinXb;
  uint8_t _pinYa;
  uint8_t _pinYb;
  uint8_t _mode;
  int8_t _slot;
  int8_t _n64Clamp;
  int _unitsPerEdge;
  long _offsetX;
  long _offsetY;
  long _deadzone;
  long _directionThreshold;
  long _fullScaleX;
  long _fullScaleY;
  long _lastX;
  long _lastY;
  long _calMinX;
  long _calMaxX;
  long _calMinY;
  long _calMaxY;
  bool _begun;
  bool _pullups;
  bool _invertX;
  bool _invertY;
  bool _swap;
  bool _baselineSet;
  bool _inCallback;
  bool _calibrating;
  bool _haveCalSample;
};

#endif
