#include <os/os.h>
#include <os/mem.h>
#include "modules/aacdec.h"
#include "bk_aac_decoder.h"

int32_t bk_aac_decoder_init(aacdecodercontext_t* aac_decoder, uint32_t sample_rate, uint32_t channels)
{
    os_memset(aac_decoder, 0 , sizeof(aacdecodercontext_t));
    HAACDecoder hAACDecoder = AACInitDecoder();
    if (!hAACDecoder) {
        return -1;
    }
    aac_decoder->aac_decoder = hAACDecoder;
    aac_decoder->aac_frame_info.nChans = channels;
    aac_decoder->aac_frame_info.sampRateCore = sample_rate;
    aac_decoder->aac_frame_info.profile = AAC_PROFILE_LC;
    aac_decoder->aac_frame_info.bitsPerSample = 16;
    if (AACSetRawBlockParams(hAACDecoder, 0, &aac_decoder->aac_frame_info) != 0) {
        return -2;
    }
    return 0;
}

int32_t bk_aac_decoder_decode(aacdecodercontext_t* aac_decoder, uint8_t* inbuf, uint32_t inlen, uint8_t** outbuf, uint32_t* outlen)
{
    int ret = 0;
    unsigned char * in = inbuf;
    int in_len = inlen;
    ret = AACDecode(aac_decoder->aac_decoder, &in, &in_len, (short int *)aac_decoder->pcm_sample);
    if(ret == 0)
    {
        AACGetLastFrameInfo(aac_decoder->aac_decoder, &aac_decoder->aac_frame_info);
        *outbuf = (uint8_t *)aac_decoder->pcm_sample;
        *outlen = (uint32_t)aac_decoder->aac_frame_info.outputSamps * 2;
    }else
    {
        *outlen = 0;
    }
    return ret;
}

void bk_aac_decoder_deinit(aacdecodercontext_t* aac_decoder)
{
    AACFreeDecoder(aac_decoder->aac_decoder);
    os_memset(aac_decoder, 0 , sizeof(aacdecodercontext_t));
}