# Alpine GPS

Standalone hiking GPS device. Shows position on user-uploaded maps, displays altitude/compass/weather, records tracks. Sleeps most of the time, wakes on button press. Replaces pulling out your phone on a hike.

## Hardware

- **MCU:** Waveshare RP2040-Zero (dual Cortex-M0+ @ 133MHz, 264KB RAM, 2MB flash, USB-C)
- **GPS:** GEPRC u-blox M10 (GPS + magnetometer + barometer) via UART
- **Display:** TFT SPI 240x320 (ILI9341 or ST7789, detect at init)
- **Storage:** SD card adapter + 64GB, shared SPI bus with display
- **Power:** 3.7V 2000mAh LiPo + TP4056 charger → RP2040 VSYS pin
- **Input:** 5 buttons (up/down/left/right/select) via GPIO

## Stack

- **Build:** PlatformIO + Pico SDK (C/C++). No Arduino.
- **UI:** Custom minimal graphics — direct SPI writes, no framework (LVGL etc.)
- **Libraries:** TinyUSB (USB mass storage), FatFs (SD card), minmea (GPS NMEA parsing)
- **Dual-core:** Core0 = UI + SD + buttons. Core1 = GPS + sensors + track logging.

## Project Structure

```
FW/                  ← firmware (PlatformIO project)
  src/
    main.c           ← core0 entry, main loop
    core1.c          ← core1 entry, GPS + sensor loop
    display/         ← TFT SPI driver + graphics primitives
    gps/             ← UART + NMEA parsing
    sensors/         ← compass + barometer (via M10 module)
    storage/         ← SD card, map loader, track recorder
    ui/              ← view state machine (dashboard, map, track, settings)
    input/           ← button debounce + press detection
    power/           ← battery ADC, sleep/wake, backlight PWM
    usb/             ← TinyUSB mass storage
  include/
    config.h         ← pin definitions, constants
  lib/
    minmea/          ← vendored NMEA parser
HW/                  ← wiring docs, pinout reference
docs/                ← design specs
assets/              ← fonts, icons, test images
```

## Key Constraints

- **264KB RAM** — cannot hold full framebuffer. Map rendering streams row-by-row from SD to SPI.
- **2MB flash** — firmware only. All data lives on SD card.
- **No wireless** — no WiFi, no BLE. File transfer via USB mass storage.
- **Maps** are pre-converted to raw RGB565 by a companion web app (separate repo). Stored on SD with a meta.json for geo-referencing.

## Conventions

- C/C++ with Pico SDK APIs
- Pin assignments defined in `include/config.h`
- Screen views are separate files in `ui/` with a common state machine
- GPS data shared between cores via spin-locked shared struct

## Current Status

- [ ] Project scaffold (PlatformIO + Pico SDK)
- [ ] Display driver
- [ ] GPS integration
- [ ] SD card + FatFs
- [ ] Button input
- [ ] Dashboard view
- [ ] Map view
- [ ] Track recording
- [ ] USB mass storage
- [ ] Power management
