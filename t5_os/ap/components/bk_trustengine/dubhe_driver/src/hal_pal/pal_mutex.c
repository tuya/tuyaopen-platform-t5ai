#include "pal_mutex.h"
#include "os/os.h"

pal_mutex_t pal_mutex_init( void )
{
	pal_mutex_t mutex = NULL;

	u32 status = rtos_enter_critical();
	rtos_init_mutex(&mutex);
	rtos_exit_critical(status);

	return mutex;
}

int32_t pal_mutex_lock( pal_mutex_t m )
{
	if(rtos_lock_mutex(&m) == 0)
		return ( MUTEX_LOCK_SUCCESS );
	else
		return ( MUTEX_LOCK_FAIL );
}

int32_t pal_mutex_unlock( pal_mutex_t m )
{
	if(rtos_unlock_mutex(&m) == 0)
		return ( MUTEX_LOCK_SUCCESS );
	else
		return ( MUTEX_UNLOCK_FAIL );
}

int32_t pal_mutex_destroy( pal_mutex_t m )
{
	u32 status = rtos_enter_critical();
	rtos_deinit_mutex(&m);
	rtos_exit_critical(status);

	return ( MUTEX_LOCK_SUCCESS );
}

/*************************** The End Of File*****************************/


