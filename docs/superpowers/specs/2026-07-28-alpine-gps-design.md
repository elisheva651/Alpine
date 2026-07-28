# Alpine GPS — Design Spec

Standalone hiking GPS device built around the RP2040-Zero. Displays position on user-uploaded geo-referenced maps, shows altitude/compass/weather data, records tracks. Sleeps most of the time, wakes on button press. Designed to replace pulling out your phone on a hike.

## Hardware

| Part | Model | Role | Interface |
|------|-------|------|-----------|
| MCU | Waveshare RP2040-Zero | Dual Cortex-M0+ @ 133MHz, 264KB RAM, 2MB flash, USB-C | — |
| GPS | GEPRC u-blox M10 | Position, compass (magnetometer), altitude/pressure (barometer) | UART (TX/RX) |
| Display | TFT SPI 240x320 | Main screen (likely ILI9341 or ST7789, detect at init) | SPI0 (SCK, MOSI, CS, DC, RST, backlight) |
| Storage | SD card adapter + 64GB card | Maps, tracks, config | SPI0 shared bus (separate CS) |
| Charger | TP4056 with protection | LiPo charging via micro-USB | Wired between USB and battery |
| Battery | 103450 3.7V 2000mAh LiPo | Power source (7.4Wh, airline safe in carry-on) | Via TP4056 → RP2040-Zero VSYS pin |
| Input | 5 tactile buttons | Up / Down / Left / Right / Select | GPIO with internal pull-ups |

### Pin Allocation (20 header pins available)

| Pin | Function |
|-----|----------|
| GP0, GP1 | UART0 TX/RX → GPS module |
| GP2 | SPI0 SCK (shared: TFT + SD) |
| GP3 | SPI0 MOSI (shared: TFT + SD) |
| GP4 | SPI0 MISO (SD card read) |
| GP5 | TFT CS |
| GP6 | SD card CS |
| GP7 | TFT DC (data/command) |
| GP8 | TFT RST |
| GP9 | TFT backlight (PWM for brightness) |
| GP10-GP14 | Buttons: Up, Down, Left, Right, Select |
| GP26 (ADC0) | Battery voltage via voltage divider |

Remaining pins (GP15, GP27, GP28, GP29) are spare for future use.

### Power Architecture

```
USB-C (TP4056) ──→ LiPo 3.7V (3.0V–4.2V) ──→ RP2040-Zero VSYS pin
                                                  │
                                           ME6211 LDO (onboard)
                                                  │
                                              3.3V rail → MCU, GPS, TFT, SD
```

- TP4056 handles charging with overcharge/overdischarge protection
- ME6211 LDO on RP2040-Zero accepts 2V–6V input, outputs 3.3V
- Battery monitoring: ADC reads voltage through a resistor divider (100K/100K gives half voltage to ADC)
- Low battery warning at 3.5V, forced sleep at 3.4V

## SDK & Build System

- **PlatformIO** with **Pico SDK** framework (C/C++)
- No Arduino framework

```ini
[env:rp2040zero]
platform = raspberrypi
board = waveshare_rp2040_zero
framework = pico-sdk
board_build.f_cpu = 133000000L
```

### Dual-Core Architecture

| Core | Responsibility |
|------|---------------|
| Core0 | UI rendering, SD card reads, button handling, main state machine |
| Core1 | GPS NMEA parsing, magnetometer/barometer reads, track logging |

Communication between cores via Pico SDK multicore FIFO or shared memory with spin locks.

### Libraries & Dependencies

