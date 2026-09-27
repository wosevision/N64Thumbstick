# N64Thumbstick

Read a Nintendo 64 optical thumbstick module from an Arduino. This is the 6-pin stick only, not the buttons, the pak port, or the Joybus wire.

The module emits quadrature on four signals, `XA`/`XB` and `YA`/`YB`. Position is the running count of edges on `XA` and `YA`. It starts at 0 wherever the stick was when `begin()` ran. Power it at **3.3 V**.

Operation of the module is described in the [Analog Stick](https://n64brew.dev/wiki/Controller#Analog_Stick) section of the N64brew controller page.

## Install

The folder name must stay `N64Thumbstick`. Place it in the sketchbook `libraries` directory, or clone this repository there:

```
Arduino/libraries/N64Thumbstick/
```

Restart the IDE. Sketches include it with:

```cpp
#include <N64Thumbstick.h>
```

Examples are under **File > Examples > N64Thumbstick**.

## Wiring

| Stick | Arduino |
| --- | --- |
| VCC | 3.3 V |
| GND | GND |
| XA | external-interrupt pin (pin 2 on an Uno) |
| XB | any digital input (pin 4 in the examples) |
| YA | external-interrupt pin (pin 3 on an Uno) |
| YB | any digital input (pin 5 in the examples) |

Do not connect VCC to 5 V. Which physical pin is A versus B varies by how the cable was labeled; [extras/Wiring.md](extras/Wiring.md) has the connector order and what to do when an axis runs backward. [extras/Quadrature.md](extras/Quadrature.md) is the decode.

`XA` and `YA` have to support external interrupts in the default mode. On an Uno that is pins 2 and 3. Any other pin works in `N64_THUMBSTICK_POLLING` mode if `update()` is called every pass through `loop()`.

## Quick start

```cpp
#include <N64Thumbstick.h>

N64Thumbstick stick;

void setup() {
  Serial.begin(9600);
  stick.begin(2, 4, 3, 5);  // XA, XB, YA, YB
}

void loop() {
  stick.update();
  Serial.print(stick.x());
  Serial.print(' ');
  Serial.println(stick.y());
  delay(50);
}
```

`delay()` is safe in interrupt mode. Do not use it in polling mode. Hold the stick centered and call `stick.zero()` when the relaxed position should read as 0.

The object has to stay alive for the whole run, usually as a global. It is not copyable. At most four objects can be active at once (`N64_THUMBSTICK_MAX_INSTANCES`).

## Examples

| Sketch | What it shows |
| --- | --- |
| BasicRead | Interrupt mode and `read()` |
| Polling | Any digital pin, `update()` every loop |
| Recenter | `zero()` and `setOrigin(0, 0)` from the Serial Monitor |
| Deadzone | Center window with no jump at the edge |
| InvertAndSwap | A stick mounted backward or turned |
| N64Ordinates | Console-style ordinates, 4 units per edge, clamp |
| Normalized | −1..1 from the configured full-scale throw |
| Velocity | Edges per second from the gap between A-edges |
| Directions | `left()`, `right()`, `up()`, `down()` |
| OnChange | Callback from `update()`, not from the interrupt |
| RangeCalibration | Learn each axis's throw |
| TwoSticks | Two modules, polling, because an Uno has two interrupt pins |
| Pullups | When a floating line needs a pull-up, and why 5 V boards should not use the internal one |
| ServoFollow | Map X onto a servo pulse without the Servo library |
| UsbMouse | Pointer motion on a Leonardo or Micro. Install Mouse, then set `N64_EXAMPLE_USE_MOUSE` to 1 |
| Diagnostics | Architecture, interrupt pins, live A/B levels, polling fallback |

## Readings

`x()` and `y()` apply the origin, invert, swap, and deadzone. Right and up are positive unless inverted. `xRaw()` and `yRaw()` are the hardware counters with none of that applied.

`xN64()` and `yN64()` scale the shaped counts by `unitsPerEdge` (default 4) and clamp. Print them with an `int` cast. `setN64Clamp(80)` is the range games are happy with. The default clamp is 127, which is where the controller chip saturates.

`xNormalized()` and `yNormalized()` divide by `(full scale − deadzone)` and clamp to −1..1. The default full scale is 18 raw edges, an approximation of one side of the gate. `RangeCalibration` replaces it.

`xVelocity()` and `yVelocity()` are logical edges per second. They return 0 once the stick has been still longer than the last gap between edges.

The method list and the contracts are in [`src/N64Thumbstick.h`](src/N64Thumbstick.h).

## Layout

This repository is a 1.5-format Arduino library. [`extras/LibrarySpecification.md`](extras/LibrarySpecification.md) maps each part of the [library specification](https://arduino.github.io/arduino-cli/1.5/library-specification/) onto a file here.

A `.development` file in the root lets the Arduino IDE edit examples in place. It is gitignored on purpose: the Library Manager indexer skips releases that still contain it. Delete it before publishing a release.

## License

The library source is [GNU LGPL 2.1 or later](LICENSE). Copyright 2026 Jackson Teather.

The sketches under `examples/` are public domain, which is the Arduino convention for examples.
