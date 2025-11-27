#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <common/bk_assert.h>
#include <driver/aon_rtc.h>

#include "media_utils.h"

media_rotate_t get_string_to_angle(char *string)
{
	media_rotate_t value = ROTATE_NONE;

	if (os_strcmp(string, "90") == 0)
		value = ROTATE_90;
	else if (os_strcmp(string, "270") == 0)
		value = ROTATE_270;
	else if (os_strcmp(string, "0") == 0)
		value = ROTATE_NONE;
	else if (os_strcmp(string, "180") == 0)
		value = ROTATE_180;
	else
		value = ROTATE_90;

	return value;
}

char * get_string_to_lcd_name(char *string)
{
	char* value = NULL;

	if (os_strcmp(string, "st7789t3") == 0)
	{
		value = "st7789t3";
	}
    else if (os_strcmp(string, "nt35512") == 0)
	{
		value = "nt35512";
	}
	else if (os_strcmp(string, "gc9503v") == 0)
	{
		value = "gc9503v";
	}
	else if (os_strcmp(string, "st7282") == 0)
	{
		value = "st7282";
	}
	else if (os_strcmp(string, "st7796s") == 0)
	{
		value = "st7796s";
	}
	else if (os_strcmp(string, "hx8282") == 0)
	{
		value = "hx8282";
	}
	else if (os_strcmp(string, "nt35510") == 0)
	{
		value = "nt35510";
	}
	else if (os_strcmp(string, "nt35510_mcu") == 0)
	{
		value = "nt35510_mcu";
	}
	else if (os_strcmp(string, "h050iwv") == 0)
	{
		value = "h050iwv";
	}
	else if (os_strcmp(string, "md0430r") == 0)
	{
		value = "md0430r";
	}
	else if (os_strcmp(string, "md0700r") == 0)
	{
		value = "md0700r";
	}
	else if (os_strcmp(string, "st7701s_ly") == 0)
	{
		value = "st7701s_ly";
	}
	else if (os_strcmp(string, "st7701sn") == 0)
	{
		value = "st7701sn";
	}
	else if (os_strcmp(string, "st7701s") == 0)
	{
		value = "st7701s";
	}
	else if (os_strcmp(string, "st7789v") == 0)
	{
		value = "st7789v";
	}
	else if (os_strcmp(string, "aml01") == 0)
	{
		value = "aml01";
	}
	else if (os_strcmp(string, "st77903_h0165y008t") == 0)
	{
		value = "st77903_h0165y008t";
	}
	else if (os_strcmp(string, "spd2010") == 0)
	{
		value = "spd2010";
	}

	return value;
}

media_ppi_t get_string_to_ppi(char *string)
{
	uint32_t value = PPI_DEFAULT;

	if (strcmp(string, "7680X4320") == 0)
	{
		value = PPI_7680X4320;
	}

	if (strcmp(string, "2304X1296") == 0)
	{
		value = PPI_2304X1296;
	}

	if (strcmp(string, "1920X1080") == 0)
	{
		value = PPI_1920X1080;
	}

	if (strcmp(string, "1600X1200") == 0)
	{
		value = PPI_1600X1200;
	}

	if (strcmp(string, "1280X720") == 0)
	{
		value = PPI_1280X720;
	}

	if (strcmp(string, "720X1280") == 0)
	{
		value = PPI_720X1280;
	}

	if (strcmp(string, "1024X600") == 0)
	{
		value = PPI_1024X600;
	}

	if (strcmp(string, "640X480") == 0)
	{
		value = PPI_640X480;
	}

	if (strcmp(string, "480X320") == 0)
	{
		value = PPI_480X320;
	}

	if (strcmp(string, "480X272") == 0)
	{
		value = PPI_480X272;
	}

	if (strcmp(string, "320X480") == 0)
	{
		value = PPI_320X480;
	}

	if (strcmp(string, "320X240") == 0)
	{
		value = PPI_320X240;
	}

	if (strcmp(string, "480X800") == 0)
	{
		value = PPI_480X800;
	}

	if (strcmp(string, "800X480") == 0)
	{
		value = PPI_800X480;
	}

	if (strcmp(string, "480X854") == 0)
	{
		value = PPI_480X854;
	}
	if (strcmp(string, "480X864") == 0)
	{
		value = PPI_480X864;
	}
	if (strcmp(string, "800X600") == 0)
	{
		value = PPI_800X600;
	}

	if (strcmp(string, "864X480") == 0)
	{
		value = PPI_864X480;
	}

	if (strcmp(string, "854X480") == 0)
	{
		value = PPI_854X480;
	}

	if (strcmp(string, "480X480") == 0)
	{
		value = PPI_480X480;
	}

	if (strcmp(string, "400X400") == 0)
	{
		value = PPI_400X400;
	}

	if (strcmp(string, "412X412") == 0)
	{
		value = PPI_412X412;
	}

	if (strcmp(string, "170X320") == 0)
	{
		value = PPI_170X320;
	}

	if (strcmp(string, "960X480") == 0)
	{
		value = PPI_960X480;
	}

	return value;
}

