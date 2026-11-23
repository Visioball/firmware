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
- `src/hardware/`: adapters for physical devices. `MPU9250Sensor` reads the
  IMU, and `NeoPixelOutput` implements the light-output interface.
- `src/main.cpp`: initializes the hardware and connects a shared `Orientation`
  to `Motion` and `Spatial`, and `Motion`, `Light`, and `Sound` to `Ball`.
- `test/architecture/`: hardware-independent C++ checks with a fake output.

`Ball` holds references to `Motion`, `Light`, and `Sound`. Its `update()` reads
motion and then updates both actuators. `Light` contains up to **72 LEDs**, and
`Sound` contains six speakers. The firmware configures the 18-LED test setup
on GPIO18 and lights LEDs in red at 96/255 brightness. It starts with
`HorizontalSplit` (upper half on) and alternates every ten seconds with
`SingleLED` (the highest LED on). Sound remains disabled.
When LEDs are tied for highest, `SingleLED` uses the lowest numbered LED.
Rotation around the vertical axis alone does not change which LED is highest.

The test mapping is in `src/hardware/TestLedPositions.h`. LEDs 1-9 are on the
lower face and 10-18 are on the sensor face, following the photographed arrow
on each face; array indices are zero-based. The nine LEDs on each triangular
board are approximated as equally spaced around a circular ring on a sphere,
not measured physical coordinates. Sensor axes are +Y toward the USB end, +Z
toward the sensor face, and +X toward the right when viewing that face with
USB at the top. Confirm the wiring order and mapping on hardware before
relying on it.

## MPU9250 test

Connect MPU9250 SDA to GPIO16 and SCL to GPIO17, with suitable power, common
ground and I²C pull-up resistors. The firmware expects the sensor at I²C
address 0x68. The firmware samples the IMU approximately every 10 ms and prints
acceleration (g), gyroscope rotation rate (degrees per second), normalized
acceleration direction, and orientation quaternion every 250 ms at 115200
baud. Keep the ball still when starting to initialize its orientation.
`Motion` uses `Orientation` to integrate gyro rates and correct tilt using
acceleration near 1 g. `Spatial` reads the same `Orientation` for position
calculations. The quaternion rotates sensor-local coordinates to
world coordinates, where Z points up. Mount the sensor axes to match the
local coordinates used for LEDs and speakers.

This is a basic estimate, not a compass: yaw is arbitrary at startup and
drifts without a magnetometer. Acceleration during motion can also disturb the
tilt estimate. Orientation is unavailable until a usable acceleration sample
arrives, and is reinitialized after a sampling gap over 250 ms. Initialization
failure is reported over serial and stops sensor sampling. The existing
sensor adapter does not yet detect failed reads.
`Serial` uses the ESP32-S3 USB-Serial/JTAG connection via the build flags in
`platformio.ini`. Close the serial monitor before uploading new firmware.

## What remains to implement

- The test LED positions approximate the ball geometry; speaker positions are
  unconfigured. Spatial light effects need a valid orientation.
- The MPU9250 adapter needs reliable I2C read failure reporting. Gyro bias
  calibration and more robust sensor fusion are not implemented.
- The four sound effects return failure and do not produce audio. A speaker
  driver, gain/level handling, effect implementations and a way to stop
  playback are still needed; sound is disabled by default.
- `LightEffectType` and `SoundEffectType` are vocabulary for future selection;
  they do not yet create effects or extend the JSON protocol.

The motion, light, and sound services must outlive the `Ball` that references
them. The shared `Orientation` must outlive both `Motion` and `Spatial`.
Effects and output adapters must also outlive the actuator using them.
`update()` returns `false` when an effect or required data is unavailable.
Callers must report or handle that failure instead of treating it as a
successful update.

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
g++ -std=c++17 -Ilib\Visioball\src test\architecture\main.cpp (Get-ChildItem lib\Visioball\src -Recurse -Filter *.cpp | ForEach-Object { $_.FullName }) -o architecture-tests.exe
.\architecture-tests.exe
```

The firmware can be built with `pio run -e esp32-s3-devkitc1-n16r2`.