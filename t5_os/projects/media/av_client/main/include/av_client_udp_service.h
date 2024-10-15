#ifndef __AV_CLIENT_UDP_SERVICE_H__
#define __AV_CLIENT_UDP_SERVICE_H__

#include <os/os.h>
#include "av_client_transmission.h"

#define APP_DEMO_UDP_SOCKET_TIMEOUT     100  // ms
#define UDP_MAX_RETRY (1000)
#define UDP_MAX_DELAY (20)

#if (!CONFIG_AV_DEMO_MODE_TCP)
extern beken_semaphore_t s_av_client_service_sem;
#endif

bk_err_t av_client_udp_service_init(void);
void av_client_udp_service_deinit(void);

#endif
