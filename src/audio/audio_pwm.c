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
// wrap=3814 → 125MHz/3814 ≈ 32774Hz DREQ ≈ AUDIO_SAMPLE_RATE
#define TIMER_PWM_SLICE  4
#define TIMER_WRAP       3814

// 10bit 解像度 → キャリア 125MHz/1024 ≈ 122kHz（可聴域外）
#define AUDIO_WRAP       1023

// 1フレーム分の L/R ペア数（32768Hz / ~59.73fps ≈ 548）
#define DMA_BUF_SAMPLES  548

static uint32_t       dma_buf[2][DMA_BUF_SAMPLES];
static int            dma_chan    = -1;
static volatile int   dma_play   = 0;     // DMA が現在再生中のバッファ番号
static volatile bool  buf_needs_fill = false;

// DMA 完了 IRQ: ブロッキングなしで即座に交互バッファへ切り替える
// FatFS SPI ドライバも DMA_IRQ_1 を共有しているため、自チャンネルのみ処理する
static void __isr audio_dma_irq(void) {
    if (!dma_channel_get_irq1_status(dma_chan)) return;
    dma_channel_acknowledge_irq1(dma_chan);
    dma_play ^= 1;
    dma_channel_set_read_addr(dma_chan, dma_buf[dma_play], false);
    dma_channel_set_trans_count(dma_chan, DMA_BUF_SAMPLES, true);  // trigger
    buf_needs_fill = true;
}

static inline uint16_t s16_to_pwm(int16_t s)
{
    // S16 [-32768, 32767] → 10bit [0, 1023]
    return (uint16_t)((s + 32768) >> 6);
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

    // DMA_IRQ_1 を共有ハンドラとして登録（FatFS SPI ドライバが同 IRQ を使うため）
    dma_channel_set_irq1_enabled(dma_chan, true);
    irq_add_shared_handler(DMA_IRQ_1, audio_dma_irq, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_1, true);

    dma_channel_start(dma_chan);
}

void audio_pwm_stop(void) {
    if (dma_chan < 0) return;
    dma_channel_set_irq1_enabled(dma_chan, false);
    dma_channel_abort(dma_chan);
    pwm_set_enabled(AUDIO_PWM_SLICE, false);
    pwm_set_enabled(TIMER_PWM_SLICE, false);
}

void audio_pwm_submit(const int16_t *samples, int n_pairs)
{
    if (!buf_needs_fill) return;

    // 現在 DMA が再生していない側のバッファへ書く
    int fill = 1 - dma_play;
    buf_needs_fill = false;

    if (n_pairs > DMA_BUF_SAMPLES) n_pairs = DMA_BUF_SAMPLES;
    for (int i = 0; i < n_pairs; i++) {
        uint32_t l = s16_to_pwm(samples[i * 2]);
        uint32_t r = s16_to_pwm(samples[i * 2 + 1]);
        dma_buf[fill][i] = (r << 16) | l;
    }
}
