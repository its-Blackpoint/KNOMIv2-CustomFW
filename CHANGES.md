# Changes compared to the stock BTT firmware

Base: [bigtreetech/KNOMI](https://github.com/bigtreetech/KNOMI) branch `firmware`, commit `0c0cda2`
(stock version `V1.0.2`). Only the files listed per change were modified.

## 1. Rotate UI 180°

**Files:** `src/display_rotation.h`, `src/display_rotation.cpp` (new), `src/lvgl_hal.h`,
`src/lvgl_hal.cpp`, `src/lvgl_usr.cpp`, `src/webserver.cpp`, `src/index_html.h`

* The GC9A01 is rotated in hardware via TFT_eSPI `setRotation(2)` (MADCTL MX|MY),
  no CPU cost.
* Touch coordinates from the CST816S are mirrored (`239 - x`, `239 - y`) while
  rotated. Swipe gestures are calculated by LVGL from these coordinates, so they
  stay correct.
* The setting is stored in NVS (`Preferences`, namespace `knomi_disp`, key `flip`),
  separate from BTT's EEPROM config struct, so existing WiFi/Moonraker settings
  stay compatible.
* A change requested by the web server is applied in the LVGL task
  (`display_rotation_loop()`), the only task that accesses the display SPI bus.
* New HTTP endpoint `GET /display[?flip=0|1|toggle]`, new card "UI Rotation" on
  the web page.

## 2. Animations follow the theme color

**Files:** `src/gif_theme.h`, `src/gif_theme.cpp` (new), `src/lvgl_usr.cpp`,
`src/ui_overlay/lv_overlay.h`, `src/ui_overlay/lv_moonraker_change_screen.cpp`,
`src/ui_overlay/lv_auto_goto_idle.cpp`

* All stock GIFs use one global color table and no local color tables, so they
  can be recolored by changing the palette only – no re-encoding.
* `gif_theme_set_src()` replaces `lv_gif_set_src()` for the main GIF. With a
  non-default theme the GIF is copied to PSRAM and every palette entry within
  ±40° hue of BTT red (`0xC02F30`) with saturation ≥ 0.2 is shifted to the theme
  hue; saturation and brightness are scaled relative to BTT red. Black, white,
  grey and other colors stay untouched.
* With the default red theme the original GIF from flash is used unchanged.
* `gif_theme_loop()` (LVGL task) re-applies the GIF of every tracked object when
  the theme color changes – this also covers the theme being loaded from EEPROM
  after the UI has started.

## 3. Standby shows only the Voron logo

**File:** `src/ui_overlay/lv_moonraker_change_screen.cpp`

* `gif_idle[]` contains only `gif_voron` (standby GIF removed).
* The 7 s idle GIF rotation only runs if there is more than one idle GIF, so the
  Voron logo is not restarted every 7 s.

## 4. Calmer Voron logo animation

**File:** `src/gif/gif_voron.c`

* Delay of the last frame (Graphic Control Extension) changed from 3 to 500
  centiseconds: the logo is built up in 1.77 s, then held for 5 s.

## 5. Animated Voron logo on the printing screen

**Files:** `src/gif/gif_voron_small.c` (new), `src/lvgl_usr.cpp`, `src/ui_overlay/lv_overlay.h`,
`src/gif_theme.h`, `src/gif_theme.cpp`, `tools/` (new)

* The accelerometer bars and X/Y/Z labels on `ScreenPrinting` are hidden. The
  accelerometer itself stays active (sensor test screen still works).
* A small animated Voron logo (`lv_gif`, 75×71 px, 23 KB) is shown in their place.
  It is a scaled copy of `gif_voron` with the same frames and timing, including
  the 5 s hold. Using a pre-scaled GIF avoids scaling every frame at runtime.
* `gif_theme` tracks several GIF objects, so both animations are recolored.
* Regenerate `gif_voron_small.c` (Python 3 + Pillow):

  ```bash
  # gif_voron.gif = the bytes of gif_voron_map[] from src/gif/gif_voron.c
  python3 tools/make_small_voron.py gif_voron.gif gif_voron_small_raw.gif
  python3 tools/strip_lct.py gif_voron_small_raw.gif gif_voron_small.gif
  ```

  `strip_lct.py` removes the per-frame color tables Pillow writes (they are
  identical to the global one), which is required for the theme recoloring. The
  result is then converted to a C array in the same format as the other GIFs.

## 6. Theme-colored value labels

**File:** `src/ui_overlay/lv_theme_color.cpp`

* `ui_label_printing_progress`, `ui_label_heating_nozzle_actual` and
  `ui_label_heating_bed_actual` (stock: yellow `0xFFD164`) use the theme color.
* Other yellow labels (temperature menu, extrude, info, sensor test) are unchanged.

## 7. No negative target temperatures

**File:** `src/ui_overlay/lv_set_temp.cpp`

* Decreasing the target stops at 0 °C; the dial does not rotate further at 0.

## 8. Print progress like Mainsail ("file-relative")

**Files:** `src/moonraker.cpp`, `src/moonraker.h`

* Stock firmware used `virtual_sdcard.progress` = `file_position / file_size`,
  which includes thumbnails and slicer config and therefore differs from Mainsail.
* Now: once per print, `GET /server/files/metadata?filename=<print_stats.filename>`
  ([Moonraker API](https://github.com/Arksine/moonraker/blob/master/docs/external_api/file_manager.md#get-gcode-metadata))
  provides `gcode_start_byte` / `gcode_end_byte`; progress is
  `(file_position − gcode_start_byte) / (gcode_end_byte − gcode_start_byte)`,
  clamped to 0–100 % and rounded down – the same as Mainsail's default
  `file-relative` mode.
* If the metadata is not available, it falls back to `virtual_sdcard.progress`
  (same as Mainsail) and retries every 10 s.
* File names are percent-encoded (folders, spaces, umlauts, `+`).
* Note: KlipperScreen shows a time-based progress and will still differ.

## 9. KNOMI V1 support removed

**Files:** `platformio.ini`, `src/pinout.h`, `src/pinout_knomi_v1.h` (deleted),
`buildroot/boards/esp32r8.json` (deleted)

* Build environment `knomiv1`, its board definition and pinout removed;
  `pinout.h` includes the V2 pinout only.

## 10. Version

**File:** `src/config.h`

* `FW_VERSION` = `BPv1.0.4`.
