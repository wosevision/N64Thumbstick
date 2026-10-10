/*
 * V-USB configuration for the ATtiny x4 and x5 builds of UsbMouse.
 *
 * Both chips sample D+ with a pin-change interrupt. The x4 has that
 * interrupt per port (PCMSK0). The x5 has one for the whole chip (PCMSK).
 *
 * x4: D+ PA0, D- PA1. The crystal keeps PB0 and PB1, and RESET stays ISP.
 * x5: D+ PB4, D- PB3, the Digispark pair. The PLL frees those pins. The
 *     stick then needs PB5, which is reset until RSTDISBL is programmed.
 *
 * Objective Development's shared mouse id is 0x16c0/0x27da. The serial
 * number is omitted because only the operating system's HID driver is
 * used. See usbdrv/USB-IDs-for-free.txt.
 */

#ifndef __usbconfig_h_included__
#define __usbconfig_h_included__

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#if (F_CPU / 1000) != 16500 || CLOCK_SOURCE != 6
#error "On the ATtiny85 use Tools > Clock > 16.5 MHz (PLL, tweaked). A crystal uses PB3 and PB4, which are D- and D+."
#endif
#define USB_CFG_IOPORTNAME B
#define USB_CFG_DMINUS_BIT 3
#define USB_CFG_DPLUS_BIT 4
#define USB_INTR_CFG PCMSK
#define USB_INTR_ENABLE_BIT PCIE
#define USB_INTR_PENDING_BIT PCIF
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#if (F_CPU / 1000) != 12000 && (F_CPU / 1000) != 12800 && \
    (F_CPU / 1000) != 15000 && (F_CPU / 1000) != 16000 && \
    (F_CPU / 1000) != 16500 && (F_CPU / 1000) != 18000 && (F_CPU / 1000) != 20000
#error "On the ATtiny84 use Tools > Clock > 16 MHz (external) and a crystal on PB0 and PB1. 12 and 20 MHz crystals also time V-USB."
#endif
#define USB_CFG_IOPORTNAME A
#define USB_CFG_DMINUS_BIT 1
#define USB_CFG_DPLUS_BIT 0
#define USB_INTR_CFG PCMSK0
#define USB_INTR_ENABLE_BIT PCIE0
#define USB_INTR_PENDING_BIT PCIF0
#else
#error "UsbMouse V-USB targets the ATtiny x4 and x5."
#endif

#define USB_CFG_CLOCK_KHZ (F_CPU / 1000)
#define USB_CFG_CHECK_CRC 0

#define USB_CFG_HAVE_INTRIN_ENDPOINT 1
#define USB_CFG_HAVE_INTRIN_ENDPOINT3 0
#define USB_CFG_EP3_NUMBER 3
#define USB_CFG_IMPLEMENT_HALT 0
#define USB_CFG_SUPPRESS_INTR_CODE 0
#define USB_CFG_INTR_POLL_INTERVAL 10
#define USB_CFG_IS_SELF_POWERED 0
#define USB_CFG_MAX_BUS_POWER 100
#define USB_CFG_IMPLEMENT_FN_WRITE 0
#define USB_CFG_IMPLEMENT_FN_READ 0
#define USB_CFG_IMPLEMENT_FN_WRITEOUT 0
#define USB_CFG_HAVE_FLOWCONTROL 0
#define USB_CFG_DRIVER_FLASH_PAGE 0
#define USB_CFG_LONG_TRANSFERS 0
#define USB_COUNT_SOF 0
#define USB_CFG_CHECK_DATA_TOGGLING 0
#define USB_CFG_HAVE_MEASURE_FRAME_LENGTH 0
#define USB_USE_FAST_CRC 0

#define USB_CFG_VENDOR_ID 0xc0, 0x16
#define USB_CFG_DEVICE_ID 0xda, 0x27
#define USB_CFG_DEVICE_VERSION 0x00, 0x01
#define USB_CFG_VENDOR_NAME \
  'g', 'i', 't', 'h', 'u', 'b', '.', 'c', 'o', 'm', '/', 'w', 'o', 's', 'e', 'v', 'i', 's', 'i', 'o', 'n'
#define USB_CFG_VENDOR_NAME_LEN 21
#define USB_CFG_DEVICE_NAME 'N', '6', '4', ' ', 'S', 't', 'i', 'c', 'k'
#define USB_CFG_DEVICE_NAME_LEN 9

#define USB_CFG_DEVICE_CLASS 0
#define USB_CFG_DEVICE_SUBCLASS 0
#define USB_CFG_INTERFACE_CLASS 3
#define USB_CFG_INTERFACE_SUBCLASS 0
#define USB_CFG_INTERFACE_PROTOCOL 0
#define USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH 52

#define USB_INTR_CFG_SET (1 << USB_CFG_DPLUS_BIT)
#define USB_INTR_CFG_CLR 0
#define USB_INTR_ENABLE GIMSK
#define USB_INTR_PENDING GIFR
#define USB_INTR_VECTOR PCINT0_vect

#endif
