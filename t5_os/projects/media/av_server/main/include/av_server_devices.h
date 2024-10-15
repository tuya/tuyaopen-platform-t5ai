#ifndef __AV_SERVER_DEVICES_H__
#define __AV_SERVER_DEVICES_H__

#include "av_server_transmission.h"

#define DB_SAMPLE_RARE_8K (8000)
#define DB_SAMPLE_RARE_16K (16000)

#define UVC_DEVICE_ID (0xFDF6)


typedef struct
{
	uint16_t id;
	uint16_t rotate;
	uint16_t fmt;
} lcd_parameters_t;

typedef enum
{
	LCD_STATUS_CLOSE,
	LCD_STATUS_OPEN,
	LCD_STATUS_UNKNOWN,
} lcd_status_t;

typedef enum
{
	CODEC_FORMAT_UNKNOW = 0,
	CODEC_FORMAT_G711A = 1,
	CODEC_FORMAT_PCM = 2,
	CODEC_FORMAT_G711U = 3,
} codec_format_t;

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
	uint8_t aec;
	uint8_t uac;
	uint8_t rmt_recoder_fmt; /* codec_format_t */
	uint8_t rmt_player_fmt; /* codec_format_t */
	uint32_t rmt_recorder_sample_rate;
	uint32_t rmt_player_sample_rate;
} audio_parameters_t;


typedef enum
{
	AA_UNKNOWN = 0,
	AA_ECHO_DEPTH = 1,
	AA_MAX_AMPLITUDE = 2,
	AA_MIN_AMPLITUDE = 3,
	AA_NOISE_LEVEL = 4,
	AA_NOISE_PARAM = 5,
} audio_acoustics_t;


void av_server_devices_deinit(void);
int av_server_devices_init(void);

int av_server_devices_set_camera_transfer_callback(void *cb);
int av_server_devices_set_audio_transfer_callback(const void *cb);

int av_server_camera_turn_on(camera_parameters_t *parameters);
int av_server_camera_turn_off(void);

int av_server_audio_turn_on(audio_parameters_t *parameters);
int av_server_audio_turn_off(void);
int av_server_audio_acoustics(uint32_t index, uint32_t param);
void av_server_audio_data_callback(uint8_t *data, uint32_t length);

int av_server_display_turn_on(uint16_t id, uint16_t rotate, uint16_t fmt);
int av_server_display_turn_off(void);

int av_server_video_transfer_turn_on(void);
int av_server_video_transfer_turn_off(void);


#endif
