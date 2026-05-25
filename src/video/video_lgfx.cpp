#include "lgfx_config.hpp"
#include "video_lcd.h"
#include <string.h>

static LGFX_PicoCalc lcd;

// DMG クラシックグリーンパレット (R, G, B 各 8bit)
static const uint8_t dmg_palette[4][3] = {
    {0x9B, 0xBC, 0x0F},
    {0x8B, 0xAC, 0x0F},
    {0x30, 0x62, 0x30},
    {0x0F, 0x38, 0x0F},
};

// LCD 上での GB 画面オフセット: (320 - 144×2) / 2 = 16
#define GB_Y_OFF  16

// 差分検出用前フレームバッファ
static uint8_t prev_fb[144][160];

// 2x スケール行バッファ (320 pixels, RGB565)
static uint16_t row_buf[320];

// RGB565 パレット (lgfx::color565 で生成、writePixels swap=true で正しく転送)
static uint16_t dmg_pal565[4];

// ─── ステータスバー定数 ───────────────────────────────────────────────────────
// 上部: y=0..15 (16px), 下部: y=304..319 (16px)
#define STATUS_BOTTOM_Y   304
#define STATUS_BOTTOM_BG  lcd.color888(24, 24, 24)
#define STATUS_HINT_FG    lcd.color888(128, 128, 128)

// Font0 (6×8) textSize=1 → 6×8px/char → 320/6 = 53文字/行
// ヒントテキストは 53 文字以内に収める（16px ストリップに Y+4 で縦中央揃え）
static const char HINTS[] = ",/[=A  ./]=B  BS=Sta  Del=Sel";

void lcd_init(void) {
    lcd.init();
    lcd.setRotation(6);  // MADCTL: MX=0,MY=0 → ミラーなし正常ポートレート

    for (int i = 0; i < 4; i++) {
        dmg_pal565[i] = lgfx::color565(dmg_palette[i][0], dmg_palette[i][1], dmg_palette[i][2]);
    }

    // 初回は全画面再描画を強制
    memset(prev_fb, 0xFF, sizeof(prev_fb));

    lcd.setFont(&lgfx::fonts::Font0);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.setCursor(0, 0);
}

void lcd_clear(void) {
    lcd.fillScreen(TFT_BLACK);
    lcd.setFont(&lgfx::fonts::Font0);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.setCursor(0, 0);
}

void lcd_print_string(const char *s) {
    lcd.setFont(&lgfx::fonts::Font0);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.print(s);
}

// GB 画面を 2x スケールで LCD に差分描画する
void lcd_gb_frame_delta(const uint8_t fb[144][160]) {
    lcd.startWrite();

    int y = 0;
    while (y < 144) {
        if (memcmp(fb[y], prev_fb[y], 160) == 0) {
            y++;
            continue;
        }

        // 連続する変化行をまとめて転送
        int ys = y;
        while (y < 144 && memcmp(fb[y], prev_fb[y], 160) != 0)
            y++;

        // LCD ウィンドウ設定 (2x スケール)
        lcd.setWindow(0, ys * 2 + GB_Y_OFF, 319, (y - 1) * 2 + 1 + GB_Y_OFF);

        for (int row = ys; row < y; row++) {
            // 1行を 2x 水平スケールでバッファに展開
            for (int x = 0; x < 160; x++) {
                uint16_t c = dmg_pal565[fb[row][x] & 3];
                row_buf[x * 2]     = c;
                row_buf[x * 2 + 1] = c;
            }
            // 同じ行を 2 回送信（2x 垂直スケール）; swap=true で rgb565_t レイアウトとして転送
            lcd.writePixels(row_buf, 320, true);
            lcd.writePixels(row_buf, 320, true);
        }
    }

    lcd.endWrite();
    memcpy(prev_fb, fb, sizeof(prev_fb));
}

// セーブステートロード後などに全画面強制再描画
void lcd_gb_frame_invalidate(void) {
    memset(prev_fb, 0xFF, sizeof(prev_fb));
}

// ─── ステータスバー ─────────────────────────────────────────────────────────

void lcd_status_draw_hints(void) {
    lcd.startWrite();
    lcd.fillRect(0, STATUS_BOTTOM_Y, 320, 16, STATUS_BOTTOM_BG);
    lcd.setFont(&lgfx::fonts::Font0);
    lcd.setTextSize(1);
    lcd.setTextColor(STATUS_HINT_FG, STATUS_BOTTOM_BG);
    lcd.setCursor(2, STATUS_BOTTOM_Y + 4);  // 8px フォントを 16px ストリップ内で縦中央に
    lcd.print(HINTS);
    lcd.endWrite();
}

void lcd_status_storage_icon(storage_icon_t state) {
    uint32_t color;
    switch (state) {
        case STORAGE_ICON_READ:  color = lcd.color888(255, 255, 255); break;  // 白: 読み込み
        case STORAGE_ICON_SD:    color = lcd.color888(64,  160, 255); break;  // 青: SD 書き込み
        case STORAGE_ICON_FLASH: color = lcd.color888(255, 200,   0); break;  // 黄: Flash 書き込み
        default:                 color = TFT_BLACK;                   break;  // 消灯
    }
    lcd.fillRect(310, 2, 8, 12, color);
}

void lcd_status_top_text(const char *msg) {
    lcd.startWrite();
    lcd.setFont(&lgfx::fonts::Font0);
    lcd.setTextSize(1);
    // 8文字 × 6px = 48px の領域をクリアしてから描画
    lcd.fillRect(0, 0, 50, 16, TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setCursor(0, 4);  // 8px フォントを 16px ストリップ内で縦中央に
    lcd.print(msg);
    lcd.endWrite();
}

// 上部 16px ストリップ右側（ストレージアイコン左隣）にバッテリー残量を表示する。
// pct: 0-100 = 残量%、-1 = 不明
void lcd_status_battery(int pct) {
    static int prev_pct = -2;
    if (pct == prev_pct) return;
    prev_pct = pct;

    char buf[5];
    uint32_t fg;
    if (pct < 0) {
        strcpy(buf, " -- ");
        fg = lcd.color888(128, 128, 128);
    } else {
        snprintf(buf, sizeof(buf), "%3d%%", pct);
        if (pct > 50)      fg = lcd.color888(0, 220, 64);
        else if (pct > 20) fg = lcd.color888(255, 200, 0);
        else               fg = lcd.color888(255, 64, 64);
    }

    // 4文字 × 6px = 24px、ストレージアイコン(x=310)の4px手前
    lcd.startWrite();
    lcd.setFont(&lgfx::fonts::Font0);
    lcd.setTextSize(1);
    lcd.fillRect(282, 0, 24, 16, TFT_BLACK);
    lcd.setTextColor(fg, TFT_BLACK);
    lcd.setCursor(282, 4);
    lcd.print(buf);
    lcd.endWrite();
}
