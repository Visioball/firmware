# Visioball

Firmware for the Visioball, a sound- and motion-enabled ball for blind and visually impaired users.

## Build

Build the Seeed XIAO ESP32S3 firmware with PlatformIO:

```powershell
pio run -e seeed_xiao_esp32s3
```

## Project structure

- `src/main.cpp`: firmware startup and task coordination.
- `src/configuration/`: compile-time settings and BLE configuration parsing. `config.json` is a reference file and is not loaded by the firmware.
- `src/debug/`: serial commands and diagnostic logging.
- `src/audio/`, `src/light/`, `src/motion/`, `src/connections/`: firmware subsystems.
- `assets/`: source audio assets; `data/`: files uploaded to LittleFS.
- `docs/`: demo guides and architecture documents.

## Documentation

- [Demo 2: server, clients, and Bluetooth](docs/DEMO2_README.md)
- [Demo 2 BLE configuration and protocol](docs/DEMO2_BLE_CONFIGURATION.md)
- [Class diagram and proposed architecture](docs/class-diagram.md)
