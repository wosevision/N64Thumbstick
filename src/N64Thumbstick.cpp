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

#include "N64Thumbstick.h"

#include "internal/Arch.h"

// The interrupt thunks live in this file on purpose. With dot_a_linkage the
// linker pulls in a .o only when something in it is referenced. begin()
// references these thunks, so the vectors stay in the archive. An ISR in a
// .cpp that nothing calls would be dropped.

N64Thumbstick* N64Thumbstick::_slots[N64_THUMBSTICK_MAX_INSTANCES];

namespace {

long magnitude(long value) {
  return (value < 0) ? -value : value;
}

bool knownMode(uint8_t mode) {
  return mode == N64_THUMBSTICK_INTERRUPT || mode == N64_THUMBSTICK_POLLING;
}

}  // namespace

N64Thumbstick::N64Thumbstick()
    : _callback(nullptr),
      _pinXa(0),
      _pinXb(0),
      _pinYa(0),
      _pinYb(0),
      _mode(N64_THUMBSTICK_POLLING),
      _slot(-1),
      _n64Clamp(static_cast<int8_t>(N64_THUMBSTICK_DEFAULT_CLAMP)),
      _unitsPerEdge(N64_THUMBSTICK_DEFAULT_UNITS_PER_EDGE),
      _offsetX(0),
      _offsetY(0),
      _deadzone(0),
      _directionThreshold(N64_THUMBSTICK_DEFAULT_DIRECTION_THRESHOLD),
      _fullScaleX(N64_THUMBSTICK_DEFAULT_FULL_SCALE),
      _fullScaleY(N64_THUMBSTICK_DEFAULT_FULL_SCALE),
      _lastX(0),
      _lastY(0),
      _calMinX(0),
      _calMaxX(0),
      _calMinY(0),
      _calMaxY(0),
      _begun(false),
      _pullups(false),
      _invertX(false),
      _invertY(false),
      _swap(false),
      _baselineSet(false),
      _inCallback(false),
      _calibrating(false),
      _haveCalSample(false) {}

N64Thumbstick::~N64Thumbstick() {
  end();
}

bool N64Thumbstick::begin(uint8_t pinXa, uint8_t pinXb, uint8_t pinYa,
                          uint8_t pinYb, N64ThumbstickMode mode) {
  end();

  if (!knownMode(static_cast<uint8_t>(mode)) ||
      !pinsAreValid(pinXa, pinXb, pinYa, pinYb)) {
    return false;
  }

  const int8_t slot = claimSlot();
  if (slot < 0) {
    return false;
  }
  _slot = slot;
  _pinXa = pinXa;
  _pinXb = pinXb;
  _pinYa = pinYa;
  _pinYb = pinYb;
  _mode = static_cast<uint8_t>(mode);

  if (pinsConflict()) {
    dropSlot();
    return false;
  }

  const uint8_t inputMode = _pullups ? INPUT_PULLUP : INPUT;
  _axisX.begin(pinXa, pinXb, inputMode);
  _axisY.begin(pinYa, pinYb, inputMode);
  _offsetX = 0;
  _offsetY = 0;
  _baselineSet = false;
  _calibrating = false;
  _haveCalSample = false;
  // Edges that arrive while attachInterrupt() is running should count.
  _begun = true;

  if (_mode == N64_THUMBSTICK_INTERRUPT && !attachInterrupts()) {
    _begun = false;
    _axisX.end();
    _axisY.end();
    dropSlot();
    return false;
  }

  return true;
}

void N64Thumbstick::update() {
  if (!_begun) {
    return;
  }

  if (_mode == N64_THUMBSTICK_POLLING) {
    _axisX.poll();
    _axisY.poll();
  }

  long xValue = 0;
  long yValue = 0;
  read(xValue, yValue);

  if (!_baselineSet) {
    _lastX = xValue;
    _lastY = yValue;
    _baselineSet = true;
    return;
  }

  if (xValue == _lastX && yValue == _lastY) {
    return;
  }

  _lastX = xValue;
  _lastY = yValue;
  if (_callback == nullptr || _inCallback) {
    return;
  }

  _inCallback = true;
  _callback(xValue, yValue);
  _inCallback = false;
}

