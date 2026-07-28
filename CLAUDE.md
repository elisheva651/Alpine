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

- **Build:** PlatformIO with [maxgerhardt/platform-raspberrypi](https://github.com/maxgerhardt/platform-raspberrypi.git) + earlephilhower Arduino-Pico core. Code uses Pico SDK APIs only (no Arduino API calls).
- **UI:** Custom minimal graphics — direct SPI writes, 5x7 bitmap font, no framework (LVGL etc.)
- **Libraries:** TinyUSB (USB mass storage), FatFs (SD card), minmea (GPS NMEA parsing, vendored, WTFPL license)
- **Dual-core:** Core0 = UI + SD + buttons. Core1 = GPS + sensors + track logging.

## Project Structure

```
platformio.ini       ← PlatformIO build config (project root)
FW/                  ← firmware source
  src/
    main.c           ← core0 entry (setup/loop), Arduino entry points
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
    shared.h         ← inter-core shared data struct (mutex-protected)
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

- C/C++ with Pico SDK APIs (hardware_spi, hardware_uart, hardware_adc, hardware_gpio, hardware_pwm, pico_multicore)
- Pin assignments and constants defined in `include/config.h`
- Screen views are separate files in `ui/` with a common state machine — each view has `view_*_render()` and `view_*_input()` functions
- GPS data shared between cores via `shared_data_t` in `include/shared.h`, protected by `mutex_t` initialized in `setup()` (SHARED_LOCK/SHARED_UNLOCK macros)
- Storage files (sd, map_loader, track) are `.cpp` because the framework's FatFS uses C++ namespaces — all headers have `extern "C"` guards for C compatibility
- Display and SD card share SPI0 bus — use `display_spi_acquire()`/`display_spi_release()` when accessing SD
- RGB565 byte order: swap bytes for SPI transmission (`(color >> 8) | (color << 8)`)

## Current Status

- [x] Project scaffold (PlatformIO + Pico SDK)
- [x] Display driver (ILI9341 + ST7789 auto-detect, full graphics primitives)
- [x] GPS integration (UART + minmea parsing GGA/RMC/GSA)
- [x] SD card + FatFs (shared SPI bus with display)
- [x] Button input (debounce + long-press detection)
- [x] Dashboard view (all sensor data rendered)
- [x] Map view (row-by-row viewport rendering, GPS overlay, d-pad pan)
- [x] Track recording (GPX format, haversine distance, elevation stats)
- [x] Settings view (brightness, sleep, units, GPS rate, track interval)
- [x] Power management (battery ADC, backlight PWM, sleep/wake)
- [x] UI state machine (Select cycles views, input routing)
- [x] Dual-core (core0=UI, core1=GPS+sensors+logging)

### Needs hardware/further work

- [ ] Compass + barometer: stubs only — need UBX binary protocol for M10's QMC5883L magnetometer and BMP280 barometer (currently heading falls back to GPS course)
- [ ] USB mass storage: stub only — need TinyUSB MSC read/write callbacks
- [ ] Sleep: uses WFE loop — need proper RP2040 dormant mode with GPIO wake interrupt
- [ ] Settings persistence: values not saved to SD yet
- [ ] Track view: Select should toggle recording (currently routed to view switch)
- [x] Build compiles clean (5.2% RAM, 5.6% Flash) — not yet tested on hardware
