#ifndef __RLK_MULTIMEDIA_CLIENT_H__
#define __RLK_MULTIMEDIA_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rlk_cli_multimedia_transmission.h"

#define MM_VIDEO_NETWORK_MAX_SIZE       (1472)
#define MM_AUDIO_NETWORK_MAX_SIZE       (1024)

typedef struct
{
    uint16_t running : 1;
    uint16_t img_status : 1;
    uint16_t aud_status : 1;
    db_channel_t *img_channel;
    db_channel_t *aud_channel;
} db_mm_service_t;

bk_err_t rlk_mm_client_recv(uint8_t *data, uint32_t len);
bk_err_t rlk_mm_client_init(void);
void rlk_mm_client_video_aud_deinit(void);
void rlk_mm_client_prepare_sleep(void);

#ifdef __cplusplus
}
#endif

#endif  ///__RLK_MULTIMEDIA_CLIENT_H__