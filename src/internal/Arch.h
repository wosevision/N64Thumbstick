/*
 * N64Thumbstick
 * Copyright (C) 2026 Jackson Teather <jacksonteather@hotmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Internal. Sketches should include <N64Thumbstick.h>.
 *
 * Architecture names come from the ARDUINO_ARCH_* macros the Arduino build
 * defines from the board platform's architecture folder. The #else branch is
 * the portable fallback: this library still builds there, and polling mode
 * does not need an external interrupt.
 */

#ifndef N64_THUMBSTICK_INTERNAL_ARCH_H
#define N64_THUMBSTICK_INTERNAL_ARCH_H

namespace N64ThumbstickInternal {

inline const char* architectureName() {
#if defined(ARDUINO_ARCH_AVR)
  return "avr";
#elif defined(ARDUINO_ARCH_MEGAAVR)
  return "megaavr";
#elif defined(ARDUINO_ARCH_SAM)
  return "sam";
#elif defined(ARDUINO_ARCH_SAMD)
  return "samd";
#elif defined(ARDUINO_ARCH_ESP8266)
  return "esp8266";
#elif defined(ARDUINO_ARCH_ESP32)
  return "esp32";
#elif defined(ARDUINO_ARCH_RP2040)
  return "rp2040";
#elif defined(ARDUINO_ARCH_RENESAS)
  return "renesas";
#elif defined(ARDUINO_ARCH_STM32)
  return "stm32";
#elif defined(ARDUINO_ARCH_NRF52)
  return "nrf52";
#elif defined(ARDUINO_ARCH_MBED)
  return "mbed";
#elif defined(ARDUINO_ARCH_ZEPHYR)
  return "zephyr";
#else
  return "generic";
#endif
}

// Hint only. pinSupportsInterrupt() is the check that matches the core.
// Classic AVR and the SAM Due have external interrupts on specific pins.
// Most other current cores can attach an interrupt to any digital pin,
// sometimes with a small cap on how many are attached at once.
inline bool limitedExternalInterrupts() {
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_SAM)
  return true;
#elif defined(ARDUINO_ARCH_MEGAAVR) || defined(ARDUINO_ARCH_SAMD) || \
    defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) || \
    defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RENESAS) || \
    defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_MBED) || \
    defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_ZEPHYR)
  return false;
#else
  return true;
#endif
}

}  // namespace N64ThumbstickInternal

#endif
