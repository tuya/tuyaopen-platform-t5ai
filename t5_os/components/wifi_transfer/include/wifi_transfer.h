
#pragma once

#include <os/os.h>
#include <common/bk_include.h>
#include "media_app.h"
#include <driver/media_types.h>
#include <components/video_types.h>
#include <common/bk_err.h>
#include <driver/psram_types.h>

#include <trans_list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint8_t id;
	uint8_t eof;
	uint8_t cnt;
	uint8_t size;
	uint8_t data[];
} transfer_data_t;

#define WIFI_RECV_CAMERA_POOL_LEN			(1472 * 35)
#define WIFI_RECV_CAMERA_RXNODE_SIZE		1472

typedef struct {
	/// the video data receive complete
	beken_semaphore_t aready_semaphore;
	/// frame_buffer
	frame_buffer_t *frame;
	/// recoder the buff ptr of every time receive video packte
	uint8_t *buf_ptr;
	/// video buff receive state
	uint8_t start_buf;
	/// dma id for memcpy for sram -> sram
	uint8_t dma_id;
	/// dma id for memcpy sram - >psram
	uint8_t dma_psram;
	/// the packet count of one frame
	uint32_t frame_pkt_cnt;
} wifi_transfer_net_camera_buffer_t;

typedef struct {
	struct trans_list_hdr hdr;
	void *buf_start;
	uint32_t buf_len;
} wifi_transfer_net_camera_elem_t;

typedef struct {
	uint8_t *pool;
	wifi_transfer_net_camera_elem_t elem[WIFI_RECV_CAMERA_POOL_LEN / WIFI_RECV_CAMERA_RXNODE_SIZE];
	struct trans_list free;
	struct trans_list ready;
} wifi_transfer_net_camera_pool_t;

typedef struct {
	media_ppi_t ppi;
	pixel_format_t fmt;
	video_send_type_t send_type;
} wifi_transfer_net_camera_param_t;

#define wifi_transfer_data_check(data,length) wifi_transfer_data_check_caller((const char*)__FUNCTION__,__LINE__,data,length)

void wifi_transfer_data_check_caller(const char *func_name, int line,uint8_t *data, uint32_t length);

bk_err_t bk_wifi_transfer_frame_open(const media_transfer_cb_t *cb);
bk_err_t bk_wifi_transfer_frame_close(void);

bk_err_t wifi_transfer_net_camera_open(media_camera_device_t *device);
bk_err_t wifi_transfer_net_camera_close(void);
uint32_t wifi_transfer_net_send_data(uint8_t *data, uint32_t length, video_send_type_t type);

#ifdef __cplusplus
}
#endif

