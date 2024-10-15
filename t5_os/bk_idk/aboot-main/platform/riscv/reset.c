/*
 * Copyright (c) 2012-2018 Andes Technology Corporation
 * All rights reserved.
 *
 */
#include"core_v5.h"

extern void c_startup(void);
extern void system_init(void);
extern void __libc_init_array(void);
extern void boot_main(void);
extern void wdt_close();
void reset_handler(void)
{
	wdt_close();
	/*
	 * Initialize LMA/VMA sections.
	 * Relocation for any sections that need to be copied from LMA to VMA.
	 */
	c_startup();

	/* Call platform specific hardware initialization */
	system_init();

	/* Do global constructors */
	__libc_init_array();

	/* Entry function */
	boot_main();
}
