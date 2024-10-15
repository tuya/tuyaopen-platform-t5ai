#include <components/log.h>
#include <modules/wifi.h>
#include <components/netif.h>
#include <components/event.h>
#include <string.h>
#include "bk_private/bk_init.h"

extern int wifi_cli_iperf_init(void);

int main(void)
{
	bk_init();
	//wifi_cli_iperf_init();
	return 0;
}

