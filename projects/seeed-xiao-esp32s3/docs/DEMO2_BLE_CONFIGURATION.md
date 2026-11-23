# Visioball BLE configuration

## Hardware

- SK6812 RGBW data: D5, configured as `Config::Pins::LED_STRIP` in `src/configuration/config.h`.
- Set `Config::Light::LED_COUNT` to the actual strip length; the default is 1.
- The strip uses GRBW at 800 kHz. Change the pixel format in `src/light/LedStrip.cpp` for a different strip variant. The white channel stays at zero because the protocol specifies RGB only.
- Use a suitable external strip power supply and a shared ground with the ESP32. Use a suitable 3.3 V to 5 V data level shifter when required by the strip.

## BLE protocol

Connect to `Visioball` and write UTF-8 JSON to:

- Service: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- Write characteristic: `beb5483e-36e1-4688-b7f5-ea07361b26a8`

```json
{
  "configuration": {
    "audio": { "volume": 75, "mode": "wav", "type": "music" },
    "light": {
      "status": true,
      "intensity": 180,
      "type": "static",
      "color": { "r": 255, "g": 100, "b": 0 }
    },
    "game": { "mode": "default" }
  }
}
```

`audio.volume` accepts 0 through 100 percent; zero mutes the audio. Light intensity and RGB channels accept integers from 0 through 255. `status` must be a boolean. Only the `static` light type is supported. Audio `mode`, audio `type`, and `game` are currently ignored; they do not change playback or game behavior.

Partial updates preserve unspecified settings. If `color` is included, all three RGB channels are required. Invalid supported fields reject the entire update, without applying its other fields.

```json
{"configuration":{"audio":{"volume":25}}}
```

Use acknowledged BLE writes. Split JSON longer than the negotiated ATT payload (`MTU - 3` bytes) into ordered chunks. The default MTU permits 20-byte chunks. Send one JSON object at a time, without interleaving commands, with at most 5 seconds between chunks. The total limit is 1024 bytes. No newline delimiter is needed. A disconnect discards an incomplete message.

Updates pass through an eight-entry queue and are applied by the system task, normally every 5 ms after startup. Queue overflow is logged and drops the incoming update. The existing battery characteristic and text commands remain available; `VOL:75` now also updates the audio engine.

## Responsibilities

- `src/connections/ble.cpp`: BLE transport, fragment assembly, and queuing.
- `src/configuration/RuntimeConfiguration.cpp`: JSON decoding and validation, without hardware access.
- `src/light/LedStrip.cpp` and `.h`: SK6812 initialization and static RGB output, without BLE or JSON dependencies.
- `src/main.cpp`: applies received settings to the existing audio engine and strip module.

## Verification

Build with `pio run -e seeed_xiao_esp32s3`.

On hardware, send the full example and verify orange light at intensity 180 and volume 75. Repeat using 20-byte chunks. Send a volume-only update and verify that the light remains unchanged. Send `status: false`, then `status: true`, and verify that color and intensity are retained. Send volume 0 and verify mute. Finally, send volume 101 alongside a valid light change and verify that neither change is applied.

Firmware compilation is verified; BLE reception, physical colors, and audible volume still require testing on the device.