/*************************************************************
 * @file        BK_System.h
 * @brief       Header file of BK_System.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 *
 * 2018-02-03   Murphy            add user memory function
 */
#ifndef  __BK_SYSTEM_H__
#define  __BK_SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "BK_config.h"

#if (MCU_CLK == MCU_CLK_26MHz)
#define tick_cnt_us         3
#define tick_cnt_ms         2700
#elif (MCU_CLK == MCU_CLK_120MHz)
#define tick_cnt_us         28
#define tick_cnt_ms         29500
#elif (MCU_CLK == MCU_CLK_180MHz)
#define tick_cnt_us         9
#define tick_cnt_ms         9000
#endif

#define CFG_BEKEN_OTA                (1)
#define BOOTLOADER_OTA_DEBUG         (1)

// define the IRQ handler attribute for this compiler
#define __IRQ                      //__irq

// define the FIQ handler attribute for this compiler
#define __FIQ                      //__irq

#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))

#define NUMBER_ROUND_UP(a,b)        (((a) + ((b)/2 - 1)) / (b))      /* [a + (b/2 - 1)] / b */
#define NUMBER_ROUND_DOWN(a,b)      ((a) / (b))

typedef unsigned char       BYTE;
typedef signed   char       int8;
typedef signed   short      int16;
typedef signed   long       int32;
typedef signed   long long  int64;
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned long       uint32;
typedef unsigned long long  uint64;
typedef float               fp32;
typedef double              fp64;
typedef signed   char       s8;
typedef signed   short      s16;
typedef signed   long       s32;
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned long       u32;
typedef unsigned long       uint32;
typedef unsigned char  		UINT8;      /* Unsigned  8 bit quantity        */
typedef signed   char  		INT8;       /* Signed    8 bit quantity        */
typedef unsigned short 		UINT16;     /* Unsigned 16 bit quantity        */
typedef signed   short 		INT16;      /* Signed   16 bit quantity        */
typedef unsigned int   		UINT32;     /* Unsigned 32 bit quantity        */
typedef signed   int   		INT32;      /* Signed   32 bit quantity        */
typedef unsigned long long  UINT64;		/* Unsigned 32 bit quantity        */
typedef signed   long long  INT64;		/* Signed   32 bit quantity        */
typedef float         		FP32;		/* Single precision floating point */
typedef double         		FP64;		/* Double precision floating point */
typedef unsigned int        size_t;
typedef unsigned char       BOOLEAN;

typedef uint32              uint32_t;
typedef enum
{
    FALSE = 0,
    TRUE = !FALSE
} bool, BOOL;

typedef enum        // by gwf
{
    OK = 0,
    ERROR = -1
} STATUS;

typedef enum        // by gwf
{
    NO = 0,
    YES = 1
} ASK;

#define kNoErr                      0       //! No error occurred.
#define kGeneralErr                -1       //! General error.
#define kInProgressErr              1       //! Operation in progress.

#if 1
#define ASSERT(exp)                                 \
{                                                   \
    if ( !(exp) )                                   \
    {                                               \
    	(printf("assert:%s:%d\r\n",__FUNCTION__,__LINE__))	;\
    }                                               \
}
#else
#define ASSERT(exp)
#endif

