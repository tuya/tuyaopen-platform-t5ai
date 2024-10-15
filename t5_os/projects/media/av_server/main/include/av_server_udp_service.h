#ifndef __AV_SERVER_UDP_SERVICE_H__
#define __AV_SERVER_UDP_SERVICE_H__

#include <os/os.h>
#include "av_server_transmission.h"

#define APP_DEMO_UDP_SOCKET_TIMEOUT     100  // ms
#define UDP_MAX_RETRY (1000)
#define UDP_MAX_DELAY (20)

//#if (!CONFIG_AV_DEMO_MODE_TCP)
extern beken_semaphore_t s_av_server_service_sem;
//#endif

bk_err_t av_server_udp_service_init(void);
void av_server_udp_service_deinit(void);

#endif
