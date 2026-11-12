# KNOMI V2 – Custom Firmware (BP)

Custom firmware for the **BIGTREETECH KNOMI V2** toolhead display, based on the
official BTT firmware ([bigtreetech/KNOMI](https://github.com/bigtreetech/KNOMI), branch `firmware`,
commit `0c0cda2`). It was built for a Voron ("Blackbeauty") and adds a few
quality-of-life changes on top of the stock firmware.

Current version: **BPv1.0.4**

> **KNOMI V2 only.** The changes were built and tested for the `knomiv2`
> environment. The `knomiv1` environment was not built or tested.

## Features / changes compared to stock BTT firmware

| # | Change | Details |
|---|--------|---------|
| 1 | **Rotate UI 180°** | Display is rotated in hardware (GC9A01, `setRotation(2)`), touch coordinates are mirrored. Switch on the web page or via HTTP. Setting is stored in NVS and survives reboots/updates. |
| 2 | **Animations follow the theme color** | All red GIF animations are recolored to the selected UI theme color (e.g. purple). Non-red colors (green "print ok", blue welcome) stay unchanged. With the default red theme the original GIFs are used. |
| 3 | **Standby shows only the Voron logo** | The standby GIF was removed from the idle rotation. |
| 4 | **Calmer Voron logo animation** | The last frame of the Voron logo animation is held for 5 s before it restarts (was 30 ms). |
| 5 | **Animated Voron logo on the printing screen** | The accelerometer bars on the printing screen are replaced by a small animated Voron logo (same animation, theme colored). |
| 6 | **Theme-colored values** | Progress % (printing screen) and the actual temperature on the heating nozzle / heating bed screens use the theme color instead of fixed yellow. |
| 7 | **No negative target temperatures** | The temperature setting stops at 0 °C. |
| 8 | **Print progress like Mainsail** | Progress is calculated "file-relative" like Mainsail's default: only the G-code between the first and last command counts (thumbnails and slicer config are ignored), rounded down. |
| 9 | **Version string** | `FW_VERSION` = `BPv1.0.4` (shown on the test screen). |

Details for each change: [CHANGES.md](CHANGES.md).

## Installation (OTA)

1. Download [`release/knomi2_BPv1.0.4.bin`](release/knomi2_BPv1.0.4.bin) (or build it yourself, see below).
   SHA-256: `474feeec75e91b321926a0391f9c94c421e9fee8ce44f0585169dd704cb9a488`
2. Open `http://knomi.local` (or the KNOMI's IP) in a browser.
3. Click **Update FW** at the bottom of the page and select the `.bin` file.
4. KNOMI restarts automatically with the new firmware.

If KNOMI does not boot after an update, flash the official firmware again via USB-C
(see [BTT KNOMI2 documentation](https://bttwiki.com/KNOMI2.html)).

The Klipper configuration is unchanged – use [`KNOMI.cfg`](https://github.com/bigtreetech/KNOMI/blob/master/KNOMI.cfg)
from the BTT repository (`_KNOMI_STATUS` macro).

## Usage

### Rotate the UI

* Web page `http://knomi.local` → card **UI Rotation** → button **Rotate**
* HTTP (e.g. from a script or Home Assistant):

| Request | Effect |
|---------|--------|
| `GET /display` | Returns the current state, e.g. `{"flipped":false}` |
| `GET /display?flip=1` | Rotate 180° |
| `GET /display?flip=0` | Normal orientation |
| `GET /display?flip=toggle` | Toggle |

Example: `curl "http://knomi.local/display?flip=toggle"`

### Theme color

Select the theme color on the KNOMI as before. Animations, the printing screen
logo and the value labels follow it immediately.

## Building

Toolchain: [PlatformIO](https://platformio.org/) (e.g. VS Code + PlatformIO extension).

```bash
pio run -e knomiv2                 # build
pio run -e knomiv2 -t upload       # flash via USB-C
```

The OTA binary is `.pio/build/knomiv2/firmware.bin`.

**LVGL bugfix (required, same as stock BTT firmware):** `platformio.ini` notes that
LVGL 8.3.7 needs [lvgl/lvgl#4487](https://github.com/lvgl/lvgl/pull/4487) plus
`d->prev_scr = NULL;` in `lv_disp.c`. The patched file is included in this repo as
`lv_disp_(bugfix_backup).c`. After the first build (when PlatformIO has downloaded
LVGL), copy it over the library file and build again:

```bash
cp "lv_disp_(bugfix_backup).c" .pio/libdeps/knomiv2/lvgl/src/core/lv_disp.c
pio run -e knomiv2
```

The released binary was built with: espressif32 6.4.0 (Arduino core 2.0.11),
LVGL 8.3.7 (with the bugfix above), TFT_eSPI 2.5.0, ArduinoJson 6.21.5,
ESPAsyncWebServer-esphome 3.0.0, AsyncTCP-esphome 2.0.1.

## Tools

`tools/` contains the scripts used to generate `src/gif/gif_voron_small.c`
(small Voron logo animation) from `gif_voron`. See [CHANGES.md](CHANGES.md#5-animated-voron-logo-on-the-printing-screen).

## Credits

* Original firmware, UI and animations: [BIGTREETECH](https://github.com/bigtreetech/KNOMI)
* Custom changes: [its-Blackpoint](https://github.com/its-Blackpoint)
