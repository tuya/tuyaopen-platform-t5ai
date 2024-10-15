#ifndef _ZEPHYR_LOG_H_
#define _ZEPHYR_LOG_H_

#if CONFIG_BLE_HOST_ETHERMIND
#include "ethal_debug_api.h"
#elif CONFIG_BLE_HOST_RW
#include "common_bt.h"
#endif

#include <assert.h>

//extern void BLE_LOGI(const char *fmt, ...);


#define BT_HOST_VER(format, ...) BK_LOGV("mesh:", "%s:" format "\r\n", __func__, ##__VA_ARGS__)
#define BT_HOST_DBG(format, ...) BK_LOGD("mesh:", "%s:" format "\r\n", __func__, ##__VA_ARGS__)
#define BT_HOST_INF(format, ...) BK_LOGI("mesh:", "%s:" format "\r\n", __func__, ##__VA_ARGS__)
#define BT_HOST_WRN(format, ...) BK_LOGW("mesh:", "%s:" format "\r\n", __func__, ##__VA_ARGS__)
#define BT_HOST_ERR(format, ...) BK_LOGE("mesh:", "%s:" format "\r\n", __func__, ##__VA_ARGS__)


#if ZP_LOG_DBG

#if 1

#define LOG_DBG(format, ...) BT_HOST_INF(format, ##__VA_ARGS__)
#define BT_HEXDUMP_DBG(_data, _length, _str)                                    \
	do {                                                                    \
		BK_LOG_RAW("[D]%s:%d %s:", __FUNCTION__, __LINE__, _str);          \
		for (int i = 0; i < _length; i++) {                             \
			BK_LOG_RAW("%02x ", ((const char *)_data)[i] & 0xFF);  \
		}                                                               \
		BK_LOG_RAW("\r\n");                                              \
	} while (0)
#endif

#else
#define LOG_DBG(...) (void)(__VA_ARGS__)
#define BT_HEXDUMP_DBG(_data, _length, _str)
#endif

#if ZP_LOG_ERR
//#define LOG_ERR(...)     bk_printf( __VA_ARGS__)
#define LOG_ERR(format, ...) BT_HOST_ERR(format, ##__VA_ARGS__)
#else
#define LOG_ERR(...) (void)(__VA_ARGS__)
#endif


#if ZP_LOG_WRN
//#define LOG_WRN(...)     bk_printf( __VA_ARGS__)
#define LOG_WRN(format, ...) BT_HOST_WRN(format, ##__VA_ARGS__)
#else
#define LOG_WRN(...) (void)(__VA_ARGS__)
#endif


#if ZP_LOG_INF
//#define LOG_INF(...)     bk_printf( __VA_ARGS__)
#define LOG_INF(format, ...) BT_HOST_INF(format, ##__VA_ARGS__)
#else
#define LOG_INF(...) (void)(__VA_ARGS__)
#endif


#if ZP_LOG_VER
//#define LOG_INF(...)     bk_printf( __VA_ARGS__)
#define LOG_VER(format, ...) BT_HOST_VER(format, ##__VA_ARGS__)
#else
#define LOG_VER(...) (void)(__VA_ARGS__)
#endif

#define BT_ERR LOG_ERR
#define BT_WARN LOG_WRN
#define BT_INFO LOG_INF
#define BT_DBG LOG_DBG
#define BT_VER LOG_VER

#define BT_ASSERT_PRINT(test) __ASSERT_LOC(test)
#define BT_ASSERT_PRINT_MSG(fmt, ...) __ASSERT_MSG_INFO(fmt, ##__VA_ARGS__)

#define BT_ASSERT_DIE()

#if 0
#define BT_ASSERT(cond)                          \
	do {                                     \
		if (!(cond)) {                   \
			BT_ASSERT_PRINT(cond);   \
			BT_ASSERT_DIE();         \
		}                                \
	} while (0)

#define BT_ASSERT_MSG(cond, fmt, ...)                              \
	do {                                                       \
		if (!(cond)) {                                     \
			BT_ASSERT_PRINT(cond);                     \
			BT_ASSERT_PRINT_MSG(fmt, ##__VA_ARGS__);   \
			BT_ASSERT_DIE();                           \
		}                                                  \
	} while (0)
#endif

#define BT_ASSERT(cond)\
	do\
	{\
		if(!(cond))\
		{\
			LOG_ERR("assert !!!");\
			assert(cond);\
		}\
	}\
	while(0)


#define BT_ASSERT_MSG(cond, fmt, ...)\
	do\
	{\
		if(!(cond))\
		{\
			LOG_ERR("assert !!! " fmt, ##__VA_ARGS__);\
			assert(cond);\
		}\
	}\
	while(0)
#endif

