# Visioball

Visioball is an innovative ball for blind and visually impaired users, developed for Koninklijke Visio.

The ball contains a gyroscope, electronics, rechargeable batteries, and six speakers. It produces sound both while rolling and while stationary.

The sound is designed to help users locate the ball and provide feedback about its speed and direction of movement.

## Choosing a board

The board is selected with the PlatformIO environment, defined in
`platformio.ini`:

- Terminal: `pio run -e esp32-s3-devkitc1-n16r2`,
  `pio run -e seeed_xiao_esp32s3` or
  `pio run -e esp32-s3-devkitc1-n32r16`, with `-t upload` to flash.
- VS Code: use the PlatformIO environment switcher in the status bar
  (the folder icon with the env name) to pick the board for build,
  upload and monitor.

## Build configurations

All three boards use the same model in `src/` and the same `src/main.cpp`:

- `esp32-s3-devkitc1-n16r2`: the MPU9250 LED demo. Audio and BLE are
  disabled on this board; a stub actuator reports sound updates as
  successful.
- `seeed_xiao_esp32s3`: the BMI270 LED demo with audio and BLE
  (`-DVISIOBALL_BMI270`). Upload the audio files to LittleFS with
  `pio run -e seeed_xiao_esp32s3 -t uploadfs` before starting the firmware.
- `esp32-s3-devkitc1-n32r16`: the ESP32-S3-DevKitC-1 v1.1 BMI270, audio and
  BLE build. GPIO 40 drives all 72 LEDs; GPIO 39 also drives LEDs 37-72.
  Upload the audio files to LittleFS with
  `pio run -e esp32-s3-devkitc1-n32r16 -t uploadfs`.
  This board uses `partitions-n32r16.csv` for the larger audio filesystem.
  Flash the firmware and its partition table before uploading LittleFS. The
  full WAV in `data/` does not fit the XIAO's smaller filesystem.
  Its custom PlatformIO board profile is `esp32-s3-devkitc1-n32r16`
  (32 MB octal flash, 16 MB octal PSRAM). The serial monitor uses the
  board's USB/UART connector at 115200 baud.

The MPU9250 DevKit uses the 18-position test mapping; the XIAO and BMI270
DevKit v1.1 use the 72-position Fusion mapping below. All builds continuously
show `HorizontalSplit`, with a smoothstep fade only just above the horizontal
boundary (world heights 0 to +0.15 on the unit sphere). The lower half stays
completely off. The number of LEDs in this narrow transition depends on their
positions and orientation. BLE can change the BMI270 builds' color, brightness
and enabled state.
The light output scales the requested intensity (0-255) to a maximum brightness
of 100/255 on every board, regardless of its source. BLE status reports the
requested intensity, so 100% in the app corresponds to the safe maximum.
The BMI270 builds switch the light off when no BLE client is connected and
restore the configured light status on connection. This does not yet put the
device to sleep or stop the audio.

The BMI270 builds start with audio muted and BLE available. Write the text
`PLAY` to the BLE command characteristic to enable audio, or `PAUSE` to mute
it again. The initial volume is 1%; volume and audio mode changes via BLE or
serial do not unmute it. Audio remains enabled after a BLE disconnect until
`PAUSE` or a restart. This quiet mode still runs the audio and I2S tasks; it
is not a power-saving sleep mode. Tap gesture controls are not used.

In WAV mode, the motion engine estimates rolling speed from the gyroscope
component perpendicular to gravity. The WAV always plays forwards: below
0.4 rad/s it plays at normal speed, then accelerates linearly up to 2x at
24 rad/s. Assuming a 25 cm diameter ball rolling without slipping, this
corresponds to approximately 0.05-3 m/s. The 3 m/s limit is a tuning estimate
for a gym floor, not a measured maximum; faster motion remains capped at 2x.
The BMI270 gyroscope is configured for +/-2000 degrees/s per axis (approximately
4.36 m/s for a 25 cm ball rotating around one sensor axis).
The speed estimate is smoothed and depends on a valid orientation; it does not
distinguish rolling from other horizontal rotations or detect slipping.
Playback uses resampling,
so pitch rises with speed. WAV delay and bitcrusher effects are disabled.

For example, in nRF Connect connect to `Visioball`, find service
`4fafc201-1fb5-459e-8fcc-c5c9c331914b`, and write `PLAY` or `PAUSE` as text
to characteristic `beb5483e-36e1-4688-b7f5-ea07361b26a8`.