void N64Thumbstick::end() {
  const bool wasBegun = _begun;
  const uint8_t mode = _mode;

  noInterrupts();
  _begun = false;
  if (_slot >= 0 && _slots[_slot] == this) {
    _slots[_slot] = nullptr;
  }
  _slot = -1;
  interrupts();

  if (!wasBegun) {
    return;
  }

  if (mode == N64_THUMBSTICK_INTERRUPT) {
    detachInterrupts();
  }
  _axisX.end();
  _axisY.end();
}

long N64Thumbstick::x() const {
  return shape(logicalX());
}

long N64Thumbstick::y() const {
  return shape(logicalY());
}

void N64Thumbstick::read(long& xOut, long& yOut) const {
  long hardwareX = 0;
  long hardwareY = 0;
  snapshot(hardwareX, hardwareY);
  xOut = shape(preX(hardwareX, hardwareY) - _offsetX);
  yOut = shape(preY(hardwareX, hardwareY) - _offsetY);
}

long N64Thumbstick::xRaw() const {
  return _axisX.count();
}

long N64Thumbstick::yRaw() const {
  return _axisY.count();
}

int8_t N64Thumbstick::xN64() const {
  return toN64(x());
}

int8_t N64Thumbstick::yN64() const {
  return toN64(y());
}

float N64Thumbstick::xNormalized() const {
  return normalize(x(), _fullScaleX);
}

float N64Thumbstick::yNormalized() const {
  return normalize(y(), _fullScaleY);
}

float N64Thumbstick::xVelocity() const {
  const float rate =
      _swap ? _axisY.countsPerSecond() : _axisX.countsPerSecond();
  return _invertX ? -rate : rate;
}

float N64Thumbstick::yVelocity() const {
  const float rate =
      _swap ? _axisX.countsPerSecond() : _axisY.countsPerSecond();
  return _invertY ? -rate : rate;
}

bool N64Thumbstick::left() const {
  return x() <= -_directionThreshold;
}

bool N64Thumbstick::right() const {
  return x() >= _directionThreshold;
}

bool N64Thumbstick::up() const {
  return y() >= _directionThreshold;
}

bool N64Thumbstick::down() const {
  return y() <= -_directionThreshold;
}

void N64Thumbstick::zero() {
  // Hold off edges until both offsets match the same sample, or a count
  // that arrives between the two writes would leave a 1-count residual.
  noInterrupts();
  const long hardwareX = _axisX.peek();
  const long hardwareY = _axisY.peek();
  _offsetX = preX(hardwareX, hardwareY);
  _offsetY = preY(hardwareX, hardwareY);
  interrupts();
}

void N64Thumbstick::setOrigin(long x, long y) {
  _offsetX = x;
  _offsetY = y;
}

void N64Thumbstick::invertX(bool invert) {
  _invertX = invert;
}

void N64Thumbstick::invertY(bool invert) {
  _invertY = invert;
}

void N64Thumbstick::swapAxes(bool swap) {
  _swap = swap;
}

void N64Thumbstick::setDeadzone(long counts) {
  _deadzone = magnitude(counts);
}

long N64Thumbstick::deadzone() const {
  return _deadzone;
}

void N64Thumbstick::setDirectionThreshold(long counts) {
  counts = magnitude(counts);
  _directionThreshold = (counts < 1) ? 1 : counts;
}

long N64Thumbstick::directionThreshold() const {
  return _directionThreshold;
}

void N64Thumbstick::setUnitsPerEdge(int units) {
  if (units < 1) {
    return;
  }
  if (units > 127) {
    units = 127;
  }
  _unitsPerEdge = units;
}

int N64Thumbstick::unitsPerEdge() const {
  return _unitsPerEdge;
}

void N64Thumbstick::setN64Clamp(int limit) {
  if (limit < 1) {
    limit = 1;
  }
  if (limit > 127) {
    limit = 127;
  }
  _n64Clamp = static_cast<int8_t>(limit);
}

int N64Thumbstick::n64Clamp() const {
  return _n64Clamp;
}

void N64Thumbstick::setFullScale(long counts) {
  setFullScale(counts, counts);
}

void N64Thumbstick::setFullScale(long countsX, long countsY) {
  if (countsX > 0) {
    _fullScaleX = countsX;
  }
  if (countsY > 0) {
    _fullScaleY = countsY;
  }
}

long N64Thumbstick::fullScaleX() const {
  return _fullScaleX;
}

long N64Thumbstick::fullScaleY() const {
  return _fullScaleY;
}

