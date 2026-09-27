/*
  I2C Stick

  This board is an I2C device at address 0x32. A master writes a register
  address, ends the transmission, then reads. Live counts are refreshed
  every loop(), so a read returns the latest sample. A write is applied in
  loop(); wait about a millisecond after endTransmission() before reading
  a value that the write just changed.

  Numbers are little-endian. Floats are IEEE-754, the same format Arduino
  uses. One read returns at most 32 bytes from the selected register
  onward. On an ATtiny85 the USI transmit buffer is 16 bytes, so request
  16 or fewer and write a new register address for the next window. End
  the write before the read; a repeated start is not reliable on that USI
  slave.

  Register map (the readable image):

    0x00 int32  x()          \
    0x04 int32  y()           read() as one pair
    0x08 int32  xRaw()
    0x0C int32  yRaw()
    0x10 int8   xN64()
    0x11 int8   yN64()
    0x12 uint8  left()
    0x13 uint8  right()
    0x14 uint8  up()
    0x15 uint8  down()
    0x16 uint8  onChange latch, 1 after a move; write 0 to clear
    0x17 uint8  last calibrateEnd(), 0xFF until the first call
    0x18 float  xNormalized()
    0x1C float  yNormalized()
    0x20 float  xVelocity()
    0x24 float  yVelocity()
    0x28 int32  deadzone()                 write: setDeadzone()
    0x2C int32  directionThreshold()       write: setDirectionThreshold()
    0x30 int32  fullScaleX()               write: setFullScale(x, current y)
    0x34 int32  fullScaleY()               write: setFullScale(current x, y)
    0x38 int16  unitsPerEdge()             write: setUnitsPerEdge()
    0x3A int8   n64Clamp()                 write: setN64Clamp()
    0x3B uint8  limitedInterruptPins()
    0x3C uint8  flags                      write replaces all four
               bit0 invertX, bit1 invertY, bit2 swapAxes, bit3 usePullups
    0x3D uint8  1 after a successful begin()
    0x3E uint8  result of the last pinSupportsInterrupt query
    0x40 char[8] architecture(), NUL padded

  Write-only commands. The byte after the register is the first argument.

    0x50 zero()
    0x51 setOrigin(int32 x, int32 y)
    0x52 setFullScale(int32 both)
    0x53 setFullScale(int32 x, int32 y)
    0x54 begin(uint8 xa, xb, ya, yb, mode)
         mode 0 is interrupt, 1 is polling
    0x55 end()
    0x56 update()
    0x57 calibrateBegin()
    0x58 calibrateUpdate()
    0x59 calibrateEnd()
    0x5A usePullups(uint8)
    0x5B invertX(uint8)
    0x5C invertY(uint8)
    0x5D swapAxes(uint8)
    0x5E pinSupportsInterrupt: write the pin, then read register 0x3E

  loop() calls update() every pass, and calibrateUpdate() between
  calibrateBegin() and calibrateEnd().

  Master, reading x and y:

    Wire.beginTransmission(0x32);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.requestFrom(0x32, 8);

  Circuit:
  - Stick VCC to 3.3 V. Do not use 5 V.
  - Stick GND to GND, common with the I2C master
  - XA to pin 2, XB to pin 4, YA to pin 3, YB to pin 5
  - SDA and SCL to the master, with pull-ups to the bus voltage
    (Uno: A4 is SDA, A5 is SCL)
  - Leonardo and Micro: pins 2 and 3 are SDA and SCL, so this sketch
    polls the stick on pins 8, 9, 10, and 11 instead
  - ATtiny85 (ATTinyCore): USI I2C is fixed, SDA on pin 0 (PB0) and SCL
    on pin 2 (PB2). Pin 2 is also the only external interrupt, so the
    stick is polled: XA pin 1, XB pin 3, YA pin 4, YB pin 5. Pin 5 is
    reset until RSTDISBL is programmed (high fuse bit 7 cleared). That
    board menu leaves the bit set, and clearing it disables ISP until a
    high-voltage programmer restores reset. Run the chip at 8 MHz or
    faster from 3.3 V. The USI driver enables the SDA and SCL pull-ups,
    and 1 MHz cannot keep a 100 kHz bus. Leave LTO enabled.

  created 27 Sep 2026
  by Jackson Teather

  This example code is in the public domain.

  https://github.com/wosevision/N64Thumbstick
*/

#include <N64Thumbstick.h>
#include <Wire.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

const uint8_t I2C_ADDRESS = 0x32;

