#include <Arduino.h>
#include <esp_heap_caps.h>
#include "gif_theme.h"
#include "knomi.h"

#define GIF_THEME_CACHE_SIZE 12
#define GIF_THEME_HUE_RANGE  (40.0f / 360.0f) // +- hue range treated as "red"
#define GIF_THEME_MIN_SAT    0.20f            // below = grey/white/black, untouched

typedef struct {
    const lv_img_dsc_t * src; // original GIF in flash
    lv_img_dsc_t dsc;         // descriptor pointing to recolored copy
    uint8_t * buf;            // recolored copy in PSRAM
    uint16_t color;           // theme color (RGB565) used for buf
} gif_theme_entry_t;

static gif_theme_entry_t cache[GIF_THEME_CACHE_SIZE];
static uint8_t * retired[GIF_THEME_CACHE_SIZE];
static uint8_t retired_count = 0;

// GIF objects using themed GIFs (main GIF, small Voron logo on ScreenPrinting)
#define GIF_THEME_MAX_OBJ 4
typedef struct {
    lv_obj_t * obj;
    const lv_img_dsc_t * src;
    uint16_t color;
} gif_theme_obj_t;
static gif_theme_obj_t tracked[GIF_THEME_MAX_OBJ];

static void rgb_to_hsv(float r, float g, float b, float * h, float * s, float * v) {
    float max = r > g ? (r > b ? r : b) : (g > b ? g : b);
    float min = r < g ? (r < b ? r : b) : (g < b ? g : b);
    float d = max - min;
    *v = max;
    *s = (max <= 0.0f) ? 0.0f : d / max;
    if (d <= 0.0f) { *h = 0.0f; return; }
    float hh;
    if (max == r)      hh = (g - b) / d;
    else if (max == g) hh = (b - r) / d + 2.0f;
    else               hh = (r - g) / d + 4.0f;
    hh /= 6.0f;
    if (hh < 0.0f) hh += 1.0f;
    *h = hh;
}

static void hsv_to_rgb(float h, float s, float v, float * r, float * g, float * b) {
    float hh = h * 6.0f;
    int i = (int)hh;
    float f = hh - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    switch (i % 6) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

static uint8_t to_u8(float x) {
    if (x < 0.0f) x = 0.0f;
    if (x > 1.0f) x = 1.0f;
    return (uint8_t)(x * 255.0f + 0.5f);
}

// Shift red tones of the GIF global color table to the target color
static void gif_recolor_palette(uint8_t * gif, uint32_t size, lv_color_t target) {
    if (size < 13 || memcmp(gif, "GIF", 3) != 0) return;
    uint8_t flags = gif[10];
    if (!(flags & 0x80)) return; // no global color table
    uint32_t n = 2u << (flags & 0x07);
    if (13 + 3 * n > size) return;

    // source: BTT red the GIFs are drawn in
    float sh, ss, sv;
    rgb_to_hsv(((LV_32BIT_BTT_RED >> 16) & 0xFF) / 255.0f,
               ((LV_32BIT_BTT_RED >> 8) & 0xFF) / 255.0f,
               (LV_32BIT_BTT_RED & 0xFF) / 255.0f, &sh, &ss, &sv);
    // target: theme color
    lv_color32_t t32;
    t32.full = lv_color_to32(target);
    float th, ts, tv;
    rgb_to_hsv(t32.ch.red / 255.0f, t32.ch.green / 255.0f, t32.ch.blue / 255.0f, &th, &ts, &tv);
    if (ss <= 0.0f || sv <= 0.0f) return;

    for (uint32_t i = 0; i < n; i++) {
        uint8_t * c = &gif[13 + 3 * i];
        float h, s, v;
        rgb_to_hsv(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, &h, &s, &v);
        float dh = h - sh;
        if (dh > 0.5f)  dh -= 1.0f;
        if (dh < -0.5f) dh += 1.0f;
        if (s < GIF_THEME_MIN_SAT || fabsf(dh) > GIF_THEME_HUE_RANGE) continue;

        float nh = th + dh;
        if (nh < 0.0f)  nh += 1.0f;
        if (nh >= 1.0f) nh -= 1.0f;
        float ns = s * ts / ss; if (ns > 1.0f) ns = 1.0f;
        float nv = v * tv / sv; if (nv > 1.0f) nv = 1.0f;
        float r, g, b;
        hsv_to_rgb(nh, ns, nv, &r, &g, &b);
        c[0] = to_u8(r);
        c[1] = to_u8(g);
        c[2] = to_u8(b);
    }
}

static bool theme_is_default(void) {
    return knomi_config.theme_color.full == lv_color_hex(LV_DEFAULT_COLOR).full;
}

static void retire(uint8_t * buf) {
    if (buf == NULL) return;
    if (retired_count < GIF_THEME_CACHE_SIZE) {
        retired[retired_count++] = buf;
    }
}

static void free_retired(void) {
    for (uint8_t i = 0; i < retired_count; i++) {
        heap_caps_free(retired[i]);
        retired[i] = NULL;
    }
    retired_count = 0;
}

static const lv_img_dsc_t * gif_theme_get(const lv_img_dsc_t * src) {
    if (src == NULL) return NULL;

    gif_theme_entry_t * e = NULL;
    for (uint8_t i = 0; i < GIF_THEME_CACHE_SIZE; i++) {
        if (cache[i].src == src) { e = &cache[i]; break; }
    }

    // default theme: original GIF, drop a stale copy
    if (theme_is_default()) {
        if (e && e->buf) {
            retire(e->buf);
            e->buf = NULL;
        }
        return src;
    }

    if (e == NULL) {
        for (uint8_t i = 0; i < GIF_THEME_CACHE_SIZE; i++) {
            if (cache[i].src == NULL) { e = &cache[i]; e->src = src; break; }
        }
        if (e == NULL) return src; // cache full, use original
    }

    uint16_t color = knomi_config.theme_color.full;
    if (e->buf && e->color == color) return &e->dsc;

    uint8_t * buf = (uint8_t *)heap_caps_malloc(src->data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buf == NULL) return src; // out of memory, use original

    memcpy(buf, src->data, src->data_size);
    gif_recolor_palette(buf, src->data_size, knomi_config.theme_color);

    retire(e->buf); // may still be displayed, freed after lv_gif_set_src()
    e->buf = buf;
    e->color = color;
    e->dsc = *src;
    e->dsc.data = buf;
    return &e->dsc;
}

void gif_theme_set_src(lv_obj_t * gif, const lv_img_dsc_t * src) {
    const lv_img_dsc_t * dsc = gif_theme_get(src);
    lv_gif_set_src(gif, dsc);
    // old GIF is closed by lv_gif_set_src(), retired buffers are unused now
    free_retired();

    gif_theme_obj_t * t = NULL;
    for (uint8_t i = 0; i < GIF_THEME_MAX_OBJ; i++) {
        if (tracked[i].obj == gif) { t = &tracked[i]; break; }
    }
    if (t == NULL) {
        for (uint8_t i = 0; i < GIF_THEME_MAX_OBJ; i++) {
            if (tracked[i].obj == NULL) { t = &tracked[i]; t->obj = gif; break; }
        }
    }
    if (t) {
        t->src = src;
        t->color = knomi_config.theme_color.full;
    }
}

void gif_theme_loop(void) {
    for (uint8_t i = 0; i < GIF_THEME_MAX_OBJ; i++) {
        if (tracked[i].obj == NULL || tracked[i].src == NULL) continue;
        if (knomi_config.theme_color.full == tracked[i].color) continue;
        gif_theme_set_src(tracked[i].obj, tracked[i].src);
    }
}