The XIAO LED data pin is D5, the BMI270 uses SPI and INT1, and I2S uses
BCK D1, WS D0 and DATA D2. See `src/configuration/config.h` for pin
definitions; the BLE JSON protocol is implemented in
`src/configuration/RuntimeConfiguration.cpp` and `src/connections/ble.cpp`.
The audio output duplicates mono over I2S stereo; it is not a
six-channel driver.

### ESP32-S3-DevKitC-1 v1.1 pinout

Select the `esp32-s3-devkitc1-n32r16` PlatformIO environment. The firmware uses
the Fusion 72-LED position map. GPIO 40 sends LEDs 1-72 and GPIO 39 sends a
duplicate of LEDs 37-72:

| ESP32-S3 pin | Component | Component pin | Note |
|---|---|---|---|
| 5V (VIN) | Protected battery / HW-373 B+ | B+ | Verify supply voltage against the board input specification |
| GND | Protected battery / HW-373 B- | B- | Common ground |
| 3V3 | BMI270 | VCC / VIN | Sensor supply |
| GPIO 4 | MAX98357A | SD | LOW while muted; HIGH while BLE playback is enabled |
| GPIO 5 | MAX98357A | DIN | I2S data |
| GPIO 6 | MAX98357A | BCLK | I2S bit clock |
| GPIO 7 | MAX98357A | LRC | I2S word select |
| GPIO 10 | BMI270 | CS / CSB | SPI chip select |
| GPIO 11 | BMI270 | SDA / MOSI | SPI MOSI |
| GPIO 12 | BMI270 | SCL / SCK | SPI clock |
| GPIO 13 | BMI270 | SDO / MISO | SPI MISO |
| GPIO 40 | SK6812 LED 1 | DIN | Add 330-470 ohm series resistor near first LED |
| GPIO 39 | SK6812 LED 37 (split wiring only) | DIN | Add 330-470 ohm series resistor near first LED |

For a single 72-LED chain, connect GPIO 40 to LED 1 and leave GPIO 39
unconnected. For two 36-LED strings, connect GPIO 40 to LED 1 and GPIO 39 to
LED 37. Do not connect GPIO 39 to LED 37 when LED 36's output is connected to
LED 37's input: each LED data input must have only one driver.

Power distribution as provided: battery/HW-373 B+ to the ESP32-S3 5V/VIN,
MAX98357A VIN and both LED-string VDD/5V pins; battery/HW-373 B- to ESP32-S3,
MAX98357A, BMI270 and both LED-string grounds. Leave HW-373 OUT+ and OUT-
unconnected. Use suitably thick power wires for the LED strings, a 470-1000 uF
capacitor near the LEDs, and a 100-470 uF capacitor near the amplifier. Keep
the grounds common.

**Power check:** a protected single-cell battery supplies about 3.3-4.2 V; it
does not provide regulated 5 V. Do not assume that connecting it directly to
the DevKit's 5V/VIN pin or to 5 V LED strips supplies the required voltage.
Confirm the exact board and component input ranges, and add an appropriately
rated boost/regulator if 5 V is required. Also verify the 3.3 V LED data signal
is accepted by the SK6812 strips at their supply voltage; use a suitable logic
level shifter if required.

### BLE communication protocol

The common UUIDs and status encoding are defined in
`src/connections/CommunicationProtocol.h`. The status characteristic
`a1b2c3d4-1111-2222-3333-abcdefabcdf0` is readable and notifies when its
value changes. Version 2 contains six bytes, so it fits the minimum BLE MTU:

| Byte | Meaning |
|---|---|
| 0 | Protocol version: `2` |
| 1 | Flags: bit 0 configured light enabled, bit 1 audio playing, bit 2 battery level available, bit 3 charging state available, bit 4 charging |
| 2 | Configured light intensity, 0–255 |
| 3 | Audio master volume, 0–100 |
| 4 | Battery percentage, 0–100, valid only if bit 2 is set |
| 5 | Active audio catalog index; `255` means no loaded/active selection |

The read-only audio catalog characteristic
`a1b2c3d4-1111-2222-3333-abcdefabcdf1` contains an ASCII JSON document, at most
512 bytes. Clients must read the complete value (GATT long read if needed):

```json
{"version":1,"modes":[{"index":0,"name":"Musicbox","type":"sf2"},{"index":1,"name":"Piano","type":"sf2"},{"index":2,"name":"Xylophone","type":"sf2"},{"index":3,"name":"Fireball","type":"wav"},{"index":4,"name":"Hedwigs Theme","type":"wav"},{"index":5,"name":"Synth","type":"synth"}]}
```

