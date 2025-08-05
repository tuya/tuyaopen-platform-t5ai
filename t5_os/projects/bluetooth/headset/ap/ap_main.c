#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "bt_manager.h"
#include "media_service.h"

#define AUTO_ENABLE_BLUETOOTH_DEMO 1

int main(void)
{
	bk_init();

	media_service_init();

#if AUTO_ENABLE_BLUETOOTH_DEMO
	bt_manager_init();

#if CONFIG_A2DP_SINK_DEMO
	extern int a2dp_sink_demo_init(uint8_t aac_supported);
	a2dp_sink_demo_init(0);
#endif

#if CONFIG_HFP_HF_DEMO
	extern int hfp_hf_demo_init(uint8_t msbc_supported);
	hfp_hf_demo_init(0);
#endif

#if CONFIG_BT
	extern int cli_headset_demo_init(void);
	cli_headset_demo_init();
#endif

#endif
	return 0;
}
