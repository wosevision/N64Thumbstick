# Quadrature

The stick has one optical disk per axis. Slots in the disk pass light to a pair of sensors. The two outputs of a pair are offset, which is ordinary quadrature. The NUS-CNT chip does not read a voltage for "how far". It counts edges.

From the [Analog Stick](https://n64brew.dev/wiki/Controller#Analog_Stick) section:

- `XA` and `YA` are the interrupt signals.
- `XB` and `YB` choose the sign.
- Any edge of A counts, rising or falling.
- The chip adds those steps onto a position that starts at 0.

The wiki's comparison is not available as plain text (the condition does not survive a fetch of the page). This library uses the 2× decode that comparison describes:

- On each edge of A, read A and B.
- If the levels are equal, add 1.
- If they differ, subtract 1.

B is not given its own interrupt. Edges of B that happen while A is steady do not count. That matches the chip, which listens for edges of `XA` and `YA` only.

If a module counts backward, `invertX()` or `invertY()` flips later readings. The stored count is not rewritten. Set invert and swap before `zero()`, or call `zero()` again afterward.

## What the console stores

The count is relative. Powering on with the stick held to one side makes that pose read as center until something recenters it. `begin()` does the same: both axes start at 0. `zero()` captures the pose you are holding. `setOrigin(0, 0)` returns to the `begin()` frame.

A measurement of the real module found that one A-edge moves the console's report by 4, and that a physical throw is about ±18 edges, roughly ±72 report units ([qwertymodo, N64 Controller](https://www.qwertymodo.com/hardware-projects/n64/n64-controller)). The report is a signed 8-bit value and the chip stops at the int8 limits. `xN64()` and `yN64()` apply that scale (`unitsPerEdge`, default 4) and a clamp (default 127). Values outside about ±80 (`0x50`) make some games misbehave, which is why `examples/N64Ordinates` calls `setN64Clamp(80)`.

The N64 programming manual's minimum throws are on the same order, about 70 console counts to the side before the center window is removed ([controller data](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro26/26-02.html)). Dividing by 4 lands near the 18-edge default full scale (`N64_THUMBSTICK_DEFAULT_FULL_SCALE`). It is an approximation. `calibrateBegin()` / `calibrateUpdate()` / `calibrateEnd()` measure the stick you have. Wear and the octagonal gate both change it. X and Y are stored separately because they are not the same travel.

The manual also allows a center window of at most about 7 console counts. That is about 2 raw edges. `examples/Deadzone` uses `setDeadzone(2)`. Inside the window the reading is 0. Outside it, the window width is subtracted so the value does not jump when you leave the window.

## Speed

The gap between A-edges gets shorter as the stick moves faster. The wiki notes that the console does not need that, and that a custom interface can still use it. `xVelocity()` and `yVelocity()` are logical edges per second from the last gap. When no new edge arrives for longer than twice that gap, and for at least 20 ms, the value returns to 0. A single fast edge does not leave a stale speed behind.

## What this library does not read

Buttons, the Joybus data wire, and the accessory port are not part of the module. Nothing in this library speaks Joybus.
