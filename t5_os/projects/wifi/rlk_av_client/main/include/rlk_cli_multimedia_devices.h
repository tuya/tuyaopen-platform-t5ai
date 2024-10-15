#ifndef __RLK_CLI_MULTIMEDIA_DEVICES_H__
#define __RLK_CLI_MULTIMEDIA_DEVICES_H__

#include "media_app.h"
#include "rlk_cli_multimedia_transmission.h"

#define UVC_DEVICE_ID (0xFDF6)

typedef struct
{
    uint16_t camera_id;
    uint16_t lcd_id;
    uint16_t video_transfer;
    uint8_t pipeline_enable;
    uint8_t audio_enable;
    media_transfer_cb_t *camera_transfer_cb;
    const media_transfer_cb_t *audio_transfer_cb;
} db_device_info_t;

typedef struct
{
    uint16_t id;
    uint16_t width;
    uint16_t height;
    uint16_t format;
    uint16_t protocol;
    uint16_t rotate;
} camera_parameters_t;

typedef struct
{
    uint16_t id;
    uint16_t rotate;
    uint16_t fmt;
} lcd_parameters_t;

typedef struct
{
    uint8_t aec;
    uint8_t uac;
    uint8_t rmt_recoder_fmt; /* codec_format_t */
    uint8_t rmt_player_fmt; /* codec_format_t */
    uint32_t rmt_recorder_sample_rate;
    uint32_t rmt_player_sample_rate;
} audio_parameters_t;


void mm_devices_deinit(void);
int mm_devices_init(void);

int mm_devices_set_camera_transfer_callback(void *cb);
int mm_devices_set_audio_transfer_callback(const void *cb);

int mm_camera_turn_on(camera_parameters_t *parameters);
int mm_camera_turn_off(void);

int mm_audio_turn_on(audio_parameters_t *parameters);
int mm_audio_turn_off(void);
void mm_audio_data_callback(uint8_t *data, uint32_t length);

int mm_display_turn_on(uint16_t id, uint16_t rotate, uint16_t fmt);
int mm_display_turn_off(void);

int mm_video_transfer_turn_on(void);
int mm_video_transfer_turn_off(void);


#endif     //__RLK_CLI_MULTIMEDIA_DEVICES_H__