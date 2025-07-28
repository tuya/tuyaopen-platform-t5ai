#if defined( TEE_M )
#include "psa_sp_api.h"
#else
#include <os/os.h>
#endif

#include "pal_time.h"

extern void psa_udelay(uint32_t us);
extern void psa_mdelay(uint32_t msecs);

void pal_msleep( uint32_t msecs )
{
#if defined( TEE_M )
    psa_msleep( msecs );
#else
    rtos_delay_milliseconds(msecs);
#endif
}

void pal_udelay( uint32_t usecs )
{
#if defined( TEE_M )
    psa_udelay( usecs );
#else
    psa_udelay( usecs );
#endif
}

/*************************** The End Of File*****************************/
