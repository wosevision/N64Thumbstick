/*
 * N64Thumbstick
 * Copyright (C) 2026 Jackson Teather <jacksonteather@hotmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Internal. Sketches should include <N64Thumbstick.h>.
 * This header lives under src/ so it is compiled, and under a subfolder so
 * Sketch > Include Library does not add it to a sketch.
 */

#ifndef N64_THUMBSTICK_INTERNAL_AXIS_DECODER_H
#define N64_THUMBSTICK_INTERNAL_AXIS_DECODER_H

#include <Arduino.h>

class N64Thumbstick;

namespace N64ThumbstickInternal {

// One quadrature axis. A is the edge source (XA or YA). B is the
// direction sample (XB or YB). The count starts at 0 and steps by 1
// on every edge of A: +1 when the new A level equals B, -1 otherwise.
class AxisDecoder {
  friend class ::N64Thumbstick;

public:
  AxisDecoder();

private:
  void begin(uint8_t pinA, uint8_t pinB, uint8_t inputMode);
  void end();
  void handleEdge();
  bool poll();
  long count() const;
  long peek() const;
  float countsPerSecond() const;
  uint8_t pinA() const;
  uint8_t pinB() const;

  uint8_t _pinA;
  uint8_t _pinB;
  uint8_t _lastA;
  volatile long _count;
  volatile unsigned long _lastEdgeUs;
  volatile unsigned long _intervalUs;
  volatile int8_t _lastStep;
};

}  // namespace N64ThumbstickInternal

#endif
