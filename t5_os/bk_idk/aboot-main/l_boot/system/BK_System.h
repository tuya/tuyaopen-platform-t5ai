/*************************************************************
 * @file        BK_System.h
 * @brief       Header file of BK_System.c
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

#ifndef min_def
#define min_def
#define min( a, b ) ( ( a ) < ( b ) ) ? ( a ) : ( b )
#endif

#ifndef max_def
#define max_def
#define max( a, b ) ( ( a ) > ( b ) ) ? ( a ) : ( b )
#endif

typedef struct
{
    UINT32 irq_vector;
    UINT32 fiq_vector;
    UINT32 swi_vector;
    UINT32 pad[1];

} world_struct;

typedef enum
{
    BK_OTA_RTT_TYPE,
    BK_OTA_ALI_TYPE,
    BK_OTA_ALI_AB,
    BK_OTA_FREERTOS_TYPE,
}OTA_TYPE;

typedef struct boot_param
{
    u32 magic;
    u32 upboot_addr;
    u32 os_addr;
    u32 ota_addr;
    OTA_TYPE ota_type;
    union
    {
        struct
        {
            u32 bkup_addr;
            u32 bkup_len;
            u32 ex_addr;
            u32 os_addr;
            u32 hd_addr;
            u32 crc;
            u32 status;
            u32 bk;
        }freertos_ota;
        
        struct
        {
            u32 dst_adr;
            u32 src_adr;
            u32 siz;
            u16 crc;
        }ali_ota;
    }ota;
    
}  BOOT_PARAM;

#define     UP_BOOT_ADDR     		0x1F00
#define     UART_DL_LIMIT_ADDR     ((((UP_BOOT_ADDR/32)*34)/1024) * 1024)

    /***********************************************/
#if ((CFG_SOC_NAME == SOC_BK7231N) || (CFG_SOC_NAME == SOC_BK7236))
#define     START_TYPE_ADDR        (0x00800000 + 84 * 4)
#elif (CFG_SOC_NAME == SOC_BK7271)
#define     START_TYPE_ADDR        (0x00807080)
#else
#define     START_TYPE_ADDR        (0x0080a080)
#endif
#define     CRASH_XAT0_VALUE              0xbedead00
#define     CRASH_UNDEFINED_VALUE         0xbedead01
#define     CRASH_PREFETCH_ABORT_VALUE    0xbedead02
#define     CRASH_DATA_ABORT_VALUE        0xbedead03
#define     CRASH_UNUSED_VALUE            0xbedead04

#define     CRASH_IN_BOOT_RESET1_VALUE            0xbedead10
#define     CRASH_IN_BOOT_RESET2_VALUE            0xbedead11
#define     CRASH_IN_BOOT_ABNORMAL_VALUE            0xbedead12


    extern void DelayNops(volatile unsigned long nops);

    extern void DelayUS(volatile unsigned long timesUS);
    extern void DelayMS(volatile unsigned long timesMS);

    extern void dump_hex(const unsigned char *ptr, int buflen);
    extern u32 uart_dl_port;
    extern u32 uart_download_status;
    extern u32 boot_downloading;

#define REG_READ(addr)          *((volatile u32 *)(addr))
#define REG_WRITE(addr, _data) 	(*((volatile u32 *)(addr)) = (_data))
#define CHECK_CNT_NUMBER0     13000
#define CHECK_CNT_NUMBER1     400000


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __BK_SYSTEM_H__ */
