# Wiring

The thumbstick is a self-contained module on a 6-pin JST-PH style connector. VCC is 3.3 V. GND is common with the Arduino. `XA` and `XB` are the X encoder. `YA` and `YB` are the Y encoder. The [N64brew controller page](https://n64brew.dev/wiki/Controller#Analog_Stick) names those signals and states the supply voltage. It does not list connector pin numbers in the text.

## Connector order

A teardown that marks pin 1 with the white wire, left to right, reports this order ([qwertymodo, N64 Controller](https://www.qwertymodo.com/hardware-projects/n64/n64-controller)):

| Pin | Signal |
| --- | --- |
| 1 (white) | Y1 |
| 2 | Y0 |
| 3 | X1 |
| 4 | GND |
| 5 | VCC (3.3 V) |
| 6 | X0 |

Treat that as one measured cable, not as a standard that every salvaged harness follows. Confirm GND and VCC before applying power. GND is the pin common with the controller ground if the module is still attached to a board. VCC is the other supply pin, and it must be 3.3 V. The four remaining pins are the two pairs.

The wiki's A wire is the one the decoder watches for edges. The B wire is the one sampled when A changes. The teardown's 0 and 1 labels are not the same names. Wire one wire of each pair to the interrupt pin and call that one A. If that axis then counts backward, either swap the two wires of the pair or call `invertX(true)` or `invertY(true)`. Swapping A and B flips the sign because the decoder compares the two levels.

`examples/Diagnostics` prints the live level of each pin next to the raw counts so you can see which pair moves.

## Pins used by the examples

Most sketches use this map:

| Signal | Pin | Why |
| --- | --- | --- |
| XA | 2 | External interrupt on an Uno (INT0) |
| XB | 4 | Sampled when XA changes. Any digital input. |
| YA | 3 | External interrupt on an Uno (INT1) |
| YB | 5 | Sampled when YA changes. Any digital input. |

`Polling` uses pins 6, 7, 8, and 9 to show that polling does not need interrupt pins.

`TwoSticks` uses 2/4/3/5 and 6/8/7/9, both in polling mode.

Leave pins 0 and 1 free. They are the serial port on most boards.

## Interrupt pins

`begin()` in `N64_THUMBSTICK_INTERRUPT` mode attaches `CHANGE` interrupts to `XA` and `YA`. `pinSupportsInterrupt()` reports what the core allows.

| Board | External-interrupt pins |
| --- | --- |
| Uno, Nano (ATmega328P) | 2, 3 |
| Mega | 2, 3, 18, 19, 20, 21 |
| Leonardo, Micro | 0, 1, 2, 3, 7 (avoid 0 and 1 if you use Serial) |
| Nano Every, SAMD, ESP32, RP2040, and most other current cores | Any digital pin, with a small cap on how many are attached at once |

An Uno cannot run two sticks in interrupt mode, because that needs four interrupt pins and the 328P has two. Use polling, or a board from the last row. `limitedInterruptPins()` is the coarse hint. `pinSupportsInterrupt()` is the check `begin()` uses.

## Pull-ups

Start with pull-ups off. The waveforms on the wiki are driven high and low. If a signal chatters while the stick is still, add a pull-up.

`usePullups(true)` enables the Arduino's internal pull-up on all four signal pins. On a 5 V board that resistor goes to 5 V. Use an external pull-up to 3.3 V instead, 10 kΩ is enough, and leave `usePullups` false. On a 3.3 V board the internal pull-up is safe.

## Other devices

Share ground with anything else the sketch drives. Keep the stick's VCC on 3.3 V even when a servo is powered from 5 V. `examples/ServoFollow` is that circuit.
