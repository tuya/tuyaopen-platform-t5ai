// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "stdio.h"
#include <modules/fdk_aac_enc/aacenc_lib.h>
#include "ff.h"
#include <modules/pm.h>
#include "cli.h"


#define AAC_FRAME_SAMPLE_LEN    (1024)
#define AAC_CHN                 (1)
#define AAC_FRAME_SIZE          (AAC_FRAME_SAMPLE_LEN*AAC_CHN*2)
#define AAC_BITRATE             (32000)
#define AAC_SAMPLE_RATE         (8000)


static HANDLE_AACENCODER aac_enc_handle = NULL;


static void cli_aac_encoder_help(void)
{
    BK_LOGD(NULL, "aac_encoder_test {xx.pcm xx.aac} \n");
}

static HANDLE_AACENCODER aac_encoder_init(int channels, int sample_rate, int bitrate)
{
    int aot = 2;
    int afterburner = 0;
    int eld_sbr = 0;
    int vbr = 0;
    uint32_t  encoder_modis = 1;
    CHANNEL_MODE mode;
    HANDLE_AACENCODER handle;

    BK_LOGD(NULL, "channels: %d, sample_rate: %d, bitrate: %d \n", channels, sample_rate, bitrate);

    switch (channels)
    {
        case 1:
            mode = MODE_1;
            break;
        case 2:
            mode = MODE_2;
            break;
        case 3:
            mode = MODE_1_2;
            break;
        case 4:
            mode = MODE_1_2_1;
            break;
        case 5:
            mode = MODE_1_2_2;
            break;
        case 6:
            mode = MODE_1_2_2_1;
            break;
        default:
            BK_LOGD(NULL, "Unsupported channels: %d\n", channels);
            return NULL;
    }

    if (aacEncOpen(&handle, encoder_modis, channels) != AACENC_OK)
    {
        BK_LOGD(NULL, "open encoder fail\n");
        return NULL;
    }
    if (aacEncoder_SetParam(handle, AACENC_AOT, aot) != AACENC_OK)
    {
        BK_LOGD(NULL, "set the AOT fail\n");
        goto fail;
    }
    if (aot == 39 && eld_sbr)
    {
        if (aacEncoder_SetParam(handle, AACENC_SBR_MODE, 0) != AACENC_OK)
        {
            BK_LOGD(NULL, "set SBR mode for ELD fail\n");
            goto fail;
        }
    }
    if (aacEncoder_SetParam(handle, AACENC_SAMPLERATE, sample_rate) != AACENC_OK)
    {
        BK_LOGD(NULL, "set the sample rate fail\n");
        goto fail;
    }
    if (aacEncoder_SetParam(handle, AACENC_CHANNELMODE, mode) != AACENC_OK)
    {
        BK_LOGD(NULL, "set the channel mode fail\n");
        goto fail;
    }
    if (aacEncoder_SetParam(handle, AACENC_CHANNELORDER, 1) != AACENC_OK)
    {
        BK_LOGD(NULL, "set the wav channel order fail\n");
        goto fail;
    }
    if (vbr)
    {
        if (aacEncoder_SetParam(handle, AACENC_BITRATEMODE, vbr) != AACENC_OK)
        {
            BK_LOGD(NULL, "set the VBR bitrate mode fail\n");
            goto fail;
        }
    }
    else
    {
        if (aacEncoder_SetParam(handle, AACENC_BITRATE, bitrate) != AACENC_OK)
        {
            BK_LOGD(NULL, "set the bitrate fail\n");
            goto fail;
        }
    }
    if (aacEncoder_SetParam(handle, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK)
    {
        BK_LOGD(NULL, "set the ADTS transmux fail\n");
        goto fail;
    }
    if (aacEncoder_SetParam(handle, AACENC_AFTERBURNER, afterburner) != AACENC_OK)
    {
        BK_LOGD(NULL, "set the afterburner mode fail\n");
        goto fail;
    }

    if (aacEncEncode(handle, NULL, NULL, NULL, NULL) != AACENC_OK)
    {
        BK_LOGD(NULL, "initialize the encoder\n");
        goto fail;
    }

    return handle;

fail:
    if (handle)
    {
        aacEncClose(&handle);
    }
    return NULL;
}

static void aac_encoder_deinit(HANDLE_AACENCODER *aac_handle)
{
    if (aac_handle)
    {
        if (AACENC_OK != aacEncClose(aac_handle))
        {
            BK_LOGD(NULL, "%s, %d, close aac encoder fail \n", __func__, __LINE__);
        }
    }
}

static int aac_encoder_frame_process(HANDLE_AACENCODER aac_handle, int in_bytes, int16_t *data_in, int *out_bytes, uint8_t *data_out)
{
    AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
    AACENC_InArgs in_args = { 0 };
    AACENC_OutArgs out_args = { 0 };
    int in_identifier = IN_AUDIO_DATA;
    int out_identifier = OUT_BITSTREAM_DATA;
    int in_size, in_elem_size;
    int out_buf_size, out_elem_size;
    void *in_ptr, *out_ptr;
    int err;

    in_ptr = data_in;
    in_size = in_bytes;
    in_elem_size = 2;
    in_args.numInSamples = in_size <= 0 ? -1 : in_size / 2;
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_size;
    in_buf.bufElSizes = &in_elem_size;

    out_ptr = data_out;
    out_buf_size = 2048;
    out_elem_size = 1;
    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_buf_size;
    out_buf.bufElSizes = &out_elem_size;

    err = aacEncEncode(aac_handle, &in_buf, &out_buf, &in_args, &out_args);

    *out_bytes = out_args.numOutBytes;

    BK_LOGD(NULL, "[out_args] numOutBytes: %d, numInSamples: %d, numAncBytes: %d, bitResState: %d \n", out_args.numOutBytes, out_args.numInSamples, out_args.numAncBytes, out_args.bitResState);

    return err;
}


void cli_aac_encoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char aac_file_name[50];
    char pcm_file_name[50];
    FIL aac_file;
    FIL pcm_file;
    FRESULT fr;
    uint32 uiTemp = 0;
    uint8_t *in_buffer = NULL;
    uint32_t out_size = 0;
    uint8_t *out_buffer = NULL;
    int err = 0;

    if (argc != 3)
    {
        cli_aac_encoder_help();
        return;
    }

    BK_LOGD(NULL, "-----------start test----------- \n");
    sprintf(pcm_file_name, "1:/%s", argv[1]);
    fr = f_open(&pcm_file, pcm_file_name, FA_READ);
    if (fr != FR_OK)
    {
        BK_LOGD(NULL, "open %s fail \n", pcm_file_name);
        goto fail;
    }

    sprintf(aac_file_name, "1:/%s", argv[2]);
    fr = f_open(&aac_file, aac_file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK)
    {
        BK_LOGD(NULL, "open %s fail\n", aac_file_name);
        goto fail;
    }

    BK_LOGD(NULL, "-----------open file ok----------- \n");

    in_buffer = (unsigned char *)os_malloc(AAC_FRAME_SIZE);
    if (!in_buffer)
    {
        BK_LOGD(NULL, "os_malloc in_buffer: %d fail\n", AAC_FRAME_SIZE);
        goto fail;
    }
    os_memset(in_buffer, 0, AAC_FRAME_SIZE);

    out_buffer = (unsigned char *)os_malloc(AAC_FRAME_SIZE);
    if (!out_buffer)
    {
        BK_LOGD(NULL, "os_malloc out_buffer: %d fail\n", AAC_FRAME_SIZE);
        goto fail;
    }
    os_memset(out_buffer, 0, AAC_FRAME_SIZE);

    BK_LOGD(NULL, "-----------open file ok----------- \n");
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);

    /* init AAC encoder */
    //aacEncSetMemType(MEM_TYPE_PSRAM);     //use psram
    aac_enc_handle = aac_encoder_init(AAC_CHN, AAC_SAMPLE_RATE, AAC_BITRATE);
    if (!aac_enc_handle)
    {
        BK_LOGD(NULL, "aac_encoder_init fail\n");
        return;
    }
    BK_LOGD(NULL, "-----------aac_encoder_init ok----------- \n");

    while (1)
    {
        BK_LOGD(NULL, "-----------NeAACDecDecode----------- \n");
        rtos_delay_milliseconds(5);
        fr = f_read(&pcm_file, in_buffer, AAC_FRAME_SIZE, &uiTemp);
        if (fr != FR_OK)
        {
            BK_LOGD(NULL, "read pcm file fail\n");
            goto fail;
        }
        else
        {
            if (uiTemp < AAC_FRAME_SIZE)
            {
                BK_LOGD(NULL, "pcm file is empty\n");
                goto fail;
            }
        }

        out_size = 0;
        //GPIO_UP(33);
        err = aac_encoder_frame_process(aac_enc_handle, AAC_FRAME_SIZE, (int16_t *)in_buffer, (int *)&out_size, out_buffer);
        //GPIO_DOWN(33);
        if (err == 0)
        {
            if (out_size > 0)
            {
                fr = f_write(&aac_file, (void *)out_buffer, out_size, &uiTemp);
                if (fr != FR_OK)
                {
                    BK_LOGD(NULL, "write output data %s fail\n", aac_file_name);
                    goto fail;
                }
            }
        }
        else
        {
            /* error */
            BK_LOGD(NULL, "aac encode Error: %d \n", err);
        }
    }

fail:

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    fr = f_close(&aac_file);
    if (fr != FR_OK)
    {
        BK_LOGD(NULL, "close file %s fail \n", aac_file_name);
    }

    fr = f_close(&pcm_file);
    if (fr != FR_OK)
    {
        BK_LOGD(NULL, "close file %s fail \n", pcm_file_name);
    }

    if (in_buffer)
    {
        os_free(in_buffer);
        in_buffer = NULL;
    }

    if (out_buffer)
    {
        os_free(out_buffer);
        out_buffer = NULL;
    }

    aac_encoder_deinit(&aac_enc_handle);
    aac_enc_handle = NULL;

    BK_LOGD(NULL, "test finish \n");
}


/*************************Note*****************************/
/*
 *   The task stack of aac encode is large, about 8~10K.
 */
#define AAC_ENC_CMD_CNT (sizeof(s_aac_enc_commands) / sizeof(struct cli_command))
static const struct cli_command s_aac_enc_commands[] =
{
    {"aac_encoder_test", "aac_encoder_test {xx.pcm xx.aac}", cli_aac_encoder_test_cmd},
};

int cli_aac_encode_init(void)
{
    return cli_register_commands(s_aac_enc_commands, AAC_ENC_CMD_CNT);
}

