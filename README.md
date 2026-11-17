# Visioball

Visioball is an innovative ball for blind and visually impaired users, developed for Koninklijke Visio.

The ball contains a gyroscope, electronics, rechargeable batteries, and six speakers. It produces sound both while rolling and while stationary.

The sound is designed to help users locate the ball and provide feedback about its speed and direction of movement.

## Hardware

- ESP32-WROVER-E-N16R8
- MAX98357A audio amplifier
- BMI260 inertial measurement unit
- WS2813 addressable LEDs

## Structure

- `lib/Visioball/src/`: the hardware-independent PlatformIO class library:
  - `interfaces/`: the abstract actuator, effect and output interfaces.
  - `actuators/light/` and `actuators/sound/`: light and sound classes,
    their components and effect types; concrete effects live in `effects/`.
  - `motion/`: motion and spatial classes.
  - `common/`: shared color, vector and quaternion types.
  - `Ball.h` and `Ball.cpp`: the ball coordinating the model.
  Each class or value type has its own header; classes with nontrivial
  behavior also have a `.cpp` file. Includes use paths relative to
  `lib/Visioball/src/` when referring to a different folder.
- `src/hardware/`: adapters for physical devices. `NeoPixelOutput` implements
  the model's light-output interface using Adafruit NeoPixel.
- `src/main.cpp`: Arduino setup, the simulated JSON input and construction of
  objects. It owns the objects referenced by the model.
- `test/architecture/`: hardware-independent C++ checks with a fake output.

The diagram's light model holds up to **72 LEDs**, and sound holds six
speakers. The existing hardware configuration remains a 60-pixel RGBW strip
on GPIO17, with only the first 45 pixels lit for USB power. The adapter clears
all 60 pixels, including the inactive ones. Do not change these hardware
limits without checking the actual strip and power supply.

The firmware still cycles through its sample `static` light JSON payloads
every three seconds. RGB values and the optional white (`w`) component, the
0–255 intensity, and the on/off status retain their existing behavior.

## What remains to implement

- `Motion` currently reports failure: no BMI260 driver, calibration, sensor
  sampling or orientation filter is connected. `Spatial` requires a valid
  unit quaternion representing local-to-world rotation and treats world Z as
  up; confirm these coordinate conventions when implementing the sensor.
- LED and speaker positions are unconfigured. `HorizontalSplit` returns
  failure until both LED positions and motion orientation are available.
- The four sound effects return failure and do not produce audio. A speaker
  driver, gain/level handling, effect implementations and a way to stop
  playback are still needed; sound is disabled by default.
- `LightEffectType` and `SoundEffectType` are vocabulary for future selection;
  they do not yet create effects or extend the JSON protocol.

Effects and output adapters are passed by reference; callers must keep them
alive longer than the actuator using them. `update()` returns `false` when an
effect or required data is unavailable. Callers must report or handle that
failure instead of treating it as a successful update.

## Future architecture

The hardware-independent model and hardware adapters are a starting point for
clean architecture, not a complete implementation of it. As the project grows,
move JSON parsing and scheduling from `main.cpp` into an application layer,
add interfaces for motion sensing and audio output, and inject the concrete
hardware implementations at startup. The PlatformIO library separates the
reusable model from the firmware; additional libraries are only worthwhile when
their responsibilities or reuse justify another boundary. Define hardware
geometry and ownership before implementing spatial effects.

## Tests

With a native C++ compiler, the model checks can be run without an ESP32:

```powershell
g++ -std=c++11 -Ilib\Visioball\src test\architecture\main.cpp (Get-ChildItem lib\Visioball\src -Recurse -Filter *.cpp | ForEach-Object { $_.FullName }) -o architecture-tests.exe
.\architecture-tests.exe
```

The firmware can be built with `pio run -e esp32-s3-devkitc1-n16r2`.