#include "mpeg4_latm_dec.h"
#include "mpeg4_get_bits.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define LOAS_SYNC_WORD   0x2b7       ///< 11 bits LOAS sync word


#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_ELEM_ID 64

typedef struct MPEG4AudioConfig
{
    int object_type;
    int sampling_index;
    int sample_rate;
    int chan_config;
    int sbr; ///< -1 implicit, 1 presence
    int ext_object_type;
    int ext_sampling_index;
    int ext_sample_rate;
    int ext_chan_config;
    int channels;
    int ps;  ///< -1 implicit, 1 presence
    int frame_length_short;
} MPEG4AudioConfig;

typedef struct LATMContext
{
    int initialized;        ///< initialized after a valid extradata was seen

    // parser data
    uint8_t use_same_stream_mux;
    uint8_t audio_mux_version;
    int audio_mux_version_A; ///< LATM syntax version

    uint8_t all_stream_same_time_framing;
    uint8_t num_sub_frames;
    uint8_t num_program;

    MPEG4AudioConfig mpeg4_audio_config;

    int frame_length_type;   ///< 0/1 variable/fixed frame length
    int latm_buffer_fullness;
    uint8_t other_data_present;
    uint32_t other_data_bits;
    uint8_t crc_present;

    int frame_length;        ///< frame length for fixed frame length


    //result
    int result_len;
} LATMContext;

enum AudioObjectType
{
    AOT_NULL = 0,
    // Support?                Name
    AOT_AAC_MAIN         =  1, ///< Y                       Main
    AOT_AAC_LC           =  2, ///< Y                       Low Complexity
    AOT_AAC_SSR          =  3, ///< N (code in SoC repo)    Scalable Sample Rate
    AOT_AAC_LTP          =  4, ///< Y                       Long Term Prediction
    AOT_SBR              =  5, ///< Y                       Spectral Band Replication
    AOT_AAC_SCALABLE     =  6, ///< N                       Scalable
    AOT_TWINVQ           =  7, ///< N                       Twin Vector Quantizer
    AOT_CELP             =  8, ///< N                       Code Excited Linear Prediction
    AOT_HVXC             =  9, ///< N                       Harmonic Vector eXcitation Coding

    AOT_TTSI             = 12, ///< N                       Text-To-Speech Interface
    AOT_MAINSYNTH        = 13, ///< N                       Main Synthesis
    AOT_WAVESYNTH        = 14, ///< N                       Wavetable Synthesis
    AOT_MIDI             = 15, ///< N                       General MIDI
    AOT_SAFX             = 16, ///< N                       Algorithmic Synthesis and Audio Effects
    AOT_ER_AAC_LC        = 17, ///< N                       Error Resilient Low Complexity

    AOT_ER_AAC_LTP       = 19, ///< N                       Error Resilient Long Term Prediction
    AOT_ER_AAC_SCALABLE  = 20, ///< N                       Error Resilient Scalable
    AOT_ER_TWINVQ        = 21, ///< N                       Error Resilient Twin Vector Quantizer
    AOT_ER_BSAC          = 22, ///< N                       Error Resilient Bit-Sliced Arithmetic Coding
    AOT_ER_AAC_LD        = 23, ///< N                       Error Resilient Low Delay
    AOT_ER_CELP          = 24, ///< N                       Error Resilient Code Excited Linear Prediction
    AOT_ER_HVXC          = 25, ///< N                       Error Resilient Harmonic Vector eXcitation Coding
    AOT_ER_HILN          = 26, ///< N                       Error Resilient Harmonic and Individual Lines plus Noise
    AOT_ER_PARAM         = 27, ///< N                       Error Resilient Parametric
    AOT_SSC              = 28, ///< N                       SinuSoidal Coding
    AOT_PS               = 29, ///< N                       Parametric Stereo
    AOT_SURROUND         = 30, ///< N                       MPEG Surround
    AOT_ESCAPE           = 31, ///< Y                       Escape Value
    AOT_L1               = 32, ///< Y                       Layer 1
    AOT_L2               = 33, ///< Y                       Layer 2
    AOT_L3               = 34, ///< Y                       Layer 3
    AOT_DST              = 35, ///< N                       Direct Stream Transfer
    AOT_ALS              = 36, ///< Y                       Audio LosslesS
    AOT_SLS              = 37, ///< N                       Scalable LosslesS
    AOT_SLS_NON_CORE     = 38, ///< N                       Scalable LosslesS (non core)
    AOT_ER_AAC_ELD       = 39, ///< N                       Error Resilient Enhanced Low Delay
    AOT_SMR_SIMPLE       = 40, ///< N                       Symbolic Music Representation Simple
    AOT_SMR_MAIN         = 41, ///< N                       Symbolic Music Representation Main
    AOT_USAC             = 42, ///< Y                       Unified Speech and Audio Coding
    AOT_SAOC             = 43, ///< N                       Spatial Audio Object Coding
    AOT_LD_SURROUND      = 44, ///< N                       Low Delay MPEG Surround
};

