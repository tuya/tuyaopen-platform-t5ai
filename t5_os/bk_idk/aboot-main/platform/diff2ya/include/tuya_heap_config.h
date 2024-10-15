/*
   
*/

#ifndef TUYA_HEAP_CONFIG_H
#define TUYA_HEAP_CONFIG_H

#include "ty_bsdiff_adapt.h"
/*-----------------------------------------------------------

 *----------------------------------------------------------*/

/* Memory allocation related definitions. */
#ifndef uint32_t
#define uint32_t unsigned int
#endif

#ifndef uint8_t
#define uint8_t unsigned char
#endif

#if 1
extern volatile uint32_t g_u32TuyaHeapSize;
extern volatile uint32_t g_u32TuyaHeapBase;

#define portBYTE_ALIGNMENT			    8
#define portBYTE_ALIGNMENT_MASK         ( 0x0007 )

#define PRIVILEGED_DATA 

#ifndef mtCOVERAGE_TEST_MARKER
	#define mtCOVERAGE_TEST_MARKER()
#endif

#define configASSERT(x) do { \
							if( (x) == 0 ) { \
                                ty_adapt_print("\n\rAssert(" #x ") failed on line %d in file %s\r\n", __LINE__, __FILE__);\
								for(;;);} \
							} while(0)

#define myconfigTOTAL_HEAP_SIZE         g_u32TuyaHeapSize
#define BLE_RAM_STA_ADDR                g_u32TuyaHeapBase
#else
#define myconfigTOTAL_HEAP_SIZE                   ( ( unsigned int ) ( 1024*12 ) )
#define BLE_RAM_STA_ADDR 0x00416600
#endif
extern void * ty_pvPortMalloc( unsigned int xWantedSize );
extern void ty_vPortFree( void * pv );
extern int prvHeapInit( void *start_addr, uint32_t len);
extern size_t xPortGetFreeHeapSize( void );

#endif /* TUYA_HEAP_CONFIG_H */

