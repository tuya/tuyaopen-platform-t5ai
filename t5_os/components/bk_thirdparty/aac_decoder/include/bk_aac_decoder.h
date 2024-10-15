#ifndef __BT_AAC_DECODER_API_H__
#define __BT_AAC_DECODER_API_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include "modules/aacdec.h"

typedef struct {
    HAACDecoder aac_decoder;
    AACFrameInfo aac_frame_info;
    int32_t pcm_sample[2][1024];
}aacdecodercontext_t;

int32_t bk_aac_decoder_init(aacdecodercontext_t* aac_decoder, uint32_t sample_rate, uint32_t channels);
int32_t bk_aac_decoder_decode(aacdecodercontext_t* aac_decoder, uint8_t* inbuf, uint32_t inlen, uint8_t** outbuf, uint32_t* outlen);
void bk_aac_decoder_deinit(aacdecodercontext_t* aac_decoder);

#endif