static const int ff_mpeg4audio_sample_rates[16] =
{
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

static const uint8_t ff_mpeg4audio_channels[15] =
{
    0,
    1, // mono (1/0)
    2, // stereo (2/0)
    3, // 3/0
    4, // 3/1
    5, // 3/2
    6, // 3/2.1
    8, // 5/2.1
    0,
    0,
    0,
    7, // 3/3.1
    8, // 3/2/2.1
    24, // 3/3/3 - 5/2/3 - 3/0/0.2
    8, // 3/2.1 - 2/0
};

static inline uint32_t latm_get_value(GetBitContext *b)
{
    int length = get_bits(b, 2);

    if (length + 1 > sizeof(uint32_t))
    {
        mpeg_loge("err, read too long %d", length + 1);
        return -1;
    }

    return get_bits_long(b, (length + 1) * 8);
}

#if 0
static int get_object_type(GetBitContext *gb)
{
    int object_type = get_bits(gb, 5);

    if (object_type == AOT_ESCAPE)
    {
        object_type = 32 + get_bits(gb, 6);
    }

    return object_type;
}

static inline int get_sample_rate(GetBitContext *gb, int *index)
{
    *index = get_bits(gb, 4);
    return *index == 0x0f ? get_bits(gb, 24) : ff_mpeg4audio_sample_rates[*index];
}

static int decode_ga_specific_config(GetBitContext *gb,
                                     int get_bit_alignment,
                                     MPEG4AudioConfig *m4ac,
                                     int channel_config)
{
    int extension_flag, ret, ep_config, res_flags;
    //uint8_t layout_map[MAX_ELEM_ID * 4][3] = {0};
    //uint8_t *layout_map = os_malloc(MAX_ELEM_ID * 4 * 3);
    //uint8_t (*layout_map)[3],

    int tags = 0;

    m4ac->frame_length_short = get_bits1(gb);

    if (m4ac->frame_length_short && m4ac->sbr == 1)
    {
        //        avpriv_report_missing_feature(avctx, "SBR with 960 frame length");
        //
        //        if (ac) { ac->warned_960_sbr = 1; }

        m4ac->sbr = 0;
        m4ac->ps = 0;
        mpeg_logw("unknow feat 1");
    }

    if (get_bits1(gb))       // dependsOnCoreCoder
    {
        skip_bits(gb, 14);    // coreCoderDelay
    }

    extension_flag = get_bits1(gb);

    if (m4ac->object_type == AOT_AAC_SCALABLE || m4ac->object_type == AOT_ER_AAC_SCALABLE)
    {
        skip_bits(gb, 3);    // layerNr
    }

    if (channel_config == 0)
    {
        mpeg_loge("unexpect channel config 0");
        return -1;
#if 0
        skip_bits(gb, 4);  // element_instance_tag
        tags = decode_pce(avctx, m4ac, layout_map, gb, get_bit_alignment);

        if (tags < 0)
        {
            return tags;
        }

#endif

    }
    else
    {
#if 0

        if ((ret = ff_aac_set_default_channel_config(ac, avctx, layout_map,
                   &tags, channel_config)))
        {
            return ret;
        }

#endif
    }

#if 0

    if (count_channels(layout_map, tags) > 1)
    {
        m4ac->ps = 0;
    }
    else if (m4ac->sbr == 1 && m4ac->ps == -1)
    {
        m4ac->ps = 1;
    }

    if (ac && (ret = ff_aac_output_configure(ac, layout_map, tags, OC_GLOBAL_HDR, 0)))
    {
        return ret;
    }

#endif

    if (extension_flag)
    {
        mpeg_logw("unexpect extension_flag");

        switch (m4ac->object_type)
        {
        case AOT_ER_BSAC:
            skip_bits(gb, 5);    // numOfSubFrame
            skip_bits(gb, 11);   // layer_length
            break;

        case AOT_ER_AAC_LC:
        case AOT_ER_AAC_LTP:
        case AOT_ER_AAC_SCALABLE:
        case AOT_ER_AAC_LD:
            res_flags = get_bits(gb, 3);

            if (res_flags)
            {
                return -1;
            }

            break;
        }

        skip_bits1(gb);    // extensionFlag3 (TBD in version 3)
    }

#if 0

    switch (m4ac->object_type)
    {
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LTP:
    case AOT_ER_AAC_SCALABLE:
    case AOT_ER_AAC_LD:
        ep_config = get_bits(gb, 2);

        if (ep_config)
        {
            avpriv_report_missing_feature(avctx,
                                          "epConfig %d", ep_config);
            return -1;
        }
    }

#endif
    return 0;
}

static int ff_mpeg4audio_get_config_gb(MPEG4AudioConfig *c, GetBitContext *gb, int sync_extension)
{
    int specific_config_bitindex, ret;
    int start_bit_index = get_bits_count(gb);

    c->object_type = get_object_type(gb);
    c->sample_rate = get_sample_rate(gb, &c->sampling_index);
    c->chan_config = get_bits(gb, 4);

    if (c->chan_config < FF_ARRAY_ELEMS(ff_mpeg4audio_channels))
    {
        c->channels = ff_mpeg4audio_channels[c->chan_config];
    }
    else
    {
        mpeg_loge("Invalid chan_config %d", c->chan_config);
        return -1;
    }

    c->sbr = -1;
    c->ps  = -1;


    if (c->chan_config == 0)
    {
        mpeg_loge("unexpect channel config 0");
        return -1;
    }

    if (c->object_type != AOT_AAC_LC)
    {
        mpeg_loge("unexpect obj type %d", c->object_type);
    }

#if 0

    if (c->object_type == AOT_SBR || (c->object_type == AOT_PS &&
                                      // check for W6132 Annex YYYY draft MP3onMP4
                                      !((show_bits(gb, 3) & 0x03) && !(show_bits(gb, 9) & 0x3F))))
    {
        if (c->object_type == AOT_PS)
        {
            c->ps = 1;
        }

        c->ext_object_type = AOT_SBR;
        c->sbr = 1;
        c->ext_sample_rate = get_sample_rate(gb, &c->ext_sampling_index);
        c->object_type = get_object_type(gb);

        if (c->object_type == AOT_ER_BSAC)
        {
            c->ext_chan_config = get_bits(gb, 4);
        }
    }

#endif
    else
    {
        c->ext_object_type = AOT_NULL;
        c->ext_sample_rate = 0;
    }

    specific_config_bitindex = get_bits_count(gb);

#if 0

    if (c->object_type == AOT_ALS)
    {
        skip_bits(gb, 5);

        if (show_bits(gb, 24) != MKBETAG('\0', 'A', 'L', 'S'))
        {
            skip_bits(gb, 24);
        }

        specific_config_bitindex = get_bits_count(gb);

        ret = parse_config_ALS(gb, c, logctx);

        if (ret < 0)
        {
            return ret;
        }
    }

#endif

    if (c->ext_object_type != AOT_SBR && sync_extension)
    {
        while (get_bits_left(gb) > 15)
        {
            if (show_bits(gb, 11) == 0x2b7)   // sync extension
            {
                get_bits(gb, 11);
                c->ext_object_type = get_object_type(gb);

                if (c->ext_object_type == AOT_SBR && (c->sbr = get_bits1(gb)) == 1)
                {
                    c->ext_sample_rate = get_sample_rate(gb, &c->ext_sampling_index);

                    if (c->ext_sample_rate == c->sample_rate)
                    {
                        c->sbr = -1;
                    }
                }

                if (get_bits_left(gb) > 11 && get_bits(gb, 11) == 0x548)
                {
                    c->ps = get_bits1(gb);
                }

                break;
            }
            else
            {
                get_bits1(gb);    // skip 1 bit
            }
        }
    }

    //PS requires SBR
    if (!c->sbr)
    {
        c->ps = 0;
    }

    //Limit implicit PS to the HE-AACv2 Profile
    if ((c->ps == -1 && c->object_type != AOT_AAC_LC) || c->channels & ~0x01)
    {
        c->ps = 0;
    }

    return specific_config_bitindex - start_bit_index;
}


static int decode_audio_specific_config_gb(LATMContext *ctx,
        GetBitContext *gb,
        int get_bit_alignment,
        int sync_extension)
{
    int i, ret;
    GetBitContext gbc = *gb;
    MPEG4AudioConfig *m4ac = &ctx->mpeg4_audio_config;
    MPEG4AudioConfig m4ac_bak = *m4ac;

    if ((i = ff_mpeg4audio_get_config_gb(m4ac, &gbc, sync_extension)) < 0)
    {
        *m4ac = m4ac_bak;
        return -1;
    }

    if (m4ac->sampling_index > 12)
    {
        mpeg_loge("invalid sampling rate index %d", m4ac->sampling_index);
        *m4ac = m4ac_bak;
        return -1;
    }

    if (m4ac->object_type == AOT_ER_AAC_LD &&
            (m4ac->sampling_index < 3 || m4ac->sampling_index > 7))
    {
        mpeg_loge(
            "invalid low delay sampling rate index %d",
            m4ac->sampling_index);
        *m4ac = m4ac_bak;
        return -1;
    }

    skip_bits_long(gb, i);

    switch (m4ac->object_type)
    {
    case AOT_AAC_MAIN:
    case AOT_AAC_SSR:
    case AOT_AAC_LTP:
    case AOT_AAC_SCALABLE:
    case AOT_TWINVQ:
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LTP:
    case AOT_ER_AAC_SCALABLE:
    case AOT_ER_TWINVQ:
    case AOT_ER_BSAC:
    case AOT_ER_AAC_LD:
        mpeg_logw("unexpect obj type %d", m4ac->object_type);

    case AOT_AAC_LC:
        if ((ret = decode_ga_specific_config(gb, get_bit_alignment,
                                             &ctx->mpeg4_audio_config, m4ac->chan_config)) < 0)
        {
            return ret;
        }

        break;

    default:
        mpeg_loge("unknow Audio object type %s%d", m4ac->sbr == 1 ? "SBR+" : "", m4ac->object_type);
        return -1;
    }

    mpeg_logd(
        "AOT %d chan config %d sampling index %d (%d) SBR %d PS %d\n",
        m4ac->object_type, m4ac->chan_config, m4ac->sampling_index,
        m4ac->sample_rate, m4ac->sbr,
        m4ac->ps);

    return get_bits_count(gb);
}


static int latm_decode_audio_specific_config(struct LATMContext *latmctx, GetBitContext *gb, int asclen)
{
    GetBitContext gbc;
    int config_start_bit  = get_bits_count(gb);
    int sync_extension    = 0;
    int bits_consumed, esize, i;

    if (asclen > 0)
    {
        sync_extension = 1;
        asclen         = FFMIN(asclen, get_bits_left(gb));
        init_get_bits(&gbc, gb->buffer, config_start_bit + asclen);
        skip_bits_long(&gbc, config_start_bit);
    }
    else if (asclen == 0)
    {
        gbc = *gb;
    }
    else
    {
        mpeg_loge("asclen err %d", asclen);
        return -1;
    }

    if (get_bits_left(gb) <= 0)
    {
        mpeg_loge("left len err");
        return -1;
    }

    bits_consumed = decode_audio_specific_config_gb(latmctx,
                    &gbc, config_start_bit,
                    sync_extension);

    if (bits_consumed < config_start_bit)
    {
        return -1;
    }

    bits_consumed -= config_start_bit;

    if (asclen == 0)
    {
        asclen = bits_consumed;
    }

#if 0

    if (!latmctx->initialized ||
            ac->oc[1].m4ac.sample_rate != m4ac->sample_rate ||
            ac->oc[1].m4ac.chan_config != m4ac->chan_config)
    {

        if (latmctx->initialized)
        {
            mpeg_logi("audio config changed (sample_rate=%d, chan_config=%d)", m4ac->sample_rate, m4ac->chan_config);
        }
        else
        {
            mpeg_logd("initializing latmctx");
        }

        latmctx->initialized = 0;

        esize = (asclen + 7) / 8;

        if (avctx->extradata_size < esize)
        {
            av_free(avctx->extradata);
            avctx->extradata = av_malloc(esize + AV_INPUT_BUFFER_PADDING_SIZE);

            if (!avctx->extradata)
            {
                return AVERROR(ENOMEM);
            }
        }

        avctx->extradata_size = esize;
        gbc = *gb;

        for (i = 0; i < esize; i++)
        {
            avctx->extradata[i] = get_bits(&gbc, 8);
        }

        memset(avctx->extradata + esize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    }

#endif

    skip_bits_long(gb, asclen);

    return 0;
}

#endif

static int read_stream_mux_config(struct LATMContext *latmctx, GetBitContext *gb)
{
    int ret, audio_mux_version = get_bits(gb, 1);
    int layers = 0;
    uint8_t unexpect_payload = 0;

    latmctx->audio_mux_version = audio_mux_version;
    latmctx->audio_mux_version_A = 0;

    if (audio_mux_version)
    {
        latmctx->audio_mux_version_A = get_bits(gb, 1);
    }
    else
    {
        mpeg_logw("audio mux ver 0");
    }

    if (!latmctx->audio_mux_version_A)
    {

        if (audio_mux_version)
        {
            latm_get_value(gb);    // taraFullness
        }

        latmctx->all_stream_same_time_framing = get_bits(gb, 1);

        if (!latmctx->all_stream_same_time_framing)
        {
            mpeg_logw("all_stream_same_time_framing 0");
        }

        latmctx->num_sub_frames = get_bits(gb, 6);
        //        skip_bits(gb, 1);                       // allStreamSameTimeFraming
        //        skip_bits(gb, 6);                       // numSubFrames

        latmctx->num_program = get_bits(gb, 4);

        if (latmctx->num_sub_frames)
        {
            mpeg_loge("multi sub frames %d", latmctx->num_sub_frames);
            unexpect_payload = 1;
        }

        // numPrograms
        if (latmctx->num_program)
        {
            //todo: maybe not zero in iphone a2dp src payload
            mpeg_loge("multi programs %d", latmctx->num_program);
            unexpect_payload = 1;
        }

        // for each program (which there is only one in DVB)

        // for each layer (which there is only one in DVB)
        if ((layers = get_bits(gb, 3)))                     // numLayer
        {
            //            avpriv_request_sample(latmctx->aac_ctx.avctx, "Multiple layers");
            mpeg_loge("multi layers %d", layers);
            unexpect_payload = 1;
        }

        if (unexpect_payload)
        {
            mpeg_loge("unexpect payload, ret err");
            return -1;
        }

        // for all but first stream: use_same_config = get_bits(gb, 1);
        if (!audio_mux_version)
        {
#if 0

            if ((ret = latm_decode_audio_specific_config(latmctx, gb, 0)) < 0)
            {
                return ret;
            }

#else
#endif
        }
        else
        {

            int ascLen = latm_get_value(gb);
#if 0

            if ((ret = latm_decode_audio_specific_config(latmctx, gb, ascLen)) < 0)
            {
                return ret;
            }

#else
            skip_bits_long(gb, ascLen);
#endif
        }

        latmctx->frame_length_type = get_bits(gb, 3);

        switch (latmctx->frame_length_type)
        {
        case 0:
            latmctx->latm_buffer_fullness = get_bits(gb, 8);

            if (!latmctx->all_stream_same_time_framing)
            {

            }

            break;

        case 1:
            mpeg_logw("frame len type %d", latmctx->frame_length_type);
            latmctx->frame_length = get_bits(gb, 9);
            break;

        case 3:
        case 4:
        case 5:
            mpeg_logw("frame len type %d", latmctx->frame_length_type);
            skip_bits(gb, 6);       // CELP frame length table index
            break;

        case 6:
        case 7:
            mpeg_logw("frame len type %d", latmctx->frame_length_type);
            skip_bits(gb, 1);       // HVXC frame length table index
            break;
        }

        latmctx->other_data_present = get_bits(gb, 1);

        if (latmctx->other_data_present)                    // other data
        {
            mpeg_logw("other_data_present 1");

            if (audio_mux_version)
            {
                latmctx->other_data_bits = latm_get_value(gb);             // other_data_bits
            }
            else
            {
                int esc;

                latmctx->other_data_bits = 0;

                do
                {
                    if (get_bits_left(gb) < 9)
                    {
                        mpeg_loge("get bits left in other data err");
                        return -1;
                    }

                    esc = get_bits(gb, 1);
                    latmctx->other_data_bits += get_bits(gb, 8);
                }
                while (esc);
            }
        }


        if (get_bits(gb, 1))                     // crc present
        {
            mpeg_logw("crc present 1");
            skip_bits(gb, 8);    // config_crc
        }
    }
    else
    {
        mpeg_logw("audio mux ver A %d", latmctx->audio_mux_version_A);
    }

    return 0;
}

static int read_payload_length_info(struct LATMContext *ctx, GetBitContext *gb)
{
    uint8_t tmp;

    if (ctx->all_stream_same_time_framing)
    {
        if (ctx->frame_length_type == 0)
        {
            int mux_slot_length = 0;

            do
            {
                if (get_bits_left(gb) < 8)
                {
                    mpeg_loge("get tmp err");
                    return -1;
                }

                tmp = get_bits(gb, 8);
                mux_slot_length += tmp;
            }
            while (tmp == 255);

            return mux_slot_length;
        }
        else if (ctx->frame_length_type == 1)
        {
            return ctx->frame_length;
        }
        else if (ctx->frame_length_type == 3 ||
                 ctx->frame_length_type == 5 ||
                 ctx->frame_length_type == 7)
        {
            return get_bits(gb, 2);          // mux_slot_length_coded
        }
    }

    return 0;
}

static int read_audio_mux_element(struct LATMContext *latmctx, GetBitContext *gb)
{
    int err;
    uint8_t use_same_mux = get_bits(gb, 1);

    latmctx->use_same_stream_mux = use_same_mux;

    if (!use_same_mux)
    {
        if ((err = read_stream_mux_config(latmctx, gb)) < 0)
        {
            return err;
        }
    }

    //    else if (!latmctx->aac_ctx.avctx->extradata)
    //    {
    //        mpeg_logd(latmctx->aac_ctx.avctx, AV_LOG_DEBUG, "no decoder config found");
    //        return 1;
    //    }

    if (latmctx->audio_mux_version_A == 0)
    {
        int mux_slot_length_bytes = read_payload_length_info(latmctx, gb);
        int bits_left = get_bits_left(gb);

        latmctx->result_len = mux_slot_length_bytes;

        mpeg_logd("len %d %f", mux_slot_length_bytes, bits_left / 8.0);

        if (mux_slot_length_bytes < 0)
        {
            mpeg_loge("mux_slot_length_bytes err %d %d", mux_slot_length_bytes);
            return -1;
        }
        else if (mux_slot_length_bytes * 8LL + (latmctx->other_data_present ? latmctx->other_data_bits : 0) > bits_left)
        {
            mpeg_loge("incomplete frame %d %d %d", mux_slot_length_bytes * 8, latmctx->other_data_present ? latmctx->other_data_bits : 0, bits_left);
            return -1;
        }

        //        else if (mux_slot_length_bytes * 8LL + 256 + (latmctx->other_data_present ? latmctx->other_data_bits : 0) < bits_left)
        //        {
        //            mpeg_loge("unknow frame %d %d %d", mux_slot_length_bytes * 8, latmctx->other_data_present ? latmctx->other_data_bits : 0, bits_left);
        //            return -1;
        //        }
    }

    return 0;
}


int mpeg4_latm_decode(uint8_t *input, uint32_t input_len, uint8_t **output, uint32_t *output_len)
{
    static struct LATMContext latmctx;
    GetBitContext gb;
    int muxlength = 0, err = 0;

    os_memset(&gb, 0, sizeof(gb));

    if ((err = init_get_bits8(&gb, input, input_len)) < 0)
    {
        mpeg_loge("init get bit err %d", err);
        return err;
    }

    os_memset(&latmctx, 0, sizeof(latmctx));

    err = read_audio_mux_element(&latmctx, &gb);

    if (err)
    {
        mpeg_loge("read audio mux elem err %d", err);

        return -1;
    }

    int start_bit_index = get_bits_count(&gb);

    if (start_bit_index % 8)
    {
        mpeg_logw("start bit not aligned %d", start_bit_index);
        start_bit_index = (start_bit_index / 8 + 1) * 8;
    }

    *output = input + start_bit_index / 8;
    *output_len = latmctx.result_len;// input_len - start_bit_index / 8;

    return 0;
}

#if 0
const uint8_t test_buffer[] =
{
    0x47, 0xFC, 0x00, 0x00, 0xB0, 0x90, 0x80, 0x03, 0x00, 0xFF, 0x48, 0x21, 0x1A, 0xCF, 0xFF, 0xBF, 0xFF, 0xFF, 0xFF, 0xA7, 0xB6, 0x4A, 0x18, 0x8E, 0x34, 0x39, 0x10, 0x4A, 0x85, 0x2A, 0xEC, 0xAA, 0x19, 0x7B, 0x97, 0x20, 0x20, 0x05, 0x20, 0x1E, 0x51, 0xCF, 0xFC, 0xC5, 0xCD, 0xDB, 0xB9, 0x16, 0xF2, 0xAE, 0x01, 0xCC, 0x73, 0xC9, 0xCD, 0xA1, 0x16, 0xCA, 0x4D, 0x67, 0x27, 0xC0, 0x71, 0x78, 0x21, 0x5A, 0x2A, 0x12, 0x29, 0x89, 0xC4, 0x70, 0xE2, 0xE7, 0xDC, 0xCF, 0x42, 0xD5, 0x8C, 0x66, 0xB4, 0xB2, 0x2D, 0xC7, 0xF4, 0x0F, 0xA3, 0xFD, 0x57, 0x43, 0x3E, 0x52, 0x98, 0x4D, 0x63, 0x7E, 0xB3, 0xB2, 0xAF, 0x6A, 0x00, 0x8C, 0x46, 0xE9, 0x08, 0x0A, 0x24, 0x1B, 0xE6, 0xD7, 0xAA, 0xEA, 0xEB, 0x7B, 0x62, 0x50, 0x53, 0x05, 0xCD, 0xBB, 0x3F, 0x51, 0x87, 0x22, 0xA4, 0xD3, 0xC1, 0x78, 0xE4, 0xA7, 0x1F, 0xC0, 0xDC, 0x94, 0x29, 0x24, 0x59, 0x18, 0xDF, 0xAD, 0x47, 0xB4, 0xE4, 0x0B, 0xF4, 0x7D, 0x61, 0xF0, 0x99, 0x49, 0x9B, 0xA0, 0xAB, 0x83, 0x82, 0xB8, 0x4E, 0x3D, 0x7F, 0xEE, 0x7E, 0x42, 0x03, 0x87, 0x33, 0x85, 0x51, 0x3D, 0x95, 0xCD, 0x11, 0x09, 0xA4, 0x6C, 0x37, 0x46, 0x59, 0xD8, 0xE5, 0x4F, 0x0C, 0x6E, 0xD7, 0xE8, 0xCD, 0x11, 0x02, 0x50, 0x95, 0x27, 0xEC, 0xB2, 0xCA, 0x1F, 0x27, 0x21, 0x36, 0x32, 0xA8, 0x04, 0x5C, 0xCD, 0x0F, 0xAD, 0x55, 0x79, 0xDE, 0x77, 0x58, 0x36, 0x63, 0x78, 0xF1, 0xD9, 0x5C, 0x89, 0x23, 0x95, 0xF3, 0xD9, 0xAB, 0xD8, 0x3F, 0x4B, 0xD3, 0x2C, 0x80, 0x01, 0x53, 0x69, 0x54, 0x9A, 0x00, 0x62, 0x30, 0x10, 0xA5, 0xF0, 0x0B, 0xF4, 0xE0, 0x0C, 0x3B, 0xD0, 0xAD, 0x00, 0x34, 0x00, 0x0E, 0x7E, 0xFB, 0xA0, 0xD9, 0x49, 0xD7, 0x95, 0xAC, 0x22, 0x71, 0x49, 0x55, 0xC6, 0x5B, 0x86, 0x3B, 0x7D, 0xC8, 0xF8, 0x51, 0xC5, 0x43, 0x55, 0x78, 0xC2, 0x23, 0x9B, 0xAB, 0x9B, 0xAF, 0xE7, 0xA8, 0x74, 0x64, 0x01, 0x5C, 0x7E, 0xED, 0x80, 0x2E, 0xE4, 0xCA, 0xD7, 0xF2, 0xD9, 0xB7, 0x30, 0xB2, 0xA6, 0xA6, 0x3A, 0xD0, 0x1A, 0x2E, 0x22, 0xCD, 0xF8, 0x3C, 0xB6, 0x20, 0x05, 0xCC, 0x22, 0xA2, 0x28, 0x5D, 0x28, 0x07, 0x82, 0xF4, 0x80, 0x92, 0x36, 0x04, 0xE5, 0x86, 0x4A, 0xA3, 0x13, 0xAE, 0x05, 0x80, 0x62, 0xA4, 0x6F, 0x34, 0x7D, 0x20, 0x1C, 0xEB,
};

const uint8_t test_buffer2[] =
{
    0x47, 0xFC, 0x00, 0x00, 0xB0, 0x90, 0x80, 0x03, 0x00, 0xFF, 0xFF, 0x8A, 0x21, 0x1B, 0x0F, 0xFB, 0xFF, 0xED, 0x87, 0xA7, 0xD5, 0x5B, 0xE0, 0xF6, 0x18, 0x13, 0x29, 0x08, 0x25, 0x79, 0x35, 0xAD, 0xCC, 0xDF, 0x55, 0x4D, 0x64, 0x89, 0x7B, 0xD2, 0xA4, 0x55, 0xC5, 0x54, 0xA8, 0x54, 0x8A, 0x5B, 0x28, 0x79, 0xA4, 0x03, 0xD5, 0xF5, 0x4E, 0x09, 0xE2, 0x80, 0x0C, 0x91, 0xB2, 0x79, 0xD5, 0xA2, 0x36, 0xD0, 0xF8, 0x4D, 0xF7, 0xB0, 0xD5, 0x5F, 0x1B, 0x89, 0xCA, 0x56, 0xD4, 0x4C, 0xA5, 0x97, 0x79, 0xDA, 0x97, 0x2D, 0xAE, 0x30, 0xBF, 0xAA, 0xCE, 0x3D, 0xD3, 0xD8, 0x74, 0x09, 0x19, 0xB4, 0xE1, 0x5A, 0x9B, 0xDF, 0x02, 0xCE, 0x32, 0xE2, 0x14, 0x01, 0x85, 0xA4, 0xE1, 0x8D, 0x70, 0x9C, 0x54, 0x00, 0x8B, 0xF1, 0xE4, 0x3A, 0x52, 0x04, 0x95, 0x73, 0x9E, 0xB2, 0x77, 0xAA, 0x1D, 0x69, 0x13, 0x8D, 0x4E, 0xA7, 0x3A, 0x29, 0x1C, 0x43, 0x30, 0xCC, 0xD1, 0x03, 0xCA, 0x7E, 0x97, 0x4E, 0x94, 0x41, 0xB3, 0x7E, 0x9E, 0x92, 0x4A, 0x02, 0x12, 0x36, 0xD4, 0xA0, 0xE7, 0x59, 0x42, 0x5F, 0xB7, 0x87, 0xAF, 0xD8, 0x70, 0x32, 0x45, 0xD6, 0x14, 0xA1, 0x07, 0x2B, 0x1C, 0xB6, 0x15, 0x8F, 0x45, 0x4B, 0x4F, 0x09, 0xFA, 0x04, 0x55, 0x1D, 0xB8, 0xF6, 0xA0, 0xCA, 0x96, 0x52, 0xB7, 0xAC, 0xA0, 0xC3, 0x65, 0xD6, 0x48, 0x96, 0x74, 0xCA, 0xCB, 0xAC, 0x28, 0xCA, 0x9C, 0xAE, 0xD9, 0xCB, 0x2A, 0x31, 0xB3, 0xC7, 0xB3, 0xFE, 0x70, 0x8A, 0x47, 0x32, 0x19, 0xEE, 0xB3, 0x65, 0x56, 0x39, 0xD6, 0x9C, 0xB8, 0xCF, 0x3C, 0xF8, 0xE7, 0x2E, 0xFE, 0xAD, 0x8A, 0x31, 0x3D, 0x54, 0x93, 0x80, 0xD6, 0xC3, 0x2C, 0xF6, 0x13, 0xBC, 0x70, 0x80, 0x06, 0x06, 0x04, 0x71, 0x24, 0xD3, 0x3A, 0xAA, 0x04, 0x44, 0x10, 0x7D, 0x2C, 0xED, 0x50, 0xD0, 0x35, 0x16, 0x98, 0x2D, 0x18, 0xDB, 0xB5, 0x4A, 0xE6, 0x6C, 0x65, 0xEB, 0x6A, 0x1F, 0x11, 0x97, 0xC9, 0xD6, 0x64, 0x8B, 0x51, 0x4A, 0x68, 0xA7, 0x6E, 0x39, 0x7A, 0xCE, 0xC4, 0x80, 0xAC, 0x25, 0x84, 0xAC, 0x02, 0xBA, 0x45, 0x05, 0x3D, 0x31, 0x86, 0x87, 0x61, 0x91, 0x50, 0x62, 0xB5, 0xF5, 0x93, 0xCF, 0x77, 0x9C, 0xF9, 0x9D, 0xDF, 0x19, 0xB9, 0x33, 0x5B, 0xD4, 0xC9, 0x13, 0x46, 0x64, 0x92, 0x97, 0xAA, 0x90, 0x22, 0x3C, 0x6B, 0xFF, 0x78, 0xDA, 0x73, 0xD4, 0x2A, 0x9D, 0xE6, 0x8E, 0x9D, 0xC4, 0xDD, 0x9B, 0x4D, 0xEC, 0xB5, 0x13, 0xCE, 0x89, 0xFA, 0x33, 0x4C, 0xCE, 0x1C, 0x87, 0xFC, 0x26, 0x89, 0x9B, 0xCE, 0x26, 0xEE, 0xD5, 0x3F, 0xD3, 0xF3, 0xC6, 0xB4, 0x68, 0x32, 0xFA, 0x9D, 0xDB, 0x72, 0x50, 0x35, 0x10, 0xF7, 0x5B, 0xD7, 0x4A, 0x56, 0xCC, 0xC2, 0xDE, 0xA2, 0xC6, 0x55, 0x24, 0xBB, 0x9D, 0x6C, 0x09, 0x0A, 0xCC, 0x74, 0xC1, 0x4C, 0x6D, 0x60, 0x25, 0xA0, 0x57, 0xC5, 0x7D, 0xE7, 0x4D, 0xD1, 0xBB, 0xAD, 0xE4, 0x4D, 0x4E, 0xA0, 0x79, 0xC7, 0xDB, 0x09, 0xE3, 0xDF, 0x1F, 0x66, 0x9D, 0xB0, 0x0F, 0xD4, 0x31, 0x80, 0xE3, 0xD5, 0x25, 0x9A, 0x8D, 0x9F, 0xE3, 0xE6, 0x3E, 0x1E, 0x11, 0x99, 0xCC, 0xAB, 0x5B, 0x36, 0xBB, 0xFB, 0x05, 0x7B, 0xD6, 0x01, 0x9E, 0x82, 0x50, 0xB4, 0x6D, 0xAE, 0xB8, 0x25, 0xE9, 0x2F, 0xB6, 0x8B, 0x57, 0xE7, 0x6C, 0xC4, 0xC5, 0x9F, 0xAA, 0x9C, 0x52, 0x59, 0xA1, 0x00, 0x1E, 0x38, 0xC5, 0xAB, 0xCE, 0x1F, 0x2F, 0xD4, 0x98, 0x6F, 0x9B, 0x10, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xE0,
};

int mpeg4_test(int argc, char **argv)
{
    uint8_t *output = 0;
    uint32_t output_len = 0;

    if (mpeg4_latm_decode((uint8_t *)test_buffer, sizeof(test_buffer), &output, &output_len))
    {
        mpeg_loge("decode err");
    }

    mpeg_loge("total %d, output offset %d len %d, %s", sizeof(test_buffer), output - test_buffer, output_len,
              (sizeof(test_buffer) - (output - test_buffer) == output_len ? "len ok" : "len not match"));

    if (mpeg4_latm_decode((uint8_t *)test_buffer2, sizeof(test_buffer2), &output, &output_len))
    {
        mpeg_loge("decode err");
    }

    mpeg_loge("total %d, output offset %d len %d, %s", sizeof(test_buffer2), output - test_buffer2, output_len,
              (sizeof(test_buffer2) - (output - test_buffer2) == output_len ? "len ok" : "len not match"));

    return 0;
}

#endif
