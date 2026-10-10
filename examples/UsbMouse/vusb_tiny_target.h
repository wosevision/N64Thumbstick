/*
 * Set for the ATtiny x4 and x5. avr-gcc defines the device macro from
 * -mmcu. The x4/x5 family macros live in the core's pins header, which
 * this file does not include, so the driver checks the device macros.
 */
#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || \
    defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define N64_VUSB_TINY 1
#endif
