# Folderindeling voor de firmware

## Advies

Houd de firmware voorlopig in `src/` en groepeer de code per subsysteem. Gebruik `lib/` alleen voor een private library die bewust herbruikbaar en zelfstandig moet zijn. Een map in `lib/` is geen vereiste om teamleden onafhankelijk aan verschillende onderdelen te laten werken.

Deze keuze sluit aan op de huidige repository en houdt de samenvoeging van de 43-LED-demo beperkt. De demo zet domeinklassen in `lib/Visioball`, hardware-adapters in `src/hardware` en de applicatiestart in `src/main.cpp`; dat is een geldige PlatformIO-indeling, maar niet nodig voor dit ene firmwareproject.

## Voorgestelde indeling

```text
src/
  main.cpp                    # Opstart, objectcompositie en FreeRTOS-taken
  Ball.h
  Ball.cpp                    # Coördinatie tussen motion, licht en audio
  IActuator.h                 # Gedeelde actuatorinterface

  configuration/
    config.h                  # Compile-time pinnen en hardware-instellingen
    RuntimeConfiguration.h
    RuntimeConfiguration.cpp

  motion/
    IMotionSensor.h           # Sensorcontract uit het klassendiagram
    MotionSample.h            # Gedeeld motion-datamodel
    Motion.h
    Motion.cpp
    MotionTarget.h
    TapDetector.h
    TapDetector.cpp

  spatial/
    Vector3.h
    Quaternion.h
    Orientation.h
    Orientation.cpp
    Spatial.h
    Spatial.cpp

  light/
    Color.h
    LED.h
    LightSettings.h
    ILightEffect.h
    ILightOutput.h             # Outputcontract; Light hangt niet aan NeoPixel vast
    Light.h
    Light.cpp
    effects/
      Solid.h
      HorizontalSplit.h
      SingleLED.h

  sound/
    ISoundEffect.h
    SoundEffectType.h
    Sound.h
    Sound.cpp
    effects/                   # Of de huidige losse sound-effectbestanden

  hardware/
    motion/
      Bmi270Sensor.h
      Bmi270Sensor.cpp
      MPU9250Sensor.h
      MPU9250Sensor.cpp
    light/
      NeoPixelOutput.h
      NeoPixelOutput.cpp

  connections/
  debug/

test/
  test_motion/
  test_light/
```

Bestanden mogen onder de huidige subsysteemmappen blijven als verplaatsen nu onnodige wijzigingen oplevert. De belangrijke grens is dat hardwaredrivers interfaces implementeren en dat domeinklassen niet rechtstreeks een specifieke sensor of LED-library hoeven te beheren.

## Grenzen tussen de onderdelen

- **Licht:** de lichtverantwoordelijke beheert `src/light/` en de NeoPixel-outputadapter. `Light` en lichteffecten gebruiken `Spatial` en de gedeelde effect-/outputinterfaces.
- **Motion en sensorselectie:** `Motion` gebruikt `IMotionSensor`; BMI270 en MPU9250 implementeren dat contract. De MPU9250-pinnen staan in `configuration/config.h` (`SDA = 16`, `SCL = 17`).
- **Spatial:** `Orientation` en `Spatial` lezen dezelfde motiontoestand die `Motion` bijwerkt. De lichtcode leest deze toestand; hij initialiseert of pollt geen eigen sensor.
- **Applicatie:** `main.cpp` maakt de objecten aan en behoudt de bestaande taakverdeling: de SystemTask werkt motion, `Ball` en licht bij; de audio-DSP-task blijft verantwoordelijk voor audio-rendering.
- **Audio:** gebruik `WavDSP` als implementatienaam waar het diagram `Spotify` noemt.
- **LED-aantal:** de 43 LEDs in de demo zijn de testopstelling. Gebruik de positiekaart en het aantal van de uiteindelijke bal; de huidige configuratie is 72 LEDs.

## Samenwerken zonder conflicten

Leg eerst de gedeelde headers en methodesignatures vast, in het bijzonder `IMotionSensor`, `MotionSample`, `Orientation`, `Spatial`, `IActuator`, `ILightEffect` en `ILightOutput`. Daarna kan ieder teamlid binnen de eigen subsystemen werken. Wijzigingen aan gedeelde contracten moeten samen worden afgestemd; laat `main.cpp` de enige plek zijn die de concrete hardware- en domeinobjecten met elkaar verbindt.

## Wanneer `lib/Visioball` wel passend is

Verplaats domeinklassen alleen naar `lib/Visioball/src/` als die module als één zelfstandige private library wordt onderhouden of ook door andere PlatformIO-projecten wordt gebruikt. Houd de afhankelijkheden dan in één richting: de library mag niet terugverwijzen naar applicatiebestanden in `src/`. Zet `main.cpp` en board-/hardwarecompositie in `src/`. Verdeel niet willekeurig één samenhangend domein over `src/` en `lib/`.

Voor de huidige firmware is de indeling per subsysteem in `src/` de eenvoudigste keuze; `lib/` kan gereserveerd blijven voor zelfstandige projectlibraries en externe dependencies.
