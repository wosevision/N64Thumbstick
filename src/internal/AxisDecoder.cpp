/*
 * N64Thumbstick
 * Copyright (C) 2026 Jackson Teather <jacksonteather@hotmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "AxisDecoder.h"

// A stuck axis can report a multi-second gap. Keep the 2x hold from wrapping.
static const unsigned long kIntervalOverflowGuard = 2000000000UL;
// After this long with no further edge, a fast flick is over.
static const unsigned long kVelocityHoldFloorUs = 20000UL;

namespace N64ThumbstickInternal {

AxisDecoder::AxisDecoder()
    : _pinA(0),
      _pinB(0),
      _lastA(0),
      _count(0),
      _lastEdgeUs(0),
      _intervalUs(0),
      _lastStep(0) {}

void AxisDecoder::begin(uint8_t pinA, uint8_t pinB, uint8_t inputMode) {
  _pinA = pinA;
  _pinB = pinB;
  pinMode(_pinA, inputMode);
  pinMode(_pinB, inputMode);
  _count = 0;
  _lastEdgeUs = 0;
  _intervalUs = 0;
  _lastStep = 0;
  _lastA = digitalRead(_pinA) == HIGH ? 1 : 0;
}

void AxisDecoder::end() {
  pinMode(_pinA, INPUT);
  pinMode(_pinB, INPUT);
}

void AxisDecoder::handleEdge() {
  const uint8_t levelA = digitalRead(_pinA) == HIGH ? 1 : 0;
  const uint8_t levelB = digitalRead(_pinB) == HIGH ? 1 : 0;
  const int8_t step = (levelA == levelB) ? 1 : -1;
  _lastA = levelA;
  _count += step;

  const unsigned long now = micros();
  if (_lastEdgeUs != 0) {
    _intervalUs = now - _lastEdgeUs;
  }
  _lastEdgeUs = now;
  _lastStep = step;
}

bool AxisDecoder::poll() {
  const uint8_t levelA = digitalRead(_pinA) == HIGH ? 1 : 0;
  if (levelA == _lastA) {
    return false;
  }
  handleEdge();
  return true;
}

long AxisDecoder::count() const {
  noInterrupts();
  const long value = _count;
  interrupts();
  return value;
}

long AxisDecoder::peek() const {
  return _count;
}

float AxisDecoder::countsPerSecond() const {
  noInterrupts();
  const unsigned long interval = _intervalUs;
  const unsigned long lastEdge = _lastEdgeUs;
  const int8_t step = _lastStep;
  interrupts();

  if (interval == 0 || step == 0 || lastEdge == 0) {
    return 0.0f;
  }

  const unsigned long since = micros() - lastEdge;
  const unsigned long hold = (interval > kIntervalOverflowGuard)
                                 ? (kIntervalOverflowGuard * 2UL)
                                 : (interval * 2UL);
  if (since > hold && since > kVelocityHoldFloorUs) {
    return 0.0f;
  }

  const float magnitude = 1000000.0f / static_cast<float>(interval);
  return (step > 0) ? magnitude : -magnitude;
}

uint8_t AxisDecoder::pinA() const {
  return _pinA;
}

uint8_t AxisDecoder::pinB() const {
  return _pinB;
}

}  // namespace N64ThumbstickInternal
