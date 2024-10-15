#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#ifdef CONFIG_RTT
#include <sys/socket.h>
#endif
#include "bk_uart.h"
#include <components/video_transfer.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include "aud_intf.h"
#include "aud_intf_types.h"
#include "lcd_act.h"
#include "media_app.h"
#include "wifi_transfer.h"
#include <modules/raw_link.h>
#include "rlk_control_client.h"
#include "rlk_multimedia_client.h"
#include "rlk_cli_multimedia_devices.h"

db_mm_service_t *db_mm_service = NULL;

camera_parameters_t camera_parameters = {UVC_DEVICE_ID,800,480,0,1,65535};
lcd_parameters_t lcd_parameters = {10,90,0};
audio_parameters_t audio_parameters = {1,0,1,1,8000,8000};

static inline void rlk_mm_client_voice_recv(db_channel_t *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length)
{
    LOGD("%s %d\n", __func__, length);
    mm_audio_data_callback(data, length);
}
static inline void rlk_mm_server_video_recv(db_channel_t *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length)
{
    //wifi_transfer_data_check(data,length);
#if (CONFIG_MEDIA_APP)
    wifi_transfer_net_send_data(data, length, TVIDEO_SND_UDP);
#endif
}

bk_err_t rlk_mm_client_recv(uint8_t *data, uint32_t len)
{
    rlk_mm_pre_header_t *header = (rlk_mm_pre_header_t *)data;

    if (header->data_type != RLK_MM_HEADER_TYPE_DATA || (db_mm_service == NULL))
    {
        return BK_FAIL;
    }

    //video
    if (header->data_subtype == RLK_MM_HEADER_DATA_SUBTYPE_VIDEO)
    {
        LOGD("video subtype %x\n", header->data_subtype);
        //mm_transmission_unpack(db_mm_service->img_channel, &data[RLK_MM_PRE_HEADER_LEN], (len - RLK_MM_PRE_HEADER_LEN), rlk_mm_server_video_recv);
    }
    else if (header->data_subtype == RLK_MM_HEADER_DATA_SUBTYPE_AUDIO)
    {
        LOGD("audio subtype %x len %d\n", header->data_subtype,len);
        mm_transmission_unpack(db_mm_service->aud_channel, &data[RLK_MM_PRE_HEADER_LEN], (len - RLK_MM_PRE_HEADER_LEN), rlk_mm_client_voice_recv);
    }
    else
    {
        LOGE("unknow data subtype %x\n", header->data_subtype);
        return BK_FAIL;
    }

    return BK_OK;
}

int rlk_mm_client_video_send_packet(uint8_t *data, uint32_t len, uint16_t *retry_cnt)
{
    int send_byte = BK_OK;
    uint8_t *media_data;
    rlk_mm_pre_header_t *header = NULL;
    uint8_t *ptr = data - sizeof(db_trans_head_t);
    uint16_t size = len + sizeof(db_trans_head_t);

    if (!db_mm_service->img_status)
    {
        return BK_FAIL;
    }

    if (rlk_client_local_env.state != RLK_STATE_CONNECTED)
    {
        return BK_FAIL;
    }

    media_data = os_malloc(RLK_MM_PRE_HEADER_LEN + size);

     if (!media_data)
     {
        LOGE("%s %d NO MEMORY\r\n",__func__,__LINE__);
        return BK_FAIL;
     }

    //wifi_transfer_data_check(data,len);
    //wifi_transfer_data_check(ptr,len);

    header = (rlk_mm_pre_header_t *)media_data;

    os_memcpy(media_data + RLK_MM_PRE_HEADER_LEN, ptr, size);

    header->data_type = RLK_MM_HEADER_TYPE_DATA;
    header->data_subtype = RLK_MM_HEADER_DATA_SUBTYPE_VIDEO;

    send_byte = bk_rlk_send(rlk_client_local_env.peer_mac_addr, media_data, (RLK_MM_PRE_HEADER_LEN + size));
    os_free(media_data);

    if (send_byte < 0)
    {
        /* err */
        LOGE("send return fd:%d\n", send_byte);
        send_byte = BK_FAIL;
    }

    return (send_byte == (RLK_MM_PRE_HEADER_LEN + size) ? len : send_byte);
}

int rlk_mm_client_img_send_prepare(uint8_t *data, uint32_t length)
{
    mm_transmission_pack(db_mm_service->img_channel, data, length);

    return 0;
}

