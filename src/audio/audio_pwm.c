#include "audio_pwm.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

// GP26 = PWM slice 5A (L), GP27 = PWM slice 5B (R)
#define AUDIO_PWM_SLICE  5
#define AUDIO_PIN_L      26
#define AUDIO_PIN_R      27

// slice 4: DMA ペーサー専用（GPIO 出力なし）
// RP2350 デフォルト 150MHz: wrap=4576 → 150MHz/4577 ≈ 32773Hz DREQ ≈ AUDIO_SAMPLE_RATE(32768Hz)
// ※ 125MHz 前提の 3814 では 39319Hz となり音が ~2.4 半音高くなる
#define TIMER_PWM_SLICE  4
#define TIMER_WRAP       4576

// 12bit 解像度 → キャリア 150MHz/4096 ≈ 36.6kHz（可聴域外）
#define AUDIO_WRAP       4095

// 548 サンプル ≈ 33ms バッファ（GB_AUDIO_SAMPLES と一致させること）
#define DMA_BUF_SAMPLES  548

static uint32_t       dma_buf[2][DMA_BUF_SAMPLES];
static int            dma_chan    = -1;
static volatile int   dma_play   = 0;

static audio_fill_cb_t fill_cb = NULL;

void audio_pwm_set_fill_cb(audio_fill_cb_t cb) {
    fill_cb = cb;
}

// DMA 完了 IRQ: 次バッファへ切り替え → fill_cb で即座に補充
// FatFS が DMA_IRQ_1 を使うため DMA_IRQ_0 を使用
static void __isr audio_dma_irq(void) {
    if (!dma_channel_get_irq0_status(dma_chan)) return;
    dma_channel_acknowledge_irq0(dma_chan);
    dma_play ^= 1;
    dma_channel_set_read_addr(dma_chan, dma_buf[dma_play], false);
    dma_channel_set_trans_count(dma_chan, DMA_BUF_SAMPLES, true);
    int fill = 1 - dma_play;
    if (fill_cb) fill_cb(dma_buf[fill], DMA_BUF_SAMPLES);
}

void audio_pwm_init(void)
{
    // PWM slice 5: 音声出力（10bit, ~122kHz キャリア）
    gpio_set_function(AUDIO_PIN_L, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PIN_R, GPIO_FUNC_PWM);
    pwm_config acfg = pwm_get_default_config();
    pwm_config_set_wrap(&acfg, AUDIO_WRAP);
    pwm_init(AUDIO_PWM_SLICE, &acfg, true);
    pwm_set_both_levels(AUDIO_PWM_SLICE, AUDIO_WRAP / 2, AUDIO_WRAP / 2);

    // PWM slice 4: DMA ペーサー（GPIO 使用なし、DREQ のみ利用）
    pwm_config tcfg = pwm_get_default_config();
    pwm_config_set_wrap(&tcfg, TIMER_WRAP);
    pwm_init(TIMER_PWM_SLICE, &tcfg, true);

    // 無音バッファで初期化
    uint32_t silence = ((uint32_t)(AUDIO_WRAP / 2) << 16) | (AUDIO_WRAP / 2);
    for (int i = 0; i < DMA_BUF_SAMPLES; i++) {
        dma_buf[0][i] = silence;
        dma_buf[1][i] = silence;
    }

    // DMA: slice 4 DREQ でペース → slice 5 CC へ 32bit 転送
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dc = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
    channel_config_set_read_increment(&dc, true);
    channel_config_set_write_increment(&dc, false);
    channel_config_set_dreq(&dc, pwm_get_dreq(TIMER_PWM_SLICE));

    dma_channel_configure(dma_chan, &dc,
        &pwm_hw->slice[AUDIO_PWM_SLICE].cc,
        dma_buf[dma_play],
        DMA_BUF_SAMPLES,
        false);

    // DMA_IRQ_0 を使用（Core 1 に登録）
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, audio_dma_irq);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_start(dma_chan);
}

void audio_pwm_stop(void) {
    if (dma_chan < 0) return;
    dma_channel_set_irq0_enabled(dma_chan, false);
    dma_channel_abort(dma_chan);
    pwm_set_enabled(AUDIO_PWM_SLICE, false);
    pwm_set_enabled(TIMER_PWM_SLICE, false);
}