| Component | Library | Notes |
|-----------|---------|-------|
| USB mass storage | TinyUSB (built into Pico SDK) | MSC device class |
| SD card filesystem | FatFs (Chan's) | FAT32, integrated via Pico SDK |
| GPS parsing | minmea | Lightweight C NMEA parser, no dependencies |
| Display driver | Custom | Direct SPI writes, support both ILI9341 and ST7789 init sequences |
| Graphics | Custom | Pixel, line, rect, text, bitmap rendering. No framework. |
| UI | Custom | State machine driven, minimal RAM footprint |

## Screen Views

Navigation between views using Select button (cycle) and directional buttons (interact within view).

### 1. Dashboard (default on wake)

```
┌──────────────────────┐
│  ALT: 2,100m         │
│  HDG: 247° WSW  ↗    │
│  PRESS: 1013 hPa     │
│  SPD: 3.2 km/h       │
│                       │
│  LAT: 46.4521° N     │
│  LON: 10.8834° E     │
│                       │
│  BAT: 78%  GPS: 3D   │
│  12:51       5 sats   │
└──────────────────────┘
```

Shows: altitude, compass heading with arrow, barometric pressure, speed, coordinates, battery level, GPS fix quality, time from GPS, satellite count.

### 2. Map View

```
┌──────────────────────┐
│                       │
│   [map tile area]     │
│                       │
│         ◉ ←you       │
│        ↗  ←heading   │
│                       │
│                       │
│  zoom: 3  ALT: 2100m │
└──────────────────────┘
```

- Displays a 240x320 viewport of the geo-referenced map image
- GPS position shown as a dot, compass heading as an arrow
- D-pad pans the viewport, Select re-centers on GPS position
- Bottom bar: zoom level indicator, altitude

### 3. Track View

```
┌──────────────────────┐
│  TRACK RECORDING  ●   │
│                       │
│  Dist:  4.7 km       │
│  Time:  1h 23m       │
│  Ascent: +340m       │
│  Descent: -120m      │
│  Avg spd: 3.4 km/h   │
│                       │
│  [breadcrumb trail]   │
│                       │
└──────────────────────┘
```

- Accumulated hike stats
- Simple breadcrumb trail plot (GPS points connected by lines, auto-scaled to fit)
- Select starts/stops track recording

### 4. Settings

- Backlight brightness (PWM)
- Sleep timeout (15s / 30s / 60s / never)
- Units (metric / imperial)
- GPS update rate
- Track log interval

## Map System

### On-device format

Maps are stored on SD as a folder per map:

```
/maps/
  trail-name/
    meta.json
    map.rgb565
```

**meta.json:**
```json
{
  "name": "Lago Benedetto Trail",
  "width": 480,
  "height": 640,
  "top_left": { "lat": 46.458, "lon": 10.879 },
  "bottom_right": { "lat": 46.445, "lon": 10.892 }
}
```

**map.rgb565:** Raw pixel data, 2 bytes per pixel (RGB565 format), row-major. No compression, no headers — firmware can seek directly to any row.

This allows the firmware to read a 240x320 viewport by seeking to the correct offset and reading 240×2 bytes per row for 320 rows. No image decoding needed on the MCU.

### Viewport rendering (RAM-efficient)

- Do NOT load full image into RAM
- Read one row at a time (or small strip) from SD → DMA to TFT via SPI
- A 240-pixel row = 480 bytes. Read 320 rows = 153,600 reads but each is tiny
- Double-buffer with 2 row buffers (960 bytes total) for smooth DMA pipeline
- Overlay GPS dot and heading arrow after tile rows are drawn

### Coordinate mapping

To convert GPS (lat, lon) to pixel (x, y) on the map:

```
x = (lon - top_left.lon) / (bottom_right.lon - top_left.lon) * width
y = (lat - top_left.lat) / (bottom_right.lat - top_left.lat) * height
```

(Mercator distortion is negligible at hiking-trail scale)

### Companion Web App (separate repo)

A small browser-based tool for preparing map packages:
1. User drops/pastes a map screenshot
2. Clicks two corners on an interactive map to set GPS coordinates
3. App crops, resizes, converts to RGB565, generates meta.json
4. Exports a zip that user extracts to SD card

This is a separate project — not part of this firmware repo.

## Track Recording

- GPS position logged to SD card as standard GPX format
- Configurable interval (1s / 5s / 10s)
- File per day: `/tracks/2026-07-28.gpx`
- Track includes: lat, lon, altitude (from barometer, more accurate than GPS altitude), timestamp
- Recording survives sleep — GPS stays active on Core1, logs to SD

## Power Management

### Sleep States

| State | Screen | GPS | Core0 | Core1 | Current Draw (est.) |
|-------|--------|-----|-------|-------|-------------------|
| Active | On | On | Running | Running | ~80-120mA |
| Screen off | Off (backlight PWM=0) | On | Idle | Logging | ~30-40mA |
| Deep sleep | Off | Off | Dormant | Dormant | ~1-2mA |

### Wake/Sleep Logic

- Button press (any) triggers GPIO interrupt → wake from dormant
- Inactivity timeout → screen off first, then deep sleep after longer timeout
- If track recording is active, stay in "screen off" state (GPS stays on) instead of deep sleep
- Hold Select for 3 seconds → manual power off (deep sleep, GPS off)

### Battery Life Estimates (2000mAh)

| Usage Pattern | Estimated Runtime |
|--------------|-------------------|
| Screen always on, GPS active | ~16-25 hours |
| Screen on 20% of time, GPS always on (tracking) | ~40-50 hours |
| Deep sleep with occasional wake | Days to weeks |

## USB Mass Storage

- On USB-C plug-in detection (VBUS sense), firmware switches to MSC mode
- SD card exposed as a USB drive to the host PC/phone
- Device shows a simple "USB CONNECTED" screen
- On unplug, firmware reboots and resumes normal operation
- This is handled by TinyUSB MSC class built into Pico SDK

## Project Structure

```
Alpine/
├── FW/
│   ├── platformio.ini
│   ├── src/
│   │   ├── main.c              ← entry point, core0 setup + main loop
│   │   ├── core1.c             ← core1 entry, GPS + sensor loop
│   │   ├── display/
│   │   │   ├── display.h       ← init, write_pixel, fill_rect, draw_text, etc.
│   │   │   ├── display.c       ← SPI driver, supports ILI9341 + ST7789
│   │   │   └── font.h          ← embedded bitmap font
│   │   ├── gps/
│   │   │   ├── gps.h
│   │   │   └── gps.c           ← UART read + minmea parsing
│   │   ├── sensors/
│   │   │   ├── compass.h
│   │   │   ├── compass.c       ← magnetometer via GPS module
│   │   │   ├── baro.h
│   │   │   └── baro.c          ← barometer via GPS module
│   │   ├── storage/
│   │   │   ├── sd.h
│   │   │   ├── sd.c            ← FatFs SD card driver
│   │   │   ├── map_loader.h
│   │   │   ├── map_loader.c    ← map viewport reading + coordinate mapping
│   │   │   ├── track.h
│   │   │   └── track.c         ← GPX track recording
│   │   ├── ui/
│   │   │   ├── ui.h            ← view state machine
│   │   │   ├── ui.c
│   │   │   ├── view_dashboard.c
│   │   │   ├── view_map.c
│   │   │   ├── view_track.c
│   │   │   └── view_settings.c
│   │   ├── input/
│   │   │   ├── buttons.h
│   │   │   └── buttons.c       ← debounce, press/long-press detection
│   │   ├── power/
│   │   │   ├── power.h
│   │   │   └── power.c         ← battery ADC, sleep/wake, backlight PWM
│   │   └── usb/
│   │       ├── msc.h
│   │       └── msc.c           ← TinyUSB mass storage callbacks
│   ├── include/
│   │   └── config.h            ← pin definitions, constants, defaults
│   └── lib/
│       └── minmea/             ← vendored NMEA parser
├── HW/
│   └── wiring.md               ← wiring diagram / pinout reference
├── docs/
├── assets/
└── README.md
```

## Open Questions / Future Work

- Exact TFT driver chip (ILI9341 vs ST7789) — detect at init by reading chip ID register
- u-blox M10 protocol for magnetometer/barometer — may use UBX binary protocol in addition to NMEA, needs datasheet review
- Companion web app — separate project, MVP needed before map view is usable
- Enclosure / case — 3D printed or off-the-shelf project box
