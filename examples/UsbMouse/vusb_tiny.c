/*
 * Virtual-USB mouse for the ATtiny x4 and x5. The driver under usbdrv/
 * is Objective Development's V-USB, GPL v2 or v3. This file is public
 * domain.
 */

#include <stdint.h>

#include "vusb_tiny_target.h"

#if defined(N64_VUSB_TINY)

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "usbdrv/usbdrv.c"

typedef struct {
  uint8_t buttonMask;
  int8_t dx;
  int8_t dy;
  int8_t dWheel;
} Report;

static Report reportBuffer;
static uint8_t idleRate;

PROGMEM const char usbHidReportDescriptor[52] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01, 0xa1, 0x00, 0x05, 0x09, 0x19,
    0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x05, 0x81, 0x03, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09,
    0x38, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06, 0xc0, 0xc0,
};

_Static_assert(sizeof(usbHidReportDescriptor) == 52, "HID report length");

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  usbRequest_t* rq = (void*)data;
  if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
    if (rq->bRequest == USBRQ_HID_GET_REPORT) {
      usbMsgPtr = (usbMsgPtr_t)&reportBuffer;
      return sizeof(reportBuffer);
    }
    if (rq->bRequest == USBRQ_HID_GET_IDLE) {
      usbMsgPtr = &idleRate;
      return 1;
    }
    if (rq->bRequest == USBRQ_HID_SET_IDLE) {
      idleRate = rq->wValue.bytes[1];
    }
  }
  return 0;
}

void n64UsbMouseBegin(void) {
  wdt_disable();
  cli();
  usbInit();
  usbDeviceDisconnect();
  uint8_t i = 0;
  while (--i) {
    _delay_ms(1);
  }
  usbDeviceConnect();
  sei();
}

void n64UsbMousePoll(void) {
  usbPoll();
}

uint8_t n64UsbMouseMove(int8_t dx, int8_t dy) {
  if (!usbInterruptIsReady()) {
    return 0;
  }
  reportBuffer.buttonMask = 0;
  reportBuffer.dx = dx;
  reportBuffer.dy = dy;
  reportBuffer.dWheel = 0;
  usbSetInterrupt((void*)&reportBuffer, sizeof(reportBuffer));
  return 1;
}

#endif
