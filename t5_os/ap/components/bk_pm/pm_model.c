#include <common/bk_include.h>
#include "arm_arch.h"
#include <common/sys_config.h>
#include "bk_drv_model.h"
#include "bk_pm_model.h"
#include <os/mem.h>
#include <os/str.h>
//#include "bk_mcu_ps.h"
//#include "bk_rf_ps.h"


DRV_PM_S dev_pm_table[PM_ID_MAX] = {
	{0},
};


UINT32 dev_pm_init(void)
{
	extern void mac_ps_pm_init(void);
	extern void rf_ps_pm_init(void);
	extern void flash_ps_pm_init(void);
	extern void ble_ps_pm_init(void);
	os_memset(dev_pm_table, 0, sizeof(dev_pm_table));

	mac_ps_pm_init();
	rf_ps_pm_init();
	flash_ps_pm_init();

#if CONFIG_BLUETOOTH
	ble_ps_pm_init();
#endif

	return 0;
}


int dev_pm_register(UINT32 id, char *name, DEV_PM_OPS_PTR pmptr)
{
	if (id > PM_ID_MAX)
		return -1;

	dev_pm_table[id].name = name;
	dev_pm_table[id].ops = pmptr;
	dev_pm_table[id].state = PM_USABLE;
	return 0;
}

UINT32 dev_pm_unregister(UINT32 id)
{
	if (id > PM_ID_MAX)
		return -1;

	dev_pm_table[id].name = NULL;
	dev_pm_table[id].ops = NULL;
	dev_pm_table[id].state = PM_UNUSABLE;
	return 0;
}