#if defined(__AVR_ATtinyX5__)
const uint8_t PIN_XA = 1;
const uint8_t PIN_XB = 3;
const uint8_t PIN_YA = 4;
const uint8_t PIN_YB = 5;
const N64ThumbstickMode STICK_MODE = N64_THUMBSTICK_POLLING;
#elif defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_MICRO) || \
    defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_ESPLORA)
const uint8_t PIN_XA = 8;
const uint8_t PIN_XB = 9;
const uint8_t PIN_YA = 10;
const uint8_t PIN_YB = 11;
const N64ThumbstickMode STICK_MODE = N64_THUMBSTICK_POLLING;
#else
const uint8_t PIN_XA = 2;
const uint8_t PIN_XB = 4;
const uint8_t PIN_YA = 3;
const uint8_t PIN_YB = 5;
const N64ThumbstickMode STICK_MODE = N64_THUMBSTICK_INTERRUPT;
#endif

const uint8_t REG_CHANGED = 0x16;
const uint8_t REG_DEADZONE = 0x28;
const uint8_t REG_DIRECTION = 0x2C;
const uint8_t REG_FULL_X = 0x30;
const uint8_t REG_FULL_Y = 0x34;
const uint8_t REG_UNITS = 0x38;
const uint8_t REG_CLAMP = 0x3A;
const uint8_t REG_FLAGS = 0x3C;
const uint8_t REG_PIN_RESULT = 0x3E;

const uint8_t CMD_ZERO = 0x50;
const uint8_t CMD_SET_ORIGIN = 0x51;
const uint8_t CMD_FULL_BOTH = 0x52;
const uint8_t CMD_FULL_PAIR = 0x53;
const uint8_t CMD_BEGIN = 0x54;
const uint8_t CMD_END = 0x55;
const uint8_t CMD_UPDATE = 0x56;
const uint8_t CMD_CAL_BEGIN = 0x57;
const uint8_t CMD_CAL_UPDATE = 0x58;
const uint8_t CMD_CAL_END = 0x59;
const uint8_t CMD_PULLUPS = 0x5A;
const uint8_t CMD_INVERT_X = 0x5B;
const uint8_t CMD_INVERT_Y = 0x5C;
const uint8_t CMD_SWAP = 0x5D;
const uint8_t CMD_PIN_IRQ = 0x5E;

const uint8_t FLAG_INVERT_X = 0x01;
const uint8_t FLAG_INVERT_Y = 0x02;
const uint8_t FLAG_SWAP = 0x04;
const uint8_t FLAG_PULLUPS = 0x08;

struct Image {
  int32_t x;
  int32_t y;
  int32_t xRaw;
  int32_t yRaw;
  int8_t xN64;
  int8_t yN64;
  uint8_t left;
  uint8_t right;
  uint8_t up;
  uint8_t down;
  uint8_t changed;
  uint8_t calibrateOk;
  float xNormalized;
  float yNormalized;
  float xVelocity;
  float yVelocity;
  int32_t deadzone;
  int32_t directionThreshold;
  int32_t fullScaleX;
  int32_t fullScaleY;
  int16_t unitsPerEdge;
  int8_t n64Clamp;
  uint8_t limitedInterrupts;
  uint8_t flags;
  uint8_t begun;
  uint8_t pinInterrupt;
  uint8_t reserved;
  char architecture[8];
};

static_assert(offsetof(Image, y) == 0x04, "y register");
static_assert(offsetof(Image, xN64) == 0x10, "xN64 register");
static_assert(offsetof(Image, changed) == 0x16, "onChange register");
static_assert(offsetof(Image, xNormalized) == 0x18, "xNormalized register");
static_assert(offsetof(Image, xVelocity) == 0x20, "xVelocity register");
static_assert(offsetof(Image, deadzone) == 0x28, "deadzone register");
static_assert(offsetof(Image, unitsPerEdge) == 0x38, "units register");
static_assert(offsetof(Image, architecture) == 0x40, "architecture register");
static_assert(sizeof(Image) <= 96, "image fits a small slave buffer");

N64Thumbstick stick;

Image image;
uint8_t published[sizeof(Image)];
volatile uint8_t regPointer = 0;
#if defined(__AVR_ATtinyX5__)
volatile uint8_t rxBuf[16];
#else
volatile uint8_t rxBuf[32];
#endif
volatile uint8_t rxLen = 0;
volatile uint8_t rxReady = 0;
volatile uint8_t stickChanged = 0;