#if BOOTLOADER_OTA_DEBUG
#ifdef assert
#undef assert
#endif
#define assert(EXPR)                                                           \
if (!(EXPR))                                                                   \
{                                                                              \
    printf("(%s) has assert failed at %s.\n", #EXPR, __FUNCTION__);            \
    while (1);                                                                 \
}

/* error level log */
#ifdef  log_e
#undef  log_e
#endif
#define log_e(...)                     printf("[boot] (%s:%d) ", __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n")

/* info level log */
#ifdef  log_i
#undef  log_i
#endif
#define log_i(...)                     printf("[boot] "); printf(__VA_ARGS__);printf("\n")

/* debug level log */
#ifdef  log_d
#undef  log_d
#endif
#define log_d(...)                     printf("[boot] (%s:%d) ", __FUNCTION__, __LINE__);           printf(__VA_ARGS__);printf("\n")

#else

#ifdef assert
#undef assert
#endif
#define assert(EXPR)                   ((void)0);

/* error level log */
#ifdef  log_e
#undef  log_e
#endif
#define log_e(...)

/* info level log */
#ifdef  log_i
#undef  log_i
#endif
#define log_i(...)

/* debug level log */
#ifdef  log_d
#undef  log_d
#endif
#define log_d(...)
#endif /* RT_OTA_DEBUG */

#ifndef min_def
#define min_def
#define min( a, b ) ( ( a ) < ( b ) ) ? ( a ) : ( b )
#endif

#ifndef max_def
#define max_def
#define max( a, b ) ( ( a ) > ( b ) ) ? ( a ) : ( b )
#endif

/***************os config***********************/
/*Need also modify value __OS_ALIOS __OS_FREERTOS to {TRUE} or {FALSE} in boot_handler.s line:18*/

#define OS_FREERTOS                             1
#define OS_ALIOS                                2
#define OS_RTTOS                                3
#define CFG_SUPPORT_OS                             OS_RTTOS

#define     FOR_TUYA_FREERTOS                       1
#if FOR_TUYA_FREERTOS
//set __OS_FREERTOS for os FREERTOS
#define CFG_SUPPORT_OS                             OS_RTTOS
#endif

    /**************addr config**********************/
#if (CFG_SUPPORT_OS == OS_FREERTOS)
#define OS_CRC_BIN_START_ADDR 0x22000
#define OS_EX_ADDR            0x20000
#define OS1_HD_ADDR           0x19000
#define OTA_TWO_IMAGE_SWITCH_RUN       0
#elif (CFG_SUPPORT_OS == OS_ALIOS)
#define OS_CRC_BIN_START_ADDR 0x8800
#define OS_EX_ADDR            0x8000
#define OS1_HD_ADDR           0x1EF000
#define OTA_TWO_IMAGE_SWITCH_RUN       1
#else
//compatibility with alios,alios use rtt OTAPackageTool pack can also use rtt ota process
#define OS_EX_ADDR            0x10000
#endif

/***********************************************/
#define     START_TYPE_ADDR              (0x0080a080)
#define     CRASH_XAT0_VALUE              0xbedead00
#define     CRASH_UNDEFINED_VALUE         0xbedead01
#define     CRASH_PREFETCH_ABORT_VALUE    0xbedead02
#define     CRASH_DATA_ABORT_VALUE        0xbedead03
#define     CRASH_UNUSED_VALUE            0xbedead04

#define     CRASH_IN_BOOT_RESET1_VALUE            0xbedead10
#define     CRASH_IN_BOOT_RESET2_VALUE            0xbedead11
#define     CRASH_IN_BOOT_ABNORMAL_VALUE          0xbedead12

extern void DelayNops(volatile unsigned long nops);
extern void DelayUS(volatile unsigned long timesUS);
extern void DelayMS(volatile unsigned long timesMS);
extern void *malloc(unsigned int size);
extern void *realloc(void *mem_address, unsigned int newsize);
extern void *calloc(unsigned int n, unsigned int size);
extern void free(void *ptr);
extern void dump_hex(const uint8_t *ptr, size_t buflen);
extern u32 boot_downloading;
extern u32 uart_dl_port;

#define rt_malloc(size)                     malloc(size)
#define rt_realloc(mem_address, newsize)    realloc(mem_address, newsize)
#define rt_calloc(n, size)                  calloc(n, size)
#define rt_free(ptr)                        free(ptr)

#define REG_READ(addr)          *((volatile u32 *)(addr))
#define REG_WRITE(addr, _data) 	(*((volatile u32 *)(addr)) = (_data))

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __BK_SYSTEM_H__ */
