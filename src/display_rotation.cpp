#include <Arduino.h>
#include <Preferences.h>
#include "display_rotation.h"
#include "lvgl_hal.h"

#define ROT_NVS_NAMESPACE "knomi_disp"
#define ROT_NVS_KEY       "flip"

// TFT_eSPI GC9A01: 0 = portrait, 2 = inverted portrait (MADCTL MX|MY)
#define ROT_NORMAL  0
#define ROT_FLIPPED 2

static volatile bool rot_applied   = false; // state currently on the display
static volatile bool rot_requested = false; // state requested by user
static volatile bool rot_pending   = false; // change waiting for LVGL task

void display_rotation_init(void) {
    Preferences prefs;
    if (prefs.begin(ROT_NVS_NAMESPACE, true)) { // read-only
        rot_requested = prefs.getBool(ROT_NVS_KEY, false);
        prefs.end();
    } else {
        rot_requested = false;
    }
    rot_applied = rot_requested;
    rot_pending = false;
    Serial.printf("display rotation: %s\r\n", rot_applied ? "180" : "0");
}

bool display_rotation_is_flipped(void) {
    return rot_applied;
}

bool display_rotation_requested(void) {
    return rot_requested;
}

void display_rotation_request(bool flipped) {
    rot_requested = flipped;
    rot_pending = true;
}

void display_rotation_loop(void) {
    if (!rot_pending) return;
    rot_pending = false;

    bool target = rot_requested;
    if (target == rot_applied) return;

    // Runs in the LVGL task: no flush is active between lv_timer_handler() calls,
    // so it is safe to touch the SPI display here.
    tft_gc9a01.setRotation(target ? ROT_FLIPPED : ROT_NORMAL);
    rot_applied = target;

    // Force a full redraw in the new orientation
    lv_obj_invalidate(lv_scr_act());

    Preferences prefs;
    if (prefs.begin(ROT_NVS_NAMESPACE, false)) {
        prefs.putBool(ROT_NVS_KEY, target);
        prefs.end();
    }
    Serial.printf("display rotation changed: %s\r\n", target ? "180" : "0");
}
