#include "pal_semaphore.h"
#include <os/os.h>
#include "pal_log.h"

#define DUBHE_WAIT_SEMA_MS (5000)

void pal_sema_init( semaphore_t *sem, int val )
{  
	rtos_init_semaphore(sem, val);
	return;
}
int pal_sema_down( semaphore_t sem )
{
	bk_err_t ret = 0;
	ret = rtos_get_semaphore(&sem, DUBHE_WAIT_SEMA_MS);
	if(ret != 0)
	{
		PAL_LOG_ERR("%s 0x%x ret = %d \r\n",__func__, &sem, ret);
		return -1;
	}
	return 0;
}
int pal_sema_up( semaphore_t sem )
{
	bk_err_t ret = 0;
	ret = rtos_set_semaphore(&sem);
	if(ret != 0){
		PAL_LOG_ERR("%s 0x%x ret = %d \r\n",__func__, &sem, ret);
		return -1;
	}
	return 0;
}
void pal_sema_destroy( semaphore_t sem )
{
	rtos_deinit_semaphore(&sem);
}
/*************************** The End Of File*****************************/