The catalog is scanned from LittleFS at startup: SF2 files alphabetically,
then WAV files alphabetically, then Synth. Names omit the extension.
Indices are session-local; reread the catalog after every connection rather
than hardcoding them. The current N32R16 `data/` directory contains exactly
these five files. Source originals remain in `assets/`.

Write `MODE:<index>` as text to the command characteristic, using an index
from the catalog (0-24). The audio engine loads the selected file and updates
status byte 5 only after success. On load failure it logs the error and reports
`255`; invalid indices are rejected without changing the existing selection.
The app waits up to 15 seconds for the selected index before sending `PLAY`.
Selection itself does not unmute audio. `PAUSE` mutes it; `VOL:0` through
`VOL:100` change master volume. The catalog transfers metadata, not audio.
While a file is being loaded, the audio task streams silence. Only one
file-backed effect is kept in memory at a time; switching WAV tracks restarts
the new track at its beginning, using the existing output sample rate.

The app reads the catalog on native BLE and Web Bluetooth, and supports status
v1 for older devices, but file selection requires this catalog and status v2.
Update both the app and firmware; the old app cannot decode status v2.

Light enabled is the configured setting; the BLE-disconnect policy can
temporarily turn the LEDs off. Audio playing is independent of volume, so
zero volume mutes without pausing playback. Charging is unknown unless bit 3
is set. Neither BMI270 board currently has a battery meter or charging sensor
connected: both availability bits remain clear. Existing JSON configuration
and legacy battery commands remain unchanged.

The XIAO build addresses 72 LEDs as one string. Its position array contains
Fusion LED lens centers projected onto a unit sphere, in BMI270 sensor axes:
+X points toward the ESP and +Z toward top PCB 4. Indices 0-35 correspond to
B01-B36 and indices 36-71 to T01-T36. Confirm the sensor orientation and LED
wiring on hardware. With one 72-LED string, `SingleLED` selects one LED across
the whole array. On the DevKit v1.1, the output adapter sends indices 0-71 to
GPIO 40 and duplicates indices 36-71 on GPIO 39.
Power the LED strip from a separate 5 V supply and connect its ground to XIAO
ground.

## Hardware

- ESP32-S3-DevKitC-1-N16R2 with MPU9250 over I²C (test setup)
- Seeed XIAO ESP32S3 with BMI270 over SPI and I2S audio (test setup)
- ESP32-S3-DevKitC-1 v1.1 with BMI270 over SPI, MAX98357A I2S audio and two
  SK6812 strings (third build target; hardware wiring must be verified)
- 18 SK6812MINI-E addressable LEDs (DevKit test setup); the XIAO build supports
  72 LEDs in a 2-by-36 arrangement

## Structure

The layout follows the class diagram in `docs/Klassendiagram v3.png`; the
`Spotifyle` sound effect in the diagram is implemented as `WavDSP`.

- `src/Ball.h` and `src/Ball.cpp`: the ball coordinating `Motion` and the
  `IActuator`s for light and sound.
- `src/interfaces/`: `IActuator` and the light and sound effect/output interfaces.
- `src/actuators/light/`: `Light` (72 `LED`s), `Color`, `LightEffectType`,
  the `LEDOutput` adapter and the LED test mapping in
  `LEDPositions.h`; concrete effects (`Solid`, `HorizontalSplit`,
  `SingleLED`, `LavaGlow`) live in `effects/`.
- `src/actuators/sound/`: the `Sound` engine (six `Speaker`s) with the
  `ISoundEffect` implementations `Beep`, `Synth`, `SoundFont` and `WavDSP`,
  plus the `I2sOutput` adapter. This subsystem requires the ESP32 and is
  built for the BMI270 boards.
- `src/motion/`: `Motion` and `Orientation`, using the `IMotionSensor`
  interface and `MotionSample`.
- `src/motion/spatial/`: `Vector3`, `Quaternion` and `Spatial`.
- `src/energy/`: `Energy` tracks BLE availability for BMI270-board lighting and
  can obtain a battery percentage from a `BatteryLevelMeter` implementation.
  A meter, if provided, must outlive the `Energy`
  instance; `getBatteryLevel` returns false without changing its output when
  a reading fails or is outside 0-100%.
  Its inactivity policy is currently not connected to the firmware loop:
  enabling it and recording activity only determine eligibility for sleep;
  they do not stop the hardware or change power consumption.
