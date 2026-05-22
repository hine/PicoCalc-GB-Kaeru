#pragma once
// RP2350 向け hardware/rtc.h スタブ。
// RP2350 には RTC ハードウェアがないため no-op 実装。
// FatFs_SPI の rtc.c がタイムスタンプ 0 を返す形でコンパイルされる。
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t year;
    int8_t  month;
    int8_t  day;
    int8_t  dotw;
    int8_t  hour;
    int8_t  min;
    int8_t  sec;
} datetime_t;

static inline void rtc_init(void) {}
static inline bool rtc_get_datetime(datetime_t *t) { (void)t; return false; }
static inline bool rtc_set_datetime(const datetime_t *t) { (void)t; return false; }
static inline bool rtc_running(void) { return false; }