uint32_t get_current_timestamp(void)
{
	uint64_t timer = 0;

#ifdef CONFIG_ARCH_RISCV
	timer = (riscv_get_mtimer() / 26) & 0xFFFFFFFF;// tick
#else // CONFIG_ARCH_RISCV

#ifdef CONFIG_AON_RTC
	timer = bk_aon_rtc_get_us() & 0xFFFFFFFF;
#endif

#endif // CONFIG_ARCH_RISCV

	return (uint32_t)timer;
}

/**
 * @brief get camera support ppi compare with user set
 * @{
 */
media_ppi_cap_t pixel_ppi_to_cap(media_ppi_t ppi)
{
	media_ppi_cap_t cap = PPI_CAP_UNKNOW;

	switch (ppi)
	{
		case PPI_170X320:
			cap = PPI_CAP_170X320;
			break;

		case PPI_320X240:
			cap = PPI_CAP_320X240;
			break;

		case PPI_320X480:
			cap = PPI_CAP_320X480;
			break;

		case PPI_480X272:
			cap = PPI_CAP_480X272;
			break;

		case PPI_480X320:
			cap = PPI_CAP_480X320;
			break;

		case PPI_480X480:
			cap = PPI_CAP_480X480;
			break;

		case PPI_480X800:
			cap = PPI_CAP_480X800;
			break;

		case PPI_640X480:
			cap = PPI_CAP_640X480;
			break;

		case PPI_800X480:
			cap = PPI_CAP_800X480;
			break;

		case PPI_800X600:
			cap = PPI_CAP_800X600;
			break;

		case PPI_864X480:
			cap = PPI_CAP_864X480;
			break;

		case PPI_1024X600:
			cap = PPI_CAP_1024X600;
			break;

		case PPI_1280X720:
			cap = PPI_CAP_1280X720;
			break;

		case PPI_1600X1200:
			cap = PPI_CAP_1600X1200;
			break;

		case PPI_DEFAULT:
		default:
			break;
	}

	return cap;
}

/**
 * @brief init sei nalu delf define, the length equal to 96 bytes
 * @{
 */

void h264_encode_sei_init(uint8_t *sei)
{
	sei[0] = 0x00;
	sei[1] = 0x00;
	sei[2] = 0x00;
	sei[3] = 0x01;
	sei[4] = 0x06;
	sei[5] = 0x05;
	sei[6] = 0x59;//96 - 7 = 89 (16 bytes uuid + 73 bytes pyload)

	for (int i = 0; i < 16; i++)
	{
		// add uuid
		sei[7 + i] = 0xAF;
	}

	// offset = 7 + 16 = 23

	sei[95] = 0x80; // rbsp trailing bits
}

void *media_malloc(uint32_t size)
{
#if (CONFIG_PSRAM_MEDIA_MALLOC_ENABLE)
    void *data = NULL;
    data = psram_malloc(size);
    if (data == NULL)
    {
        return os_malloc(size);
    }
    else
    {
        return data;
    }
#else
    return os_malloc(size);
#endif
}

