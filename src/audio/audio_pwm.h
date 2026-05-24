#pragma once
#include <stdint.h>

// GP26（L）/ GP27（R）を PWM+DMA で鳴らす初期化。
void audio_pwm_init(void);

// S16 インターリーブステレオサンプル配列（[L,R,L,R,...]）を DMA キューに積む。
// n_pairs: L/R ペアの数（= AUDIO_SAMPLES_TOTAL / 2）
void audio_pwm_submit(const int16_t *samples, int n_pairs);

// DMA IRQ と PWM を停止する。dormant 前に呼ぶこと。
void audio_pwm_stop(void);
