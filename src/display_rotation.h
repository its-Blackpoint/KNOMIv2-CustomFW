#ifndef DISPLAY_ROTATION_H
#define DISPLAY_ROTATION_H

// UI 180° rotation for KNOMI
// - Setting is stored in NVS (namespace "knomi_disp"), separate from the
//   EEPROM config struct, so existing configs stay compatible.
// - Display is rotated in hardware (GC9A01 MADCTL via TFT_eSPI setRotation),
//   touch coordinates are mirrored to match.

// Load stored setting. Call once before lvgl_hal_init().
void display_rotation_init(void);

// Currently applied state (used by display/touch driver).
bool display_rotation_is_flipped(void);

// Request a new state. Safe to call from any task (e.g. web server).
// The change is applied and saved by display_rotation_loop() in the LVGL task.
void display_rotation_request(bool flipped);

// Requested state (what the web page should show).
bool display_rotation_requested(void);

// Call in the LVGL task loop, before lv_timer_handler().
void display_rotation_loop(void);

#endif
