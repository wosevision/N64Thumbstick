# How this library follows the Arduino library specification

This repository is meant to be read next to the [Arduino library specification, 1.5 format rev. 2.2](https://arduino.github.io/arduino-cli/1.5/library-specification/). Each section below is a rule from that document and the file here that satisfies it.

The IDE does not compile anything in `extras/`. This file is documentation only.

## 1.5 source layout, not the 1.0 layout

Libraries that only need IDE 1.5 and newer put sources in `src/`. The root is not scanned for `.cpp`, and a nested folder under `src/` is compiled. The pre-1.5 layout (headers in the library root, private code in `utility/`) is the other legal shape. This library uses `src/` because the internal decoder is a subfolder and the 1.0 layout does not compile subfolders recursively.

```
N64Thumbstick/
  library.properties
  keywords.txt
  LICENSE
  README.md
  src/N64Thumbstick.h          public header, the one sketches include
  src/N64Thumbstick.cpp
  src/internal/Arch.h          not offered by Include Library
  src/internal/AxisDecoder.h
  src/internal/AxisDecoder.cpp
  examples/...
  extras/...
```

Only `src/` is on the include path. `N64Thumbstick.cpp` includes `"internal/AxisDecoder.h"`, which resolves under `src/`. Sketch > Include Library adds the headers that sit directly in `src/`, and only those. `includes=N64Thumbstick.h` in `library.properties` makes that menu add one line:

```cpp
#include <N64Thumbstick.h>
```

Without `includes`, the IDE would add every `.h` in the root of `src/`. There is only one today. The property still documents the intended include, and it stays correct if another public header is added later.

`src/internal/` is the spec's rule that headers you do not promise to keep stable go in a subfolder. `AxisDecoder` is an implementation detail. Friend access lets `N64Thumbstick` call it without putting those methods on the public header.

## library.properties

The file is in the library root. Values are a single line each, because this format is not a list of paragraphs.

| Field | Value | Why |
| --- | --- | --- |
| `name` | `N64Thumbstick` | Letters and digits, starts with a letter, matches the folder name. Names starting with `Arduino` are reserved for official libraries. |
| `version` | `1.0.0` | Semver. `1.2` would be accepted. `r5` and `1.1c` would not. |
| `author` | Jackson Teather and email | The spec's author field. One person here. Several authors are separated by commas. |
| `maintainer` | same | Required, and it is a single maintainer. |
| `sentence` | first sentence of the Library Manager blurb | One sentence. |
| `paragraph` | the following sentences | Library Manager prints `sentence`, then `paragraph`. The paragraph does not repeat the sentence. |
| `category` | `Sensors` | One of the allowed categories. The stick is a sensor. |
| `url` | the GitHub repository | The "More info" link. This is the library's project, not the hardware wiki. |
| `architectures` | `*` | No board is excluded. The code uses the Arduino core API (`attachInterrupt`, `digitalRead`, `micros`). |
| `includes` | `N64Thumbstick.h` | Optional. See above. |
| `dot_a_linkage` | `true` | Optional. Sources are archived into a `.a` and then linked, instead of passing every `.o` to the link line. |

### Fields this library does not set

`depends` is for libraries this library must compile against. There are none. A sketch that wants `Mouse.h` includes it itself (`examples/UsbMouse`). Putting `depends=Mouse` here would make Library Manager install Mouse for every user of the stick, including ones who only print counts.

Version constraints are syntax on `depends`, for example `depends=ArduinoHttpClient (>=1.0.0)`. They are unused here for the same reason. The operators (`=`, `>`, `>=`, `<`, `<=`, `!`, `&&`, `||`, and parentheses) are defined in the spec and are not repeated as a fake dependency.

`precompiled` is for shipping `.a` or `.so` files built for one MCU. This library is source. Turning `precompiled` on without those files buys nothing, and `precompiled=full` would skip the sources on any MCU that happened to find an archive. If a later release did ship binaries, the spec's layout would be:

```
src/cortex-m0plus/libN64Thumbstick.a
src/cortex-m4/libN64Thumbstick.a
src/cortex-m4/fpv4-sp-d16-softfp/libN64Thumbstick.a
```

The archive name starts with `lib`. The extra directory is `{build.fpu}-{build.float-abi}` when the core uses a hardware floating-point ABI. `src/{mcu}/` is the fallback when that directory is absent. None of those directories exist here.

`ldflags` would add linker flags such as `-lm`. The float math in this library is covered by the core's normal link line. An extra flag is not set.

The specification's property list has no `license` key. The license is `LICENSE` and the header comment in the sources.

## Keywords

`keywords.txt` is in the library root. Fields are separated by one tab. Spaces do not work. The spec allows four fields:

```
KEYWORD <tab> KEYWORD_TOKENTYPE <tab> REFERENCE_LINK <tab> RSYNTAXTEXTAREA_TOKENTYPE
```

| Token type | Used for |
| --- | --- |
| `KEYWORD1` | types: `N64Thumbstick`, `N64ThumbstickMode`, `N64ThumbstickCallback` |
| `KEYWORD2` | methods |
| `KEYWORD3` | structures. Not used. |
| `LITERAL1` | constants such as `N64_THUMBSTICK_POLLING` |
| `LITERAL2` | Not used. |

`REFERENCE_LINK` would open an Arduino Language Reference page. Third-party names are not in that reference, so the field is empty.

`RSYNTAXTEXTAREA_TOKENTYPE` overrides the second field on IDE 1.6.5 and newer. The datatype lines set it to `DATA_TYPE`, which is the override that matches `KEYWORD1`, so the four-field line is present in the file:

```
N64Thumbstick <tab> KEYWORD1 <tab> <tab> DATA_TYPE
```

The method and constant lines use the two-field form. Both are legal. Empty fields may be omitted from the right.

`keywords.txt` is global. Installing the library highlights those words in every sketch, not only sketches that include the header. `x`, `y`, `read`, `left`, `right`, `up`, and `down` are real methods and are left out of the file so they are not painted everywhere. The comments at the top of `keywords.txt` say this.

## Examples

The folder is `examples`, lowercase. Each sketch is a directory whose name matches its `.ino` file, which is the sketch specification's primary-file rule. The IDE lists them under File > Examples > N64Thumbstick.

The sketches are public domain. The library is LGPL 2.1 or later. Arduino's example guidance asks for examples people can copy without inheriting the library license, and for each sketch to stand alone: circuit, what it does, and a `setup`/`loop` that compiles. That is why the wiring comment is repeated instead of pointing at another sketch. `examples/UsbMouse/usbdrv/` is the exception: it is Objective Development's V-USB under GPL version 2 or 3, with `License.txt` beside the sources. The sketch file around it stays public domain. Building that example for an ATtiny84 or ATtiny85 links the two, and the resulting firmware is GPL.

`examples/UsbMouse` calls `Mouse.move` only when `N64_EXAMPLE_USE_MOUSE` is 1 and `USBCON` is defined. `USBCON` is the 32U4 USB register macro, so it is present on a Leonardo or Micro and absent on an Uno. The flag defaults to 0, which keeps the `#include <Mouse.h>` out of the build: the IDE discovers libraries from includes the preprocessor actually reaches, so a probe such as `__has_include` never causes Mouse to be linked. Set the flag after installing Mouse. On an ATtiny84 or ATtiny85 the same sketch includes V-USB instead. `vusb_tiny_target.h` matches the x4 and x5 device macros, and `vusb_tiny.c` and `vusb_tiny.S` compile the driver only then, so an Uno build does not assemble the bit-bang driver. `usbconfig.h` is where the two chips diverge: port and pin-change registers, and the clock each one can actually run. `examples/ServoFollow` pulses the servo pin itself so the sketch does not depend on the Servo library, whose `architectures` list is narrower than `*`.

## extras and the development flag

`extras/` is bundled and ignored by the builder. It holds this note, the wiring note, the quadrature note, and `I2cStickClient.ino`, a one-page master for the I2cStick device written as plain functions. The spec's warning is that a large binary here inflates every install. These are short text files.

`.development` in the library root, next to `library.properties`, makes the IDE treat examples as editable (IDE 1.6.6 and newer). The Library Manager indexer skips any release that still contains that file. `.gitignore` excludes it so a normal commit does not publish it. Delete the working copy as well before cutting a release zip.

## More than one architecture

The spec's mechanism is the `ARDUINO_ARCH_*` macro, one macro per platform architecture folder. `src/internal/Arch.h` uses the `#if / #elif / #else` chain from that section.

There is no `#error` branch. The spec's `#error` form is for a library that cannot run on other boards. `architectures=*` means the opposite: a board this file does not name still compiles, `architecture()` returns `"generic"`, and polling mode only needs `digitalRead`. `limitedInterruptPins()` is a hint. `pinSupportsInterrupt()` asks the core, and `begin()` fails cleanly when an interrupt pin does not exist, so the sketch can retry in polling mode. `examples/Diagnostics` does that.

`dot_a_linkage=true` interacts with interrupts. The linker keeps a `.o` from the `.a` only when some symbol in it is referenced. The `CHANGE` handlers are in `src/N64Thumbstick.cpp`, which `begin()` references, so they are not dropped. An interrupt vector in a `.cpp` that nothing calls would link away, and the stick would silently stop counting. That is why the handlers are not in their own file.

## What the builder sees

Compiling any sketch under `examples/` for a board whose architecture is `*` builds `src/N64Thumbstick.cpp` and `src/internal/AxisDecoder.cpp`, archives them, and links the archive. `extras/`, `examples/` other than the open sketch, `keywords.txt`, and `library.properties` are not compiled.