void *rlk_mm_client_img_get_tx_buf(void)
{
    if (db_mm_service == NULL)
    {
        LOGE("%s, service null\n",__func__);
        return NULL;
    }

    if (db_mm_service->img_channel == NULL)
    {
        LOGE("%s, img_channel null\n",__func__);
        return NULL;
    }

    LOGD("%s, tbuf %p\n", __func__,db_mm_service->img_channel->tbuf);

    return db_mm_service->img_channel->tbuf + 1;
}

int rlk_mm_client_img_get_tx_size(void)
{
    if (db_mm_service == NULL)
    {
        LOGE("%s, service null\n",__func__);
        return 0;
    }

    if (db_mm_service->img_channel == NULL)
    {
        LOGE("%s, img_channel null\n",__func__);
        return 0;
    }

    return db_mm_service->img_channel->tsize - sizeof(db_trans_head_t);
}

int rlk_mm_client_voice_send_packet(uint8_t *data, uint32_t len, uint16_t *retry_cnt)
{
    int send_byte = BK_OK;
    uint8_t *media_data;
    rlk_mm_pre_header_t *header = NULL;
    uint8_t *ptr = data - sizeof(db_trans_head_t);
    uint16_t size = len + sizeof(db_trans_head_t);

    if (!db_mm_service->aud_status)
    {
        return BK_FAIL;
    }

    if (rlk_client_local_env.state != RLK_STATE_CONNECTED)
    {
        return BK_FAIL;
    }

    media_data = os_malloc(RLK_MM_PRE_HEADER_LEN + size);

    if (!media_data)
    {
        LOGE("%s %d NO MEMORY\r\n",__func__,__LINE__);
        return BK_FAIL;
    }

    LOGD("voice len %d\n", size);

    //wifi_transfer_data_check(data,len);

    header = (rlk_mm_pre_header_t *)media_data;

    os_memcpy(media_data + RLK_MM_PRE_HEADER_LEN, ptr, size);
    header->data_type = RLK_MM_HEADER_TYPE_DATA;
    header->data_subtype = RLK_MM_HEADER_DATA_SUBTYPE_AUDIO;

    send_byte = bk_rlk_send(rlk_client_local_env.peer_mac_addr, media_data, (RLK_MM_PRE_HEADER_LEN + size));
    os_free(media_data);

    if (send_byte < (RLK_MM_PRE_HEADER_LEN + size))
    {
        /* err */
        LOGE("need_send: %d, send_complete: %d\n", (RLK_MM_PRE_HEADER_LEN + size), send_byte);
        send_byte = BK_FAIL;
    }

    return (send_byte == (RLK_MM_PRE_HEADER_LEN + size) ? len : send_byte);
}

int rlk_mm_client_aud_send_prepare(uint8_t *data, uint32_t length)
{
    mm_transmission_pack(db_mm_service->aud_channel, data, length);

    return 0;
}

void *rlk_mm_client_aud_get_tx_buf(void)
{
    if (db_mm_service == NULL)
    {
        LOGE("%s, service null\n",__func__);
        return NULL;
    }

    if (db_mm_service->aud_channel == NULL)
    {
        LOGE("%s, aud_channel null\n",__func__);
        return NULL;
    }

    LOGD("%s, tbuf %p\n",__func__, db_mm_service->aud_channel->tbuf);

    return db_mm_service->aud_channel->tbuf + 1;
}

int rlk_mm_client_aud_get_tx_size(void)
{
    if (db_mm_service == NULL)
    {
        LOGE("%s, service null\n",__func__);
        return 0;
    }

    if (db_mm_service->aud_channel == NULL)
    {
        LOGE("%s, img_channel null\n",__func__);
        return 0;
    }

    return db_mm_service->aud_channel->tsize - sizeof(db_trans_head_t);
}

static media_transfer_cb_t rlk_mm_client_img_channel =
{
    .send = rlk_mm_client_video_send_packet,
    .prepare = rlk_mm_client_img_send_prepare,
    .get_tx_buf = rlk_mm_client_img_get_tx_buf,
    .get_tx_size = rlk_mm_client_img_get_tx_size,
    .fmt = PIXEL_FMT_JPEG
};

static const media_transfer_cb_t rlk_mm_client_aud_channel =
{
    .send = rlk_mm_client_voice_send_packet,
    .prepare = rlk_mm_client_aud_send_prepare,
    .get_tx_buf = rlk_mm_client_aud_get_tx_buf,
    .get_tx_size = rlk_mm_client_aud_get_tx_size,
    .fmt = PIXEL_FMT_UNKNOW
};

