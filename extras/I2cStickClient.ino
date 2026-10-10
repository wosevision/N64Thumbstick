/*
  I2C Stick client

  Plain functions for the I2cStick device at address 0x32. The IDE does
  not compile extras/. Copy this file into a sketch folder named
  I2cStickClient to run it.

  Numbers are little-endian. A read writes the register, stops, then
  requests. Ask for 16 bytes or fewer when the device is an ATtiny85 or
  ATtiny84.
  After a write, wait about a millisecond before reading that value.
  Send z in the Serial Monitor to call zero().

  Circuit: SDA, SCL, and GND in common with the I2cStick board.
  Pull SDA and SCL up to the bus voltage.

  created 27 Sep 2026
  by Jackson Teather

  This example code is in the public domain.

  https://github.com/wosevision/N64Thumbstick
*/

#include <Wire.h>

const uint8_t STICK_ADDRESS = 0x32;

bool stickRead(uint8_t reg, uint8_t* data, uint8_t count) {
  Wire.beginTransmission(STICK_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  if (Wire.requestFrom(STICK_ADDRESS, count) != count) {
    return false;
  }
  for (uint8_t i = 0; i < count; i++) {
    data[i] = (uint8_t)Wire.read();
  }
  return true;
}

bool stickWrite(uint8_t reg, const uint8_t* data, uint8_t count) {
  Wire.beginTransmission(STICK_ADDRESS);
  Wire.write(reg);
  for (uint8_t i = 0; i < count; i++) {
    Wire.write(data[i]);
  }
  return Wire.endTransmission() == 0;
}

int32_t stickI32(const uint8_t* data) {
  const uint32_t value = (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
                         ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
  return (int32_t)value;
}

bool stickPosition(int32_t& x, int32_t& y) {
  uint8_t data[8];
  if (!stickRead(0x00, data, 8)) {
    return false;
  }
  x = stickI32(data);
  y = stickI32(data + 4);
  return true;
}

bool stickZero() {
  return stickWrite(0x50, nullptr, 0);
}

void setup() {
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif
  Wire.begin();
}

void loop() {
  int32_t x = 0;
  int32_t y = 0;
  if (Serial.available() > 0 && Serial.read() == 'z') {
    stickZero();
  }
  if (stickPosition(x, y)) {
    Serial.print(x);
    Serial.print(' ');
    Serial.println(y);
  }
  delay(50);
}