- `src/sensors/`: the `IMotionSensor` adapters `MPU9250Sensor` (MPU9250 DevKit)
  and `Bmi270Sensor` (XIAO and DevKit v1.1).
- `src/configuration/`, `src/connections/`, `src/debug/`: BLE JSON
  configuration, BLE transport and diagnostic logging (BMI270 builds).
- `src/main.cpp`: initializes the board-specific hardware and connects a
  shared `Orientation` to `Motion` and `Spatial`, and `Motion`, `Light`,
  and the sound actuator to `Ball`.
- `test/architecture/`: hardware-independent C++ checks with fake
  output, sensor and sound actuator.

The per-board source selection is done with `build_src_filter` in
`platformio.ini`; headers shared by all boards must stay free of
board-specific dependencies.

`Ball` holds references to `Motion`, `Light`, and the sound `IActuator`. Its
`update()` reads motion and then updates both actuators. `Light` contains up
to **72 LEDs**, and `Sound` contains six speakers. The MPU9250 DevKit firmware
configures the 18-LED test setup and lights LEDs in red at 96/255 brightness.
It starts with `HorizontalSplit` (upper half on) and alternates every ten
seconds with `SingleLED` (the highest LED on). On the MPU9250 DevKit sound
remains disabled.
When LEDs are tied for highest, `SingleLED` uses the lowest numbered LED.
Rotation around the vertical axis alone does not change which LED is highest.

The test mapping is in `src/actuators/light/LEDPositions.h`. LEDs 1-9 are on
the lower face and 10-18 are on the sensor face, following the photographed
arrow on each face; array indices are zero-based. The nine LEDs on each
triangular board are approximated as equally spaced around a circular ring on
a sphere, not measured physical coordinates. Sensor axes are +Y toward the
USB end, +Z toward the sensor face, and +X toward the right when viewing that
face with USB at the top. Confirm the wiring order and mapping on hardware
before relying on it.

## MPU9250 test (DevKit)

Connect MPU9250 SDA to GPIO16 and SCL to GPIO17, with suitable power, common
ground and I²C pull-up resistors, and the LED data line to GPIO18. The
firmware expects the sensor at I²C address 0x68. The firmware samples the IMU
approximately every 10 ms and prints
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
The MPU9250 DevKit uses the ESP32-S3 USB-Serial/JTAG connection for `Serial`;
the N32R16 build uses the USB/UART connector. Close the serial monitor before
uploading new firmware.

## Known limitations

- The test LED positions approximate the ball geometry; speaker positions are
  axis-based defaults. Spatial light effects need a valid orientation.
- The MPU9250 adapter needs reliable I2C read failure reporting. More robust
  sensor fusion is not implemented.
- On the MPU9250 DevKit no audio is produced. The audio output duplicates mono
  to both I2S channels; six discrete speaker channels still need a
  PCM1681-style driver.
- `LightEffectType` is vocabulary for future selection; it does not yet
  create effects or extend the JSON protocol.
- Battery measurement is not yet available on the BMI270 builds: there is no
  battery-voltage ADC connection, charging sensor or meter implementation.
  Add a suitable measurement circuit before enabling measurements or calling
  BLE `sendBattery`.
  See [Seeed's battery usage documentation](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/#battery-usage).

The motion, light, and sound services must outlive the `Ball` that references
them. The shared `Orientation` must outlive both `Motion` and `Spatial`.
Effects and output adapters must also outlive the actuator using them.
`update()` returns `false` when an effect or required data is unavailable.
Callers must report or handle that failure instead of treating it as a
successful update.

## Future architecture

The hardware-independent model and hardware adapters are a starting point for
clean architecture, not a complete implementation of it. As the project grows,
add input handling and scheduling in an application layer and keep injecting
the concrete hardware implementations at startup in `main.cpp`. Define
hardware geometry and ownership before implementing spatial effects.

## Tests

With a native C++ compiler, the model checks can be run without an ESP32:

```powershell
g++ -std=c++17 -Itest\architecture -Isrc test\architecture\main.cpp src\Ball.cpp src\energy\Energy.cpp src\actuators\light\Light.cpp src\actuators\light\LEDOutput.cpp (Get-ChildItem src\actuators\light\effects -Filter *.cpp | ForEach-Object { $_.FullName }) src\motion\Motion.cpp src\motion\Orientation.cpp src\motion\spatial\Spatial.cpp -o architecture-tests.exe
.\architecture-tests.exe
```

Firmware builds for all three boards are listed above. Device behavior, LED
wiring, audio playback and BLE should also be verified on the respective
hardware.