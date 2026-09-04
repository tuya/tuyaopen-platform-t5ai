#include <modules/pm.h>

#include "tkl_mutex.h"
#include "tkl_system.h"

static uint32_t s_audio_pm_ref_cnt = 0;
static TKL_MUTEX_HANDLE s_audio_pm_mutex = NULL;

static TKL_MUTEX_HANDLE __tkl_aud_pm_get_mutex(void)
{
    TKL_MUTEX_HANDLE new_mutex = NULL;
    uint32_t irq_mask = 0;

    if (s_audio_pm_mutex != NULL) {
        return s_audio_pm_mutex;
    }

    if (tkl_mutex_create_init(&new_mutex) != OPRT_OK) {
        return NULL;
    }

    irq_mask = tkl_system_enter_critical();
    if (s_audio_pm_mutex == NULL) {
        s_audio_pm_mutex = new_mutex;
        new_mutex = NULL;
    }
    tkl_system_exit_critical(irq_mask);

    if (new_mutex != NULL) {
        tkl_mutex_release(new_mutex);
    }

    return s_audio_pm_mutex;
}

void tkl_aud_pm_acquire(void)
{
    TKL_MUTEX_HANDLE mutex = __tkl_aud_pm_get_mutex();

    if (mutex == NULL) {
        return;
    }

    if (tkl_mutex_lock(mutex) != OPRT_OK) {
        return;
    }

    if (s_audio_pm_ref_cnt == 0) {
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);
        bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 0, 0);
    }

    s_audio_pm_ref_cnt++;

    tkl_mutex_unlock(mutex);
}

void tkl_aud_pm_release(void)
{
    TKL_MUTEX_HANDLE mutex = __tkl_aud_pm_get_mutex();

    if (mutex == NULL) {
        return;
    }

    if (tkl_mutex_lock(mutex) != OPRT_OK) {
        return;
    }

    if (s_audio_pm_ref_cnt == 0) {
        tkl_mutex_unlock(mutex);
        return;
    }

    s_audio_pm_ref_cnt--;

    if (s_audio_pm_ref_cnt == 0) {
        bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AUDP, 1, 0);
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);
    }

    tkl_mutex_unlock(mutex);
}