bool calibrating = false;
bool shadowInvertX = false;
bool shadowInvertY = false;
bool shadowSwap = false;
bool shadowPullups = false;
uint8_t pinInterruptResult = 0;
uint8_t begunOk = 0;

int32_t readInt32(const uint8_t* bytes) {
  const uint32_t value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
                         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
  return (int32_t)value;
}

uint8_t flagByte() {
  uint8_t flags = 0;
  if (shadowInvertX) {
    flags |= FLAG_INVERT_X;
  }
  if (shadowInvertY) {
    flags |= FLAG_INVERT_Y;
  }
  if (shadowSwap) {
    flags |= FLAG_SWAP;
  }
  if (shadowPullups) {
    flags |= FLAG_PULLUPS;
  }
  return flags;
}

void applyFlags(uint8_t flags) {
  shadowInvertX = (flags & FLAG_INVERT_X) != 0;
  shadowInvertY = (flags & FLAG_INVERT_Y) != 0;
  shadowSwap = (flags & FLAG_SWAP) != 0;
  shadowPullups = (flags & FLAG_PULLUPS) != 0;
  stick.invertX(shadowInvertX);
  stick.invertY(shadowInvertY);
  stick.swapAxes(shadowSwap);
  stick.usePullups(shadowPullups);
}

void onStickChanged(long x, long y) {
  (void)x;
  (void)y;
  stickChanged = 1;
}

void fillImage() {
  long x = 0;
  long y = 0;
  stick.read(x, y);
  image.x = (int32_t)x;
  image.y = (int32_t)y;
  image.xRaw = (int32_t)stick.xRaw();
  image.yRaw = (int32_t)stick.yRaw();
  image.xN64 = stick.xN64();
  image.yN64 = stick.yN64();
  image.left = stick.left() ? 1 : 0;
  image.right = stick.right() ? 1 : 0;
  image.up = stick.up() ? 1 : 0;
  image.down = stick.down() ? 1 : 0;
  image.changed = stickChanged;
  image.xNormalized = stick.xNormalized();
  image.yNormalized = stick.yNormalized();
  image.xVelocity = stick.xVelocity();
  image.yVelocity = stick.yVelocity();
  image.deadzone = (int32_t)stick.deadzone();
  image.directionThreshold = (int32_t)stick.directionThreshold();
  image.fullScaleX = (int32_t)stick.fullScaleX();
  image.fullScaleY = (int32_t)stick.fullScaleY();
  image.unitsPerEdge = (int16_t)stick.unitsPerEdge();
  image.n64Clamp = (int8_t)stick.n64Clamp();
  image.limitedInterrupts = N64Thumbstick::limitedInterruptPins() ? 1 : 0;
  image.flags = flagByte();
  image.begun = begunOk;
  image.pinInterrupt = pinInterruptResult;

  memset(image.architecture, 0, sizeof(image.architecture));
  const char* name = N64Thumbstick::architecture();
  for (uint8_t i = 0; i < 7 && name[i] != '\0'; i++) {
    image.architecture[i] = name[i];
  }
}

void publishImage() {
  noInterrupts();
  memcpy(published, &image, sizeof(image));
  interrupts();
}