void N64Thumbstick::calibrateBegin() {
  _calibrating = true;
  _haveCalSample = false;
}

void N64Thumbstick::calibrateUpdate() {
  if (!_begun || !_calibrating) {
    return;
  }

  const long xValue = logicalX();
  const long yValue = logicalY();
  if (!_haveCalSample) {
    _calMinX = xValue;
    _calMaxX = xValue;
    _calMinY = yValue;
    _calMaxY = yValue;
    _haveCalSample = true;
    return;
  }

  if (xValue < _calMinX) {
    _calMinX = xValue;
  }
  if (xValue > _calMaxX) {
    _calMaxX = xValue;
  }
  if (yValue < _calMinY) {
    _calMinY = yValue;
  }
  if (yValue > _calMaxY) {
    _calMaxY = yValue;
  }
}

bool N64Thumbstick::calibrateEnd() {
  _calibrating = false;
  if (!_haveCalSample) {
    return false;
  }

  const long spanX = magnitude(_calMinX) > magnitude(_calMaxX)
                         ? magnitude(_calMinX)
                         : magnitude(_calMaxX);
  const long spanY = magnitude(_calMinY) > magnitude(_calMaxY)
                         ? magnitude(_calMinY)
                         : magnitude(_calMaxY);
  bool learned = false;
  if (spanX > 0) {
    _fullScaleX = spanX;
    learned = true;
  }
  if (spanY > 0) {
    _fullScaleY = spanY;
    learned = true;
  }
  return learned;
}

void N64Thumbstick::onChange(N64ThumbstickCallback callback) {
  _callback = callback;
}

void N64Thumbstick::usePullups(bool enabled) {
  _pullups = enabled;
  if (_begun) {
    applyPinModes();
  }
}

const char* N64Thumbstick::architecture() {
  return N64ThumbstickInternal::architectureName();
}

bool N64Thumbstick::limitedInterruptPins() {
  return N64ThumbstickInternal::limitedExternalInterrupts();
}

bool N64Thumbstick::pinSupportsInterrupt(uint8_t pin) {
#if defined(digitalPinToInterrupt) && defined(NOT_AN_INTERRUPT)
  return digitalPinToInterrupt(pin) != NOT_AN_INTERRUPT;
#else
  (void)pin;
  return false;
#endif
}

int8_t N64Thumbstick::claimSlot() {
  for (uint8_t i = 0; i < N64_THUMBSTICK_MAX_INSTANCES; i++) {
    if (_slots[i] == nullptr) {
      _slots[i] = this;
      return static_cast<int8_t>(i);
    }
  }
  return -1;
}

void N64Thumbstick::dropSlot() {
  noInterrupts();
  if (_slot >= 0 && _slots[_slot] == this) {
    _slots[_slot] = nullptr;
  }
  _slot = -1;
  interrupts();
}

bool N64Thumbstick::pinsAreValid(uint8_t pinXa, uint8_t pinXb, uint8_t pinYa,
                                 uint8_t pinYb) const {
  const uint8_t pins[4] = {pinXa, pinXb, pinYa, pinYb};
  for (uint8_t i = 0; i < 4; i++) {
#if defined(NUM_DIGITAL_PINS)
    if (pins[i] >= NUM_DIGITAL_PINS) {
      return false;
    }
#endif
    for (uint8_t j = static_cast<uint8_t>(i + 1); j < 4; j++) {
      if (pins[i] == pins[j]) {
        return false;
      }
    }
  }
  return true;
}

bool N64Thumbstick::pinsConflict() const {
  const uint8_t mine[4] = {_pinXa, _pinXb, _pinYa, _pinYb};
  for (uint8_t i = 0; i < N64_THUMBSTICK_MAX_INSTANCES; i++) {
    N64Thumbstick* other = _slots[i];
    if (other == nullptr || other == this) {
      continue;
    }
    const uint8_t theirs[4] = {other->_pinXa, other->_pinXb, other->_pinYa,
                               other->_pinYb};
    for (uint8_t a = 0; a < 4; a++) {
      for (uint8_t b = 0; b < 4; b++) {
        if (mine[a] == theirs[b]) {
          return true;
        }
      }
    }
  }
  return false;
}

