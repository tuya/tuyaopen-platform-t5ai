#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <common/bk_assert.h>

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

	if (os_strcmp(string, "nt35512") == 0)
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