void handleCommand(const uint8_t* data, uint8_t length) {
  if (length < 1) {
    return;
  }
  const uint8_t reg = data[0];
  const uint8_t* payload = data + 1;
  const uint8_t count = (uint8_t)(length - 1);

  switch (reg) {
    case REG_CHANGED:
      if (count >= 1 && payload[0] == 0) {
        stickChanged = 0;
      }
      break;
    case REG_DEADZONE:
      if (count >= 4) {
        stick.setDeadzone(readInt32(payload));
      }
      break;
    case REG_DIRECTION:
      if (count >= 4) {
        stick.setDirectionThreshold(readInt32(payload));
      }
      break;
    case REG_FULL_X:
      if (count >= 4) {
        stick.setFullScale(readInt32(payload), stick.fullScaleY());
      }
      break;
    case REG_FULL_Y:
      if (count >= 4) {
        stick.setFullScale(stick.fullScaleX(), readInt32(payload));
      }
      break;
    case REG_UNITS:
      if (count >= 2) {
        const int units = (int)payload[0] | ((int)payload[1] << 8);
        stick.setUnitsPerEdge(units);
      }
      break;
    case REG_CLAMP:
      if (count >= 1) {
        stick.setN64Clamp(payload[0]);
      }
      break;
    case REG_FLAGS:
      if (count >= 1) {
        applyFlags(payload[0]);
      }
      break;
    case CMD_ZERO:
      stick.zero();
      break;
    case CMD_SET_ORIGIN:
      if (count >= 8) {
        stick.setOrigin(readInt32(payload), readInt32(payload + 4));
      }
      break;
    case CMD_FULL_BOTH:
      if (count >= 4) {
        stick.setFullScale(readInt32(payload));
      }
      break;
    case CMD_FULL_PAIR:
      if (count >= 8) {
        stick.setFullScale(readInt32(payload), readInt32(payload + 4));
      }
      break;
    case CMD_BEGIN:
      if (count >= 5) {
        const N64ThumbstickMode mode = (payload[4] == N64_THUMBSTICK_POLLING)
                                           ? N64_THUMBSTICK_POLLING
                                           : N64_THUMBSTICK_INTERRUPT;
        begunOk = stick.begin(payload[0], payload[1], payload[2], payload[3], mode) ? 1 : 0;
      }
      break;
    case CMD_END:
      stick.end();
      begunOk = 0;
      break;
    case CMD_UPDATE:
      stick.update();
      break;
    case CMD_CAL_BEGIN:
      stick.calibrateBegin();
      calibrating = true;
      break;
    case CMD_CAL_UPDATE:
      stick.calibrateUpdate();
      break;
    case CMD_CAL_END:
      image.calibrateOk = stick.calibrateEnd() ? 1 : 0;
      calibrating = false;
      break;
    case CMD_PULLUPS:
      if (count >= 1) {
        shadowPullups = payload[0] != 0;
        stick.usePullups(shadowPullups);
      }
      break;
    case CMD_INVERT_X:
      if (count >= 1) {
        shadowInvertX = payload[0] != 0;
        stick.invertX(shadowInvertX);
      }
      break;
    case CMD_INVERT_Y:
      if (count >= 1) {
        shadowInvertY = payload[0] != 0;
        stick.invertY(shadowInvertY);
      }
      break;
    case CMD_SWAP:
      if (count >= 1) {
        shadowSwap = payload[0] != 0;
        stick.swapAxes(shadowSwap);
      }
      break;
    case CMD_PIN_IRQ:
      if (count >= 1) {
        pinInterruptResult = N64Thumbstick::pinSupportsInterrupt(payload[0]) ? 1 : 0;
      }
      break;
    default:
      break;
  }
}

void serviceI2c() {
  if (!rxReady) {
    return;
  }
  uint8_t local[sizeof(rxBuf)];
  uint8_t length = 0;
  noInterrupts();
  length = rxLen;
  memcpy(local, (const void*)rxBuf, length);
  rxReady = 0;
  interrupts();
  handleCommand(local, length);
}

void receiveEvent(int howMany) {
  uint8_t length = 0;
  while (Wire.available() > 0 && length < sizeof(rxBuf)) {
    rxBuf[length] = (uint8_t)Wire.read();
    length++;
    (void)howMany;
  }
  if (length == 0) {
    return;
  }
  regPointer = rxBuf[0];
  rxLen = length;
  // One byte below the command block only moves the read pointer.
  // A command, or a register with a payload, is applied in loop().
  rxReady = (length == 1 && rxBuf[0] < CMD_ZERO) ? 0 : 1;
}

void requestEvent() {
  const uint8_t start = regPointer;
  if (start >= sizeof(published)) {
    return;
  }
  uint8_t count = (uint8_t)(sizeof(published) - start);
  uint8_t limit = 32;
#if defined(__AVR_ATtinyX5__)
  limit = 16;
#endif
  if (count > limit) {
    count = limit;
  }
  Wire.write(published + start, count);
}

void setup() {
#if !defined(__AVR_ATtinyX5__)
  Serial.begin(9600);
#if defined(USBCON)
  const unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 2000) {
  }
#endif
#endif

  memset(&image, 0, sizeof(image));
  image.calibrateOk = 0xFF;
  stick.onChange(onStickChanged);
  begunOk = stick.begin(PIN_XA, PIN_XB, PIN_YA, PIN_YB, STICK_MODE) ? 1 : 0;
  fillImage();
  publishImage();

  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

#if !defined(__AVR_ATtinyX5__)
  Serial.print(F("I2C stick at 0x"));
  Serial.println(I2C_ADDRESS, HEX);
  if (!begunOk) {
    Serial.println(F("begin() failed. A master can still send command 0x54."));
  }
#endif
}

void loop() {
  serviceI2c();
  if (calibrating) {
    stick.calibrateUpdate();
  }
  stick.update();
  fillImage();
  publishImage();
}
