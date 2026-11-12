#ifndef GIF_THEME_H
#define GIF_THEME_H

#include "lvgl.h"

// Theme-colored GIF animations
// The stock GIFs are drawn in BTT red. When another theme color is selected,
// a copy of the GIF is placed in PSRAM and the red tones of its global color
// table are shifted to the theme color (black/white/grey and non-red colors,
// e.g. the green "print ok", stay untouched). With the default red theme the
// original GIF from flash is used unchanged.

// Use instead of lv_gif_set_src() for theme-colored GIFs.
void gif_theme_set_src(lv_obj_t * gif, const lv_img_dsc_t * src);

// Call in the LVGL task loop. Re-applies the current GIF of every object set
// via gif_theme_set_src() when the theme color changed (color wheel, or config
// loaded from EEPROM after UI start). Each GIF source must only be shown by one
// object at a time.
void gif_theme_loop(void);

#endif
