# Visioball

Visioball is an innovative ball for blind and visually impaired users, developed for Koninklijke Visio.

The ball contains a gyroscope, electronics, rechargeable batteries, and six speakers. It produces sound both while rolling and while stationary.

The sound is designed to help users locate the ball and provide feedback about its speed and direction of movement.

## Hardware

- ESP32-WROVER-E-N16R8
- MAX98357A audio amplifier
- MPU9250 inertial measurement unit (test setup)
- 18 SK6812MINI-E addressI able LEDs (test setup)

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
- `src/main.cpp`: Arduino setup, construction of objects, MPU9250 sampling and
  motion-driven light controls. It owns the objects referenced by the model.
- `test/architecture/`: hardware-independent C++ checks with a fake output.

The diagram's light model holds up to **72 LEDs**, and sound holds six
speakers. The current test setup is configured for 18 SK6812MINI-E RGB LEDs
on GPIO18, connected in one chain. `LavaGlow` maps accumulated gyro rotation
around X to brightness, Y to color (red to yellow), and Z to animation speed.
These settings stay latched while the ball is still and can be adjusted by
rotating back the other way. A 1.5 degrees-per-second dead zone suppresses
gyro noise. LED output is capped at 96/255 brightness. The glow continues
animating at its slowest speed while stationary. Sensor samples are read at
50 Hz, and the strip is refreshed at 40 Hz. Confirm the pin, power supply and
wiring before connecting the LEDs. The previous simulated JSON input and its
parser have been removed.

## MPU9250 test

Connect MPU9250 SDA to GPIO16 and SCL to GPIO17, with suitable power, common
ground and I²C pull-up resistors. Open the serial monitor at 115200 baud. The
firmware checks I²C addresses 0x68 and 0x69 and prints acceleration (g) and
bias-corrected angular velocity (degrees per second) for X, Y and Z every
250 ms. Keep the sensor stationary for the one-second gyro calibration at
startup and after reconnect. The gyro rates, with their offsets removed, are
integrated into latched light controls; acceleration is printed for reference
and includes gravity. Axes are relative to the sensor. If the sensor is
unavailable or reading fails, an error is printed and initialization is
retried every five seconds. If the I²C bus itself cannot be initialized, an
error is printed and a restart is required; the light continues its idle
animation.
`Serial` uses the ESP32-S3 USB-Serial/JTAG connection via the build flags in
`platformio.ini`. Close the serial monitor before uploading new firmware.

## What remains to implement

- `Motion` still reports failure and is not connected to the firmware's
  MPU9250 test path. The firmware directly uses calibrated gyro rates.
  `Spatial` requires a valid
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
add input handling and scheduling in an application layer,
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