bool N64Thumbstick::attachInterrupts() {
#if defined(digitalPinToInterrupt) && defined(NOT_AN_INTERRUPT)
  const int irqX = digitalPinToInterrupt(_pinXa);
  const int irqY = digitalPinToInterrupt(_pinYa);
  if (irqX == NOT_AN_INTERRUPT || irqY == NOT_AN_INTERRUPT || _slot < 0) {
    return false;
  }

  void (*isrX)() = isrX0;
  void (*isrY)() = isrY0;
  switch (_slot) {
    case 0:
      isrX = isrX0;
      isrY = isrY0;
      break;
    case 1:
      isrX = isrX1;
      isrY = isrY1;
      break;
    case 2:
      isrX = isrX2;
      isrY = isrY2;
      break;
    default:
      isrX = isrX3;
      isrY = isrY3;
      break;
  }

  attachInterrupt(irqX, isrX, CHANGE);
  attachInterrupt(irqY, isrY, CHANGE);
  return true;
#else
  return false;
#endif
}

void N64Thumbstick::detachInterrupts() {
#if defined(digitalPinToInterrupt)
  detachInterrupt(digitalPinToInterrupt(_pinXa));
  detachInterrupt(digitalPinToInterrupt(_pinYa));
#endif
}

void N64Thumbstick::applyPinModes() {
  const uint8_t inputMode = _pullups ? INPUT_PULLUP : INPUT;
  pinMode(_pinXa, inputMode);
  pinMode(_pinXb, inputMode);
  pinMode(_pinYa, inputMode);
  pinMode(_pinYb, inputMode);
}

void N64Thumbstick::snapshot(long& hardwareX, long& hardwareY) const {
  noInterrupts();
  hardwareX = _axisX.peek();
  hardwareY = _axisY.peek();
  interrupts();
}

long N64Thumbstick::preX(long hardwareX, long hardwareY) const {
  const long hardware = _swap ? hardwareY : hardwareX;
  return _invertX ? -hardware : hardware;
}

long N64Thumbstick::preY(long hardwareX, long hardwareY) const {
  const long hardware = _swap ? hardwareX : hardwareY;
  return _invertY ? -hardware : hardware;
}

long N64Thumbstick::logicalX() const {
  long hardwareX = 0;
  long hardwareY = 0;
  snapshot(hardwareX, hardwareY);
  return preX(hardwareX, hardwareY) - _offsetX;
}

long N64Thumbstick::logicalY() const {
  long hardwareX = 0;
  long hardwareY = 0;
  snapshot(hardwareX, hardwareY);
  return preY(hardwareX, hardwareY) - _offsetY;
}

long N64Thumbstick::shape(long logical) const {
  if (_deadzone <= 0) {
    return logical;
  }
  if (logical > _deadzone) {
    return logical - _deadzone;
  }
  if (logical < -_deadzone) {
    return logical + _deadzone;
  }
  return 0;
}

int8_t N64Thumbstick::toN64(long shaped) const {
  long scaled = shaped * static_cast<long>(_unitsPerEdge);
  const long limit = _n64Clamp;
  if (scaled > limit) {
    scaled = limit;
  } else if (scaled < -limit) {
    scaled = -limit;
  }
  return static_cast<int8_t>(scaled);
}

float N64Thumbstick::normalize(long shaped, long fullScale) const {
  long span = fullScale - _deadzone;
  if (span < 1) {
    return 0.0f;
  }
  float value = static_cast<float>(shaped) / static_cast<float>(span);
  if (value > 1.0f) {
    return 1.0f;
  }
  if (value < -1.0f) {
    return -1.0f;
  }
  return value;
}

void N64Thumbstick::dispatch(uint8_t slot, uint8_t axis) {
  N64Thumbstick* stick = _slots[slot];
  if (stick == nullptr || !stick->_begun) {
    return;
  }
  if (axis == 0) {
    stick->_axisX.handleEdge();
  } else {
    stick->_axisY.handleEdge();
  }
}

void N64Thumbstick::isrX0() { dispatch(0, 0); }
void N64Thumbstick::isrY0() { dispatch(0, 1); }
void N64Thumbstick::isrX1() { dispatch(1, 0); }
void N64Thumbstick::isrY1() { dispatch(1, 1); }
void N64Thumbstick::isrX2() { dispatch(2, 0); }
void N64Thumbstick::isrY2() { dispatch(2, 1); }
void N64Thumbstick::isrX3() { dispatch(3, 0); }
void N64Thumbstick::isrY3() { dispatch(3, 1); }
