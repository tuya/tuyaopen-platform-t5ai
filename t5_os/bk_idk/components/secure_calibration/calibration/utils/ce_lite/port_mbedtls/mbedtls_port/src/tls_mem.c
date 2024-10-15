#include "tls_config.h"

#ifdef MBEDTLS_PLATFORM_MEMORY
//#include <os/mem.h>
#include "heap.h"
void *tls_mbedtls_mem_calloc(size_t n, size_t size)
{
#if 0
	unsigned int len = n * size;
	if(len == 0){
		return 0;
	}
	
    return os_zalloc( len );
#else

     return kcalloc(n, size);
#endif
}

void tls_mbedtls_mem_free(void *ptr)
{
#if 0
   os_free(ptr);
#else
    kfree(ptr);
#endif
}

#endif /* !MBEDTLS_PLATFORM_MEMORY */