bk_err_t rlk_mm_client_video_aud_init(void)
{
    GLOBAL_INT_DECLARATION();
    int ret;

    GLOBAL_INT_DISABLE();
    db_mm_service->img_status = 1;
    db_mm_service->running = 1;
    db_mm_service->aud_status = 1;
    GLOBAL_INT_RESTORE();

    ret = mm_camera_turn_on(&camera_parameters);

    if (ret != BK_OK)
    {
        LOGE("turn on video transfer failed\n");
        goto out;
    }

    ret = mm_video_transfer_turn_on();

    if (ret != BK_OK)
    {
        LOGE("turn on camera failed\n");
        goto out;
    }

    ret = mm_display_turn_on(lcd_parameters.id, lcd_parameters.rotate, lcd_parameters.fmt);

    if (ret != BK_OK)
    {
        LOGE("turn on display failed\n");
        goto out;
    }

    ret = mm_audio_turn_on(&audio_parameters);

    if (ret != BK_OK)
    {
        LOGE("turn on audio failed\n");
        goto out;
    }

    return BK_OK;

out:

    LOGE("%s exit %d\n",__func__, db_mm_service->running);

    rlk_mm_client_video_aud_deinit();

    return BK_FAIL;
}

void rlk_mm_client_prepare_sleep(void)
{
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    if (db_mm_service != NULL && db_mm_service->running)
    {
        rlk_mm_client_video_aud_deinit();
    }

    GLOBAL_INT_RESTORE();
}

bk_err_t rlk_mm_client_init(void)
{
    int ret;

    LOGI("%s %d\n", __func__,__LINE__);

    /*********************< db info init >**************************/
    if (db_mm_service != NULL)
    {
        LOGE("%s already init\n",__func__);
        return BK_FAIL;
    }

    ret = mm_devices_init();

    if (ret != BK_OK)
    {
        LOGE("db_mm_service malloc failed\n");
        goto error;
    }

    db_mm_service = os_malloc(sizeof(db_mm_service_t));

    if (db_mm_service == NULL)
    {
        LOGE("db_mm_service malloc failed\n");
        goto error;
    }

    os_memset(db_mm_service, 0, sizeof(db_mm_service_t));

    /*********************< video init start >**************************/
    db_mm_service->img_channel = mm_transmission_malloc(1600, MM_VIDEO_NETWORK_MAX_SIZE);

    if (db_mm_service->img_channel == NULL)
    {
        LOGE("img_channel malloc failed\n");
        goto error;
    }

    mm_devices_set_camera_transfer_callback(&rlk_mm_client_img_channel);

    /*********************< video init end >**************************/

    /*********************< voice init start >**************************/
    db_mm_service->aud_channel = mm_transmission_malloc(1600, MM_AUDIO_NETWORK_MAX_SIZE);

    if (db_mm_service->aud_channel == NULL)
    {
        LOGE("aud_channel malloc failed\n");
        goto error;
    }

    mm_devices_set_audio_transfer_callback(&rlk_mm_client_aud_channel);

    /*********************< voice init end >**************************/
    rlk_mm_client_video_aud_init();

    LOGI("db_mm_service->img_channel %p\n", db_mm_service->img_channel);
    LOGI("db_mm_service->aud_channel %p\n", db_mm_service->aud_channel);

    return BK_OK;

error:

    rlk_mm_client_video_aud_deinit();

    return BK_FAIL;
}


void rlk_mm_client_video_aud_deinit(void)
{
    GLOBAL_INT_DECLARATION();

    LOGI("%s\n", __func__);

    if (db_mm_service == NULL)
    {
        LOGE("%s service is NULL\n", __func__);
        return;
    }

    if (db_mm_service->running)
    {
        mm_video_transfer_turn_off();
        mm_camera_turn_off();
        mm_audio_turn_off();
        mm_display_turn_off();
    }

    mm_devices_deinit();

    if (db_mm_service->aud_channel)
    {
        os_free(db_mm_service->aud_channel);
        db_mm_service->aud_channel = NULL;
    }

    if (db_mm_service->img_channel)
    {
        os_free(db_mm_service->img_channel);
        db_mm_service->img_channel = NULL;
    }

    GLOBAL_INT_DISABLE();
    db_mm_service->running == 0;
    GLOBAL_INT_RESTORE();

    os_free(db_mm_service);
    db_mm_service = NULL;
}
