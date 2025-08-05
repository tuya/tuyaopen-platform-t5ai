/*
*1、30ms模型
*2、目录里面有tdnn_compute的代码，外面覆盖即可
*----
*1、变量 .wanson_asr_data1(11KB左右)/wanson_asr_data2/wanson_asr_data3（data2和data3各一半，各25KB），在ld文件里配置
*data1，有分为3部分，data11、data12、data13
*
*__attribute__((section(".wanson_asr_data1"))) uint8_t packet_buffer1[PACKET_BUFFER_SIZE];
*__attribute__((section(".wanson_asr_data2"))) uint8_t packet_buffer2[PACKET_BUFFER_SIZE];
*__attribute__((section(".wanson_asr_data3"))) uint8_t packet_buffer2[PACKET_BUFFER_SIZE];
*
*2、API .wanson_asr_code，在ld文件里配置
*__attribute__((section(".wanson_asr_code"))) int8_t api_func();
*
*3、常量 .wanson_asr_rodata，在ld文件里配置
*__attribute__((section(".wanson_asr_rodata"))) int8_t tdnn_weight7[] = {}
*
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <os/os.h>
#include <os/mem.h>

#include "common/bk_assert.h"

#if (CONFIG_WANSON_SIMD)
#include "arm_nn_compiler.h"
#endif


typedef signed char int8_t;

void asr_func_null(void){}

#if  1
void tdnn_quant(float *in_buf, int in_dim, int T, int skip,
                int8_t *out_buf, float *out_scale) {
  int i, j;
  int len = in_dim * T;
  float abs_max = 0.0f;

  for (i = 0; i < len; i++) {
    if (skip && i == in_dim) i = in_dim << 1;
    float v = fabs(in_buf[i]);
    if (v > abs_max) abs_max = v;
  }

  int factor = 127;
  float scale = factor / abs_max;
  for (i = 0, j = 0; i < len; i++, j++) {
    if (skip && i == in_dim) i = in_dim << 1;
    out_buf[j] = round(in_buf[i] * scale);
  }

  *out_scale = abs_max / factor;
}


// in_dim = 256, out_dim = 128
// in_buf大小为256, tdnn_weight大小为256乘128, float数组大小都为128

#if 1
void tdnn_compute(int8_t *in_buf, float in_scale, int in_dim, int out_dim,
                  const int8_t *tdnn_weight, const float *tdnn_scale, const float *tdnn_bias,
                  const float *tdnn_mean, const float *tdnn_var, const float *tdnn_mean_var,
                  float *out_buf, int last_layer) {
    int __maybe_unused i, j, k = 0;

    int8_t tmp_buf0[in_dim];
    int8_t tmp_buf1[in_dim];

    os_memcpy(tmp_buf0, in_buf, in_dim);

    for (i = 0; i < out_dim; i++, k++) {
        int __maybe_unused dot_prod = 0 ;
        int __maybe_unused dot_prod1 = 0 ;
        #if 1
        if (k == 4)
        {
			k = 0;
        }
		os_memcpy(tmp_buf1, tdnn_weight+i*in_dim, in_dim);

        for (j = 0; j < in_dim; j += 4)
        {
        #if (CONFIG_WANSON_SIMD)
			dot_prod = SMLAD(PKHBT(tmp_buf0[j], tmp_buf0[j + 1], 16), PKHBT(tmp_buf1[j], tmp_buf1[j + 1], 16), dot_prod);
			dot_prod = SMLAD(PKHBT(tmp_buf0[j+2], tmp_buf0[j + 3], 16), PKHBT(tmp_buf1[j+2], tmp_buf1[j + 3], 16), dot_prod);
        #else
			dot_prod += tmp_buf0[j]*tmp_buf1[j] + tmp_buf0[j+1]*tmp_buf1[j+1] + tmp_buf0[j+2]*tmp_buf1[j+2] + tmp_buf0[j+3]*tmp_buf1[j+3];
        #endif
        }
        #elif 0
        const int8_t *w = tdnn_weight + i * in_dim;
        for (j = 0; j < in_dim; j += 4)
        {
            dot_prod += tmp_buf0[j]*w[j] + tmp_buf0[j+1]*w[j+1] + tmp_buf0[j+2]*w[j+2] + tmp_buf0[j+3]*w[j+3];
        }
        #else

        #endif
        
        float z = dot_prod * in_scale * tdnn_scale[i] + tdnn_bias[i];

        if (!last_layer) {
          if (z > 0) {
            out_buf[i] = (z - tdnn_mean[i]) * tdnn_var[i];
          } else {
            out_buf[i] = tdnn_mean_var[i];
          }
        } else {
          out_buf[i] = z;
        }
    }
}



#else
void tdnn_compute(int8_t *in_buf, float in_scale, int in_dim, int out_dim,
                  const int8_t *tdnn_weight, const float *tdnn_scale, const float *tdnn_bias,
                  const float *tdnn_mean, const float *tdnn_var, const float *tdnn_mean_var,
                  float *out_buf, int last_layer) {
    int i, j;

    int8_t tmp_buf0[in_dim];
    int8_t tmp_buf1[in_dim];
    
    os_memcpy(tmp_buf0, in_buf, in_dim);
    uint64_t start_time = rtos_get_time();

    for (i = 0; i < out_dim; i++) {
        int dot_prod = 0;

        #if 1
        os_memcpy(tmp_buf1, tdnn_weight+i*in_dim, in_dim);
        for (j = 0; j < in_dim; j += 4)
        {
            dot_prod += tmp_buf0[j]*tmp_buf1[j] + tmp_buf0[j+1]*tmp_buf1[j+1] + tmp_buf0[j+2]*tmp_buf1[j+2] + tmp_buf0[j+3]*tmp_buf1[j+3];
        }
        #elif 0
        const int8_t *w = tdnn_weight + i * in_dim;
        for (j = 0; j < in_dim; j += 4)
        {
            dot_prod += tmp_buf0[j]*w[j] + tmp_buf0[j+1]*w[j+1] + tmp_buf0[j+2]*w[j+2] + tmp_buf0[j+3]*w[j+3];
        }
        #else

#if 0
        for (j = 0; j < 256; j += 2) 
        {
            uint16_t a_pack = ((uint16_t)(int8_t)tmp_buf0[j]) |
                              ((uint16_t)(int8_t)tmp_buf0[j+1] << 8);
            uint16_t w_pack = ((uint16_t)(int8_t)tmp_buf1[j]) |
                              ((uint16_t)(int8_t)tmp_buf1[j+1] << 8);

            __asm__ volatile (
                "smlad %[acc], %[a], %[b], %[acc]\n"
                : [acc] "+r" (dot_prod)
                : [a] "r" (a_pack), [b] "r" (w_pack)
            );
        }
#else

		for (j = 0; j < in_dim; j += 2) {  // 每次处理 2 个元素
					uint16_t a_pack, b_pack;
					// 将两个 int8 打包成 16 位整数
					a_pack = ((uint16_t)(int8_t)tmp_buf0[j]) | ((uint16_t)(int8_t)tmp_buf0[j + 1] << 8);
					b_pack = ((uint16_t)(int8_t)tmp_buf1[j]) | ((uint16_t)(int8_t)tmp_buf1[j + 1] << 8);
					// 使用 smlad 指令计算点积
					__asm volatile (
						"smlad %[acc], %[a], %[b], %[acc]\n"
						: [acc] "+r" (dot_prod)
						: [a] "r" (a_pack), [b] "r" (b_pack)
					);
		}

#endif
		
        #endif
        
        float z = dot_prod * in_scale * tdnn_scale[i] + tdnn_bias[i];

        if (!last_layer) {
          if (z > 0) {
            out_buf[i] = (z - tdnn_mean[i]) * tdnn_var[i];
          } else {
            out_buf[i] = tdnn_mean_var[i];
          }
        } else {
          out_buf[i] = z;
        }
    }
    uint64_t stop_time = rtos_get_time();
    tdnn_pot_t = (uint32_t)(stop_time - start_time);
}
#endif

#else
void tdnn_quant(float *in_buf, int in_dim, int T, int skip,
                int8_t *out_buf, float *out_scale) {
  int i, j;
  int len = in_dim * T;
  float abs_max = 0.0f;

  for (i = 0; i < len; i++) {
    if (skip && i == in_dim) i = in_dim << 1;
    float v = fabs(in_buf[i]);
    if (v > abs_max) abs_max = v;
  }

  int factor = 127;
  float scale = factor / abs_max;
  for (i = 0, j = 0; i < len; i++, j++) {
    if (skip && i == in_dim) i = in_dim << 1;
    out_buf[j] = round(in_buf[i] * scale);
  }

  *out_scale = abs_max / factor;
}

void tdnn_compute(int8_t *in_buf, float in_scale, int in_dim, int out_dim,
                  const int8_t *tdnn_weight, const float *tdnn_scale, const float *tdnn_bias,
                  const float *tdnn_mean, const float *tdnn_var, const float *tdnn_mean_var,
                  float *out_buf, int last_layer) {
  int i, j;
  
  for (i = 0; i < out_dim; i++) {
    int dot_prod = 0;
    const int8_t *w = tdnn_weight + i * in_dim;
    for (j = 0; j < in_dim; j++) {
      dot_prod += in_buf[j] * w[j];
    }
    float z = dot_prod * in_scale * tdnn_scale[i] + tdnn_bias[i];
    if (!last_layer) {
      if (z > 0) {
        out_buf[i] = (z - tdnn_mean[i]) * tdnn_var[i];
      } else {
        out_buf[i] = tdnn_mean_var[i];
      }
    } else {
      out_buf[i] = z;
    }
  }
}
#endif