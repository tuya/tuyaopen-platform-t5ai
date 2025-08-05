#pragma once

#include "components/log.h"
#include "os/mem.h"

enum
{
    MPEG4_LATM_DEBUG_LEVEL_ERROR,
    MPEG4_LATM_DEBUG_LEVEL_WARNING,
    MPEG4_LATM_DEBUG_LEVEL_INFO,
    MPEG4_LATM_DEBUG_LEVEL_DEBUG,
    MPEG4_LATM_DEBUG_LEVEL_VERBOSE,
};


#define MPEG4_LATM_DEBUG_LEVEL MPEG4_LATM_DEBUG_LEVEL_INFO

#define mpeg_loge(format, ...) do{if(MPEG4_LATM_DEBUG_LEVEL >= MPEG4_LATM_DEBUG_LEVEL_ERROR)   BK_LOGE("mpeg4", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define mpeg_logw(format, ...) do{if(MPEG4_LATM_DEBUG_LEVEL >= MPEG4_LATM_DEBUG_LEVEL_WARNING) BK_LOGW("mpeg4", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define mpeg_logi(format, ...) do{if(MPEG4_LATM_DEBUG_LEVEL >= MPEG4_LATM_DEBUG_LEVEL_INFO)    BK_LOGI("mpeg4", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define mpeg_logd(format, ...) do{if(MPEG4_LATM_DEBUG_LEVEL >= MPEG4_LATM_DEBUG_LEVEL_DEBUG)   BK_LOGD("mpeg4", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define mpeg_logv(format, ...) do{if(MPEG4_LATM_DEBUG_LEVEL >= MPEG4_LATM_DEBUG_LEVEL_VERBOSE) BK_LOGV("mpeg4", "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)

int mpeg4_latm_decode(uint8_t *input, uint32_t input_len, uint8_t **output, uint32_t *output_len);
