#include "menu.h"
#include "video/video_lcd.h"
#include "input/input_keyboard.h"
#include <stdio.h>
#include <string.h>

#define N_ITEMS 6

static bool    s_open    = false;
static int     s_cursor  = 0;
static bool    s_confirm = false;

static uint8_t s_palette;
static uint8_t s_audio_en;
static uint8_t s_backlight;

// パレットプレビュー用: menu_open 時のフレームバッファ参照（Core 0 はポーズ中に変更しない）
static const uint8_t (*s_fb)[160];

static const uint8_t BL_PRESETS[] = {64, 128, 192, 255};
#define N_BL_PRESETS 4

static char s_labels[N_ITEMS][40];
static const char *s_label_ptrs[N_ITEMS];

static void build_labels(void) {
    snprintf(s_labels[0], sizeof(s_labels[0]), "Palette : %s", lcd_palette_name(s_palette));
    snprintf(s_labels[1], sizeof(s_labels[1]), "Audio   : %s", s_audio_en ? "ON" : "OFF");
    snprintf(s_labels[2], sizeof(s_labels[2]), "Backlight: %d", s_backlight);
    snprintf(s_labels[3], sizeof(s_labels[3]), "Backup to SD");
    snprintf(s_labels[4], sizeof(s_labels[4]), "Restore from SD");
    snprintf(s_labels[5], sizeof(s_labels[5]), "Clear Flash");
    for (int i = 0; i < N_ITEMS; i++) s_label_ptrs[i] = s_labels[i];
}

void menu_open(const uint8_t fb[144][160],
               uint8_t palette, uint8_t audio_en, uint8_t backlight) {
    s_fb        = fb;
    s_palette   = palette;
    s_audio_en  = audio_en;
    s_backlight = backlight;
    s_cursor    = 0;
    s_confirm   = false;
    s_open      = true;

    build_labels();
    lcd_menu_draw(s_label_ptrs, N_ITEMS, s_cursor);
}

bool menu_is_open(void)          { return s_open; }
uint8_t menu_get_palette(void)       { return s_palette;   }
uint8_t menu_get_audio_enabled(void) { return s_audio_en;  }
uint8_t menu_get_backlight(void)     { return s_backlight;  }

static menu_action_t handle_confirm(int key) {
    if (key == ',' || key == '[' || key == KEY_ENTER) {
        s_confirm = false;
        s_open    = false;
        return MENU_ACT_FLASH_CLEAR_EXEC;
    }
    if (key == '.' || key == ']' || key == KEY_ESC) {
        s_confirm = false;
        lcd_menu_draw(s_label_ptrs, N_ITEMS, s_cursor);
    }
    return MENU_ACT_NONE;
}

static menu_action_t handle_select(int idx) {
    switch (idx) {
        case 0: {
            s_palette = (uint8_t)((s_palette + 1) % lcd_palette_count());
            lcd_palette_set(s_palette);
            // ゲーム画面を新パレットで再描画してからメニューを重ねる
            lcd_gb_frame_redraw(s_fb);
            build_labels();
            lcd_menu_draw(s_label_ptrs, N_ITEMS, s_cursor);
            return MENU_ACT_NONE;
        }
        case 1: {
            s_audio_en ^= 1;
            build_labels();
            lcd_menu_item_redraw(s_labels[1], 1, true);
            return MENU_ACT_NONE;
        }
        case 2: {
            int pi = 0;
            for (int i = 0; i < N_BL_PRESETS; i++)
                if (BL_PRESETS[i] == s_backlight) { pi = i; break; }
            pi = (pi + 1) % N_BL_PRESETS;
            s_backlight = BL_PRESETS[pi];
            build_labels();
            lcd_menu_item_redraw(s_labels[2], 2, true);
            return MENU_ACT_NONE;
        }
        case 3:
            return MENU_ACT_SRAM_TO_SD;
        case 4:
            return MENU_ACT_SD_TO_FLASH;
        case 5:
            s_confirm = true;
            lcd_menu_draw_confirm();
            return MENU_ACT_NONE;
    }
    return MENU_ACT_NONE;
}

menu_action_t menu_tick(int key) {
    if (!s_open) return MENU_ACT_NONE;

    if (s_confirm) return handle_confirm(key);

    if (key == KEY_UP) {
        int prev = s_cursor;
        s_cursor = (s_cursor - 1 + N_ITEMS) % N_ITEMS;
        lcd_menu_item_redraw(s_labels[prev],     prev,     false);
        lcd_menu_item_redraw(s_labels[s_cursor], s_cursor, true);
    } else if (key == KEY_DOWN) {
        int prev = s_cursor;
        s_cursor = (s_cursor + 1) % N_ITEMS;
        lcd_menu_item_redraw(s_labels[prev],     prev,     false);
        lcd_menu_item_redraw(s_labels[s_cursor], s_cursor, true);
    } else if (key == ',' || key == '[' || key == KEY_ENTER) {
        return handle_select(s_cursor);
    } else if (key == '.' || key == ']' || key == KEY_ESC) {
        s_open = false;
        return MENU_ACT_CLOSE;
    }

    return MENU_ACT_NONE;
}
