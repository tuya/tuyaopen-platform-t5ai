#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <media_service.h>
#include "bk_smart_config.h"
#include <doorbell_comm.h>


int main(void)
{
	bk_init();
    media_service_init();

#if CONFIG_INTEGRATION_DOORBELL
    bk_smart_config_init();
    doorbell_core_init();

#if (CONFIG_ASR_SERVICE_WITH_MIC)
	extern int doorbell_asr_turn_on(void);
	doorbell_asr_turn_on();
#endif

#endif
	return 0;
}
