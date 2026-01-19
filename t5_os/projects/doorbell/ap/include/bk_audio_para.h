#ifndef __BK_AUDIO_PARA_H__
#define __BK_AUDIO_PARA_H__

#include <components/audio_param_ctrl.h>

/**
 * @brief 获取应用音频自定义参数
 * @param service_type 服务类型
 * @return 音频参数结构体指针，如果服务类型不支持则返回NULL
 */
app_aud_para_t *get_app_aud_cust_para(app_aud_service_type_t service_type);

/**
 * @brief 设置应用音频自定义服务句柄
 * @param service_handle 服务句柄
 * @param service_type 服务类型
 * @return 无
 */
void set_app_aud_cust_service_handle(void *service_handle, app_aud_service_type_t service_type);

#endif /* __BK_AUDIO_PARA_H__ */