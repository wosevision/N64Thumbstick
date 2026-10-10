#ifndef VUSB_TINY_H
#define VUSB_TINY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void n64UsbMouseBegin(void);
void n64UsbMousePoll(void);
uint8_t n64UsbMouseMove(int8_t dx, int8_t dy);

#ifdef __cplusplus
}
#endif

#endif
