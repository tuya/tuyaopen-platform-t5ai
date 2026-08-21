/**
 * @file mt_fft.c
 * @brief
 *
 * @copyright Copyright (c) 2025 Tuya Inc. All Rights Reserved.
 *
 * Permission is hereby granted, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), Under the premise of complying
 * with the license of the third-party open source software contained in the software,
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software.
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Public macros -------------------------------------------------------------*/

/* Public typedefs -----------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Public function prototypes ------------------------------------------------*/

// 是否使用频谱重心（0: 峰值法, 1: 重心法）
#define USE_CENTROID 0

// ------------------------------------------------------------
// 复数结构体
typedef struct {
    double real;
    double imag;
} complex;

// ------------------------------------------------------------
// 迭代FFT（基2，输入长度必须为2的幂）
static void fft(complex *x, int n) {
    // 位逆序重排
    int j = 0;
    for (int i = 0; i < n - 1; i++) {
        if (i < j) {
            complex tmp = x[i]; x[i] = x[j]; x[j] = tmp;
        }
        int k = n >> 1;
        while (j & k) {
            j ^= k;
            k >>= 1;
        }
        j ^= k;
    }

    // 蝶形运算
    for (int len = 2; len <= n; len <<= 1) {
        double ang = -2.0 * M_PI / len;
        complex wlen = { cos(ang), sin(ang) };
        for (int i = 0; i < n; i += len) {
            complex w = { 1.0, 0.0 };
            for (int j = 0; j < len / 2; j++) {
                complex u = x[i + j];
                complex v = {
                    w.real * x[i + j + len/2].real - w.imag * x[i + j + len/2].imag,
                    w.real * x[i + j + len/2].imag + w.imag * x[i + j + len/2].real
                };
                x[i + j] = (complex){ u.real + v.real, u.imag + v.imag };
                x[i + j + len/2] = (complex){ u.real - v.real, u.imag - v.imag };
                // 更新旋转因子
                double wr = w.real * wlen.real - w.imag * wlen.imag;
                double wi = w.real * wlen.imag + w.imag * wlen.real;
                w.real = wr; w.imag = wi;
            }
        }
    }
}

// ------------------------------------------------------------
// 计算下一个2的幂
static uint32_t next_power_of_two(uint32_t n) {
    uint32_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

// ------------------------------------------------------------
// 主接口：计算中心频率
// 输入：buffer      - 16位有符号PCM（按 uint16_t* 传入，内部按 int16 解释）
//      len         - 样本点数
//      sample_rate - 采样率（Hz）
// 返回：估算的中心频率（Hz），失败返回0
int func_calc_freq(uint16_t *buffer, uint32_t len, uint32_t sample_rate)
{
    if (buffer == NULL || len < 2) return 0;

    // 1. 将输入转换为浮点复数信号（取值范围[-1,1]）
    uint32_t n = next_power_of_two(len);
    complex *x = (complex*)malloc(n * sizeof(complex));
    if (!x) return 0;

    for (uint32_t i = 0; i < len; i++) {
        /* ADC/DAC path is signed int16 centered at 0 */
        double sample = (double)((int16_t)buffer[i]) / 32768.0;
        x[i].real = sample;
        x[i].imag = 0.0;
    }
    for (uint32_t i = len; i < n; i++) {
        x[i].real = 0.0;
        x[i].imag = 0.0;
    }

    // 2. 执行FFT
    fft(x, n);

    // 3. 计算幅度谱（只用到正频率部分，即0 ~ n/2）
    double *mag = (double*)malloc((n/2 + 1) * sizeof(double));
    if (!mag) {
        free(x);
        return 0;
    }
    for (int i = 0; i <= n/2; i++) {
        mag[i] = sqrt(x[i].real * x[i].real + x[i].imag * x[i].imag);
    }

    // 4. 寻找中心频率
    int freq = 0;
#if USE_CENTROID == 0
    // 方法A：峰值法（最大幅度对应频率）
    int max_idx = 1;
    double max_mag = mag[1];
    for (int i = 2; i <= n/2; i++) {
        if (mag[i] > max_mag) {
            max_mag = mag[i];
            max_idx = i;
        }
    }
    double peak_idx = max_idx;
    if (max_idx > 0 && max_idx < n/2) {
        double y0 = mag[max_idx-1];
        double y1 = mag[max_idx];
        double y2 = mag[max_idx+1];
        double denom = y0 - 2*y1 + y2;
        if (fabs(denom) > 1e-6) {
            double p = (y0 - y2) / (2 * denom);
            peak_idx = max_idx + p;
        }
    }
    freq = (int)(peak_idx * sample_rate / n + 0.5);

#else
    // 方法B：频谱重心法（加权平均频率，适合宽谱信号）
    double sum_mag = 0.0;
    double sum_freq = 0.0;
    for (int i = 1; i <= n/2; i++) {
        double f = (double)i * sample_rate / n;
        sum_mag += mag[i];
        sum_freq += mag[i] * f;
    }
    if (sum_mag > 1e-6) {
        double centroid = sum_freq / sum_mag;
        freq = (int)(centroid + 0.5);
    }
#endif

    free(mag);
    free(x);
    return freq;
}

#if 0
int main(int argc, char *argv[])
{
    double target_freq = 3000.0;
    uint32_t num_samples = 8192;
    int opt;

    // 解析命令行参数
    while ((opt = getopt(argc, argv, "f:n:")) != -1) {
        switch (opt) {
            case 'f':
                target_freq = atof(optarg);
                break;
            case 'n':
                num_samples = atoi(optarg);
                if (num_samples < 2) num_samples = 2;
                break;
            default:
                fprintf(stderr, "用法: %s [-f 频率] [-n 样本点数]\n", argv[0]);
                return 1;
        }
    }

    const double sample_rate = 44100.0;

    uint16_t *buffer = (uint16_t*)malloc(num_samples * sizeof(uint16_t));
    if (!buffer) {
        printf("内存分配失败\n");
        return -1;
    }

    printf("生成正弦波: 频率 = %.1f Hz, 采样率 = %.0f Hz, 样本数 = %u\n",
           target_freq, sample_rate, num_samples);
    for (uint32_t i = 0; i < num_samples; i++) {
        double t = (double)i / sample_rate;
        double sample = sin(2.0 * M_PI * target_freq * t);
        uint16_t value = (uint16_t)((sample + 1.0) * 32767.5);
        buffer[i] = value;
    }

    int fmax = target_freq + target_freq / 50;
    int fmin = target_freq - target_freq / 50;
    int detected_freq = func_calc_freq(buffer, num_samples, (uint32_t)sample_rate);
    printf("检测到的频率: %d Hz, expect %d - %d\n", detected_freq, fmin, fmax);
    if ((abs(detected_freq) <= fmax) && (abs(detected_freq) >= fmin))
        printf("测试通过 ✓\n");
    else
        printf("测试失败 ✗ (偏差较大)\n");

    free(buffer);
    return 0;
}
#endif
