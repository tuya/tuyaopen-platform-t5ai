#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>


#define APP_TIMEOUT_VALUE    BEKEN_WAIT_FOREVER

extern int bk_ipc_init(void);

int main(void)
{
	bk_init();
	return 0;
}