; The IDE preprocesses every sketch file as C++ while it looks for libraries.
; That pass does not define __ASSEMBLER__, and this file is not C++.
; The clock modules are named .h, not .inc, because the sketch copier
; only brings .c, .h, and .S into the build directory.
#if defined(__ASSEMBLER__)
#include "vusb_tiny_target.h"
#if defined(N64_VUSB_TINY)
#include "usbdrv/usbdrvasm.S"
#endif
#endif
