#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>

#define APP_TIMEOUT_VALUE    BEKEN_WAIT_FOREVER

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);


#if CONFIG_FREERTOS_SMP
static beken_semaphore_t app_semaphore;

static void cpu1_test_task(void *arg)
{
    BK_LOGD(NULL, "===cpu1_test_task===:\r\n");
    for(;;) {
        rtos_get_semaphore(&app_semaphore, BEKEN_WAIT_FOREVER);
        BK_LOGD(NULL, "cpu1_test_task run core: %d\r\n", rtos_get_core_id());
    }
}

static void cpu2_test_task(void *arg)
{
    BK_LOGD(NULL, "cpu2_test_task run core: %d\r\n", rtos_get_core_id());
  
    for(;;) {
        rtos_set_semaphore(&app_semaphore);
        BK_LOGD(NULL, "cpu2_test_task run core: %d\r\n", rtos_get_core_id());
        rtos_delay_milliseconds(1000);
    }
}

void app_test_smp_core0(void)
{
    int ret;
    beken_thread_t cpu1_thread;

    /* create a semaphore */
    ret = rtos_init_semaphore(&app_semaphore, 5);
    if (ret != kNoErr) {
        BK_LOGD(NULL, "Error: Failed to init app_semaphore: %d\r\n",ret);
    }

    /* create a thread on core 0 */
    ret = rtos_core0_create_thread(&cpu1_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "cpu1_test_task",
                             (beken_thread_function_t)cpu1_test_task,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGD(NULL, "Error: Failed to create cpu1_test_task: %d\r\n",ret);
    }
}

void app_test_smp_core1(void)
{
    int ret;
    beken_thread_t cpu2_thread;
    
    /* create a thread on core 1 */
    ret = rtos_core1_create_thread(&cpu2_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "cpu2_test_task",
                             (beken_thread_function_t)cpu2_test_task ,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGD(NULL, "Error: Failed to create cpu1_test_task: %d\r\n",ret);
    }
}
#endif

int main(void)
{
	bk_init();

#if CONFIG_FREERTOS_SMP_TEST
    app_test_smp_core0();
    app_test_smp_core1();
#endif

	return 0;
}