
#ifndef __AUDIO_SUBSYS_RNN_VAD_H__
#define __AUDIO_SUBSYS_RNN_VAD_H__

#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rnn_vad_param_in {
    float min_speech_len;    //最短语音长度(ms)，太小会导致误报,默认200ms
    float max_speech_interval; //最大语音间隔(ms) 太小会导致断句过快，默认1000ms
} rnn_vad_param_in;

void *rnn_vad_create();

int rnn_vad_init(rnn_vad_param_in* param_in, void* obj);

float rnn_vad_process(void *obj, short *x);

void rnn_vad_destroy(void *obj);

void rnn_vad_start(void *obj);

void rnn_vad_stop(void *obj);

/*
参考，安静环境下，环境噪声在-60~-70dB左右波动，设置阈值时建议留有余量
嘈杂环境在-50~-60dB左右波动
建议阈值设置在-30到-40dB左右（嘈杂）安静可以在-50dB左右
*/
void rnn_vad_set_callback(void *obj, float threshold);

#ifdef __cplusplus
}
#endif

#endif

