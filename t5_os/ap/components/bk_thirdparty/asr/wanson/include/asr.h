#ifndef ASR_FN_ASR_H_
#define ASR_FN_ASR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "fst_graph.h"
#include "fst_words.h"

// Return value   : 0 - OK, -1 - Error
int  Wanson_ASR_Init();

void Wanson_ASR_Reset();

/*****************************************
* Input:
*      - buf      : Audio data (16k, 16bit, mono) 
*      - buf_len  : Now must be 480 (30ms) 
*
* Output:
*      - text     : The text of ASR 
*      - score    : The confidence of ASR (Now not used)
*
* Return value    :  0 - No result
*                    1 - Has result 
*                   -1 - Error
******************************************/
int  Wanson_ASR_Recog(short *buf, int buf_len, const char **text, float *score);

void Wanson_ASR_Release();

#if (CONFIG_WANSON_NEW_LIB_SEG)
typedef signed char int8_t;
extern void asr_func_null(void);

extern void tdnn_quant(float *in_buf, int in_dim, int T, int skip, int8_t *out_buf, float *out_scale);
extern void tdnn_compute(int8_t *in_buf, float in_scale, int in_dim, int out_dim,
                  const int8_t *tdnn_weight, const float *tdnn_scale, const float *tdnn_bias,
                  const float *tdnn_mean, const float *tdnn_var, const float *tdnn_mean_var,
                  float *out_buf, int last_layer);

#endif

#ifdef __cplusplus
}
#endif

#endif
