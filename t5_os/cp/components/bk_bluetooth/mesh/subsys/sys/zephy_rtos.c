#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "zephyr.h"
#include <os/mem.h>
#include "zephyr/zephyr_rtos.h"
#include "zephyr/zephyr_log.h"
#include "sys/z_work.h"
#include "sys/z_poll.h"

#define USE_RECURSIVE_MUTEX 1

const k_timeout_t os_never_timeout = {
	.ticks = BEKEN_NEVER_TIMEOUT,
};

const k_timeout_t os_no_wait_timeout = {
	.ticks = BEKEN_NO_WAIT,
};

static beken_mutex_t s_rtos_mutex;


uint32_t z_tick_get(void)
{
	uint32_t t = rtos_get_time();

	return t;
}

static void beken_thread_function_zephyr( void* arg)
{
	struct k_thread *new_thread = (struct k_thread *)arg;

	if(new_thread && new_thread->delay.ticks){
		rtos_delay_milliseconds(new_thread->delay.ticks);
	}
	if(new_thread && new_thread->entry ) {
		new_thread->entry(new_thread->p1,new_thread->p2,new_thread->p3);
	}

}

k_tid_t k_thread_create(struct k_thread *new_thread,
			      k_thread_stack_t *stack,
			      size_t stack_size, k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, uint32_t options, k_timeout_t delay)
{
	if(new_thread == NULL) {
		return NULL;
	}
	irq_lock();
	new_thread->entry = entry;
	new_thread->p1 = p1;
	new_thread->p2 = p2;
	new_thread->p3 = p3;
	rtos_create_thread(&new_thread->thd,prio,"z",beken_thread_function_zephyr,stack_size,new_thread);

	irq_unlock(0);
	return new_thread;
}

void k_thread_start(k_tid_t thread)
{
	//TODO: We create thread and start it immediately
	return ;
}

int k_thread_name_set(struct k_thread *thread, const char *value)
{
	if(thread){
		thread->name = value;
	}
	return 0;
}

int z_impl_k_thread_name_set(struct k_thread *thread, const char *value)
{
	if(thread){
		thread->name = value;
	}
	return 0;
}

const char *k_thread_name_get(struct k_thread *thread)
{
	if(thread){
		return thread->name;
	}
	return NULL;
}

int k_thread_name_copy(k_tid_t thread_id, char *buf, size_t size)
{
	if(thread_id){
		strncpy(buf, thread_id->name, size);
	}else{
		if(size){
			buf[0] = '\0';
		}
	}

	return 0;
}

void** k_current_get(void)
{
	return rtos_get_current_thread();
}


void k_sleep(k_timeout_t timeout)
{
	rtos_delay_milliseconds(timeout.ticks);
}

long long k_now_ms()
{
	return rtos_get_time();
}

int k_sem_init(struct k_sem *sem, unsigned int initial_count,
               unsigned int limit)
{
	unsigned int key;

	if (NULL == sem) {
		BT_ERR("sem is NULL\n");
		return -EINVAL;
	}
	if (limit == 0U || initial_count > limit) {
		BT_ERR("limit is null or initial_count > limit");
		return -EINVAL;
	}
	key = irq_lock();
	sem->init_count = initial_count;
	sem->maxCount = limit;
	sem->pend_count = 0U;
	sem->sem = NULL;
	sys_dlist_init(&sem->poll_events);
	irq_unlock(key);
	return 0;
}

int k_sem_reset(struct k_sem *sem)
{
	BT_ERR("%s TODO: impl\n", __func__);
	return 0;
}

int k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
	unsigned int key;
	int ret = 0;

	if (NULL == sem) {
		BT_ERR("sem is NULL\n");
		return -EINVAL;
	}

	key = irq_lock();

	if (sem->init_count > 0U) {
		sem->init_count--;
		irq_unlock(key);
		return 0;
	}
	if(sem->sem == NULL){
		if (rtos_init_semaphore(&sem->sem, sem->maxCount) != kNoErr) {
			irq_unlock(key);
			BT_ERR("init sem err");
			return -EINVAL;
		}
	}

	sem->pend_count++;
	irq_unlock(key);
	ret = (rtos_get_semaphore(&sem->sem, timeout.ticks) == kNoErr) ? 0 : -1;
	key = irq_lock();
	sem->pend_count--;
	if (sem->pend_count == 0U) {
		rtos_deinit_semaphore(&sem->sem);
		sem->sem = NULL;
	}
	irq_unlock(key);

	return ret;
}

int k_sem_give(struct k_sem *sem)
{
	if (NULL == sem) {
		BT_ERR("sem is NULL\n");
		return -EINVAL;
	}
	unsigned int key;

	key = irq_lock();
	if (sem->sem) {
		rtos_set_semaphore(&sem->sem);
	} else {
		sem->init_count += (sem->init_count != sem->maxCount) ? 1 : 0;
	}
	irq_unlock(key);
	return 0;
}

unsigned int k_sem_count_get(struct k_sem *sem)
{
	return sem->init_count;
}

void k_mutex_init(struct k_mutex *mutex)
{
	int stat;

	if (NULL == mutex) {
		BT_ERR("mutex is NULL\n");
		return;
	}

	stat = rtos_init_mutex(&mutex->mutex);
	if(stat){
		BT_ERR("mutex buffer over\n");
	}
	sys_dlist_init(&mutex->poll_events);
	return;
}

int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
	unsigned int key;
	key = irq_lock();
	if(mutex->mutex == NULL){
		rtos_init_mutex(&mutex->mutex);
	}
	irq_unlock(key);
	return (rtos_lock_mutex(&mutex->mutex) == kNoErr) ? 0 : -1;
}

void k_mutex_unlock(struct k_mutex *mutex)
{
	if (NULL == mutex) {
		BT_ERR("mutex is NULL\n");
		return;
	}
	unsigned int key;
	key = irq_lock();
	if(mutex->mutex == NULL){
		rtos_init_mutex(&mutex->mutex);
	}
	irq_unlock(key);

	rtos_unlock_mutex(&mutex->mutex);
}

int k_mutex_is_uninitialized(struct k_mutex *mutex)
{
	if(mutex && mutex->mutex){
		return 0;
	}
	if(mutex){
		return 1;
	}else{
		return -1;
	}
}

int k_yield(void)
{
	return 0;
}

#undef ASSERT
#ifndef ASSERT
#define ASSERT(fmt, ...)
#endif

void k_timer_init(k_timer_t *timer, k_timer_expiry_t handle, void *args)
{
    int ret;

    ASSERT(timer, "timer is NULL");
    BT_DBG("timer %p,handle %p,args %p", timer, handle, args);
    timer->handler = handle;
    timer->timeout  = 1;
	timer->args = args;
    ret = rtos_init_timer(&timer->timer, timer->timeout,timer->handler, (void*)timer);
    if (ret != kNoErr) {
        BT_DBG("fail to create a timer");
    }
}

void k_timer_start(k_timer_t *timer, uint32_t timeout)
{
    int ret;
    ASSERT(timer, "timer is NULL");
    BT_DBG("timer %p,timeout %u", timer, timeout);

    rtos_stop_timer(&timer->timer);

    timer->timeout  = timeout;
    timer->start_ms = rtos_get_time();

    ret = rtos_change_period(&timer->timer, timer->timeout);
    if (ret != kNoErr) {
        BT_DBG("fail to change timeout");
    }

    ret = rtos_start_timer(&timer->timer);
    if (ret != kNoErr) {
        BT_DBG("fail to start timer");
    }
}

void k_timer_stop(k_timer_t *timer)
{
    int ret;
    ASSERT(timer, "timer is NULL");
/**
 * Timer may be reused, so its timeout value
 * should be cleared when stopped.
 */
    if (!timer->timeout)
        return;

    BT_DBG("timer %p", timer);
    ret = rtos_stop_timer(&timer->timer);
    if (ret) {
        BT_DBG("fail to stop timer");
    }

    timer->timeout = 0;
}

void *k_malloc(uint32_t size)
{
    return os_malloc(size);
}

void k_free(void* data)
{
    os_free(data);
}

///static struct k_mutex g_mutex;   //add mutex lock.

void fz_mutex_lock_init(void)
{
	//rtos_enter_critical();
  ///  k_mutex_init(&g_mutex);
	//rtos_exit_critical();
    BT_DBG("add mutex lock!");
}

extern long rt_hw_interrupt_disable(void);
extern void rt_hw_interrupt_enable(long level);

unsigned int irq_lock(void)
{
#if USE_RECURSIVE_MUTEX
    int32_t ret = rtos_lock_recursive_mutex(&s_rtos_mutex);
#else
    int32_t ret = rtos_lock_mutex(&s_rtos_mutex);
#endif

    if(ret)
    {
        BT_ERR("%s err %d\n", __func__, ret);
    }

    return 0;
}


void irq_unlock(unsigned int key)
{
#if USE_RECURSIVE_MUTEX
    int32_t ret = rtos_unlock_recursive_mutex(&s_rtos_mutex);
#else
    int32_t ret = rtos_unlock_mutex(&s_rtos_mutex);
#endif

    if(ret)
    {
        BT_ERR("%s err %d\n", __func__, ret);
    }
}

extern char * pcTaskGetName( void *arg);
unsigned int irq_lock_debug(const char *func)
{
    char * current_thread_name = pcTaskGetName(NULL);

    BK_LOGD(NULL,"%s %s %s\n", __func__, func, current_thread_name);
    return irq_lock();
}

void irq_unlock_debug(unsigned int key, const char *func)
{
    char * current_thread_name = pcTaskGetName(NULL);

    BK_LOGD(NULL,"%s %s %s\n", __func__, func, current_thread_name);
    return irq_unlock(key);
}

////true: A comes before B
int Z_WORK_CMP_TIME(uint32_t a, uint32_t b)
{
	if(a > b){
		if((a - b) <= (0xFFFFFFFFU >> 1)){
			return 1;
		}else{

	return 0;
		}
	}else{
		if(b >= (0xFFFFFFFFU >> 1)){
			if((b - a) >= (0xFFFFFFFFU >> 1))
				return 1;
			else
				return 0;
		}else{
			return 0;
		}
	}
}

/////result = a - b
unsigned int Z_WORK_SUB_TIME_DELT(uint32_t a, uint32_t b)
{
	if( a >= b ) {
		return (a - b);
	}else{
		if((a < (0xFFFFFFFFU >> 1)) && (b > (0xFFFFFFFFU >> 1))){
			return (0xFFFFFFFFU - b) + a;
		}
	}

	return 0;
}

uint32_t z_timeout_end_calc(k_timeout_t timeout)
{
	k_timeout_t dt;

	if (timeout.ticks == BEKEN_NEVER_TIMEOUT /*K_FOREVER*/) {
		return BEKEN_NEVER_TIMEOUT;
	} else if (timeout.ticks == BEKEN_NO_WAIT /*K_NO_WAIT*/) {
		return k_now_ms();
	}

	dt = timeout;
#if CONFIG_TIMEOUT_64BIT
	if (IS_ENABLED(CONFIG_TIMEOUT_64BIT) && Z_TICK_ABS(dt) >= 0) {
		return Z_TICK_ABS(dt);
	}
#endif
	return k_now_ms() + MAX(1, dt.ticks);
}

void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
			k_timeout_t timeout)
{
	uint32_t now, end = k_now_ms() + timeout.ticks;
	void *ret = NULL;
	unsigned int key;

	while (ret == NULL) {
		key = irq_lock();
		ret = k_malloc(bytes);

		now = k_now_ms();
		int delt = Z_WORK_SUB_TIME_DELT(end, now);
		irq_unlock(key);
		if ((ret != NULL) || (delt <= 0)) {
			break;
		}
		if(delt > 10){
			k_sleep(K_MSEC(10));
		}else if(delt > 0){
			k_sleep(K_MSEC(delt));
		}else{
			break;
		}
	}

	return ret;
}

void k_heap_free(struct k_heap *h, void *mem)
{
	unsigned int key;

	key = irq_lock();
	k_free(mem);
	irq_unlock(key);
}

int char2hex(char c, uint8_t *x)
{
	if (c >= '0' && c <= '9') {
		*x = c - '0';
	} else if (c >= 'a' && c <= 'f') {
		*x = c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		*x = c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

int hex2char(uint8_t x, char *c)
{
	if (x <= 9) {
		*c = x + '0';
	} else  if (x <= 15) {
		*c = x - 10 + 'a';
	} else {
		return -EINVAL;
	}

	return 0;
}

size_t bin2hex(const uint8_t *buf, size_t buflen, char *hex, size_t hexlen)
{
	if ((hexlen + 1) < buflen * 2) {
		return 0;
	}

	for (size_t i = 0; i < buflen; i++) {
		if (hex2char(buf[i] >> 4, &hex[2 * i]) < 0) {
			return 0;
		}
		if (hex2char(buf[i] & 0xf, &hex[2 * i + 1]) < 0) {
			return 0;
		}
	}

	hex[2 * buflen] = '\0';
	return 2 * buflen;
}

size_t hex2bin(const char *hex, size_t hexlen, uint8_t *buf, size_t buflen)
{
	uint8_t dec;

	if (buflen < hexlen / 2 + hexlen % 2) {
		return 0;
	}

	/* if hexlen is uneven, insert leading zero nibble */
	if (hexlen % 2) {
		if (char2hex(hex[0], &dec) < 0) {
			return 0;
		}
		buf[0] = dec;
		hex++;
		buf++;
	}

	/* regular hex conversion */
	for (size_t i = 0; i < hexlen / 2; i++) {
		if (char2hex(hex[2 * i], &dec) < 0) {
			return 0;
		}
		buf[i] = dec << 4;

		if (char2hex(hex[2 * i + 1], &dec) < 0) {
			return 0;
		}
		buf[i] += dec;
	}

	return hexlen / 2 + hexlen % 2;
}

uint8_t uint8_to_dec(char *buf, uint8_t buflen, uint8_t value)
{
       uint8_t divisor = 100;
       uint8_t num_digits = 0;
       uint8_t digit;

       while (buflen > 0 && divisor > 0) {
               digit = value / divisor;
               if (digit != 0 || divisor == 1 || num_digits != 0) {
                       *buf = (char)digit + '0';
                       buf++;
                       buflen--;
                       num_digits++;
               }

               value -= digit * divisor;
               divisor /= 10;
       }

       if (buflen) {
               *buf = '\0';
       }

       return num_digits;
}

void freertos_zephyr_port_init(void)
{
    BT_INFO("%s", __func__);

#if USE_RECURSIVE_MUTEX
    if(rtos_init_recursive_mutex(&s_rtos_mutex))
#else
    if(rtos_init_mutex(&s_rtos_mutex))
#endif
    {
        BT_ERR("%s init rtos mutex err", __func__);
    }

	k_sys_work_q_init(NULL);
	fz_mutex_lock_init();
}

static struct k_mutex sched_mutex;

void k_sched_lock(void)
{
    BT_ERR("k_sched_lock");
	if(sched_mutex.mutex == NULL){
		rtos_init_mutex(&sched_mutex.mutex);
	}
	rtos_lock_mutex(&sched_mutex.mutex);

}

void k_sched_unlock(void)
{
    BT_ERR("k_sched_unlock");
	if(sched_mutex.mutex == NULL){
		rtos_init_mutex(&sched_mutex.mutex);
	}
	rtos_unlock_mutex(&sched_mutex.mutex);
}


/////////////////////////// Test demo ///////////////////////////////////////
#if (defined(Z_SEM_TEST_CFG) && Z_SEM_TEST_CFG) \
	|| (defined(Z_MUTEX_TEST_CFG) && Z_MUTEX_TEST_CFG) \
	|| (defined(Z_RTOS_TEST_CFG) && Z_RTOS_TEST_CFG)
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#define Z_MUTEX_TEST_NUM  2
static struct k_mutex _k_mut = { 0 };
static struct k_sem _ksem = {0};
static struct k_thread __zthread[2] = {0};

int k_sem_is_uninitialized(struct k_sem *sem)
{
	if(sem && sem->sem) {
		return 0;
	}
	if(sem){
		return 1;
	}else{
		return -1;
	}
}

static void m_thread_function( void* arg)
{
	struct k_mutex *km = (struct k_mutex *)arg;
	k_timeout_t timeout;
	void* *thd =  rtos_get_current_thread();
	int ret;

	BT_INFO("%s(thd:%p) run\r\n",__FUNCTION__,thd);
	timeout.ticks = 5 * 1000;
	while(1) {
		ret = k_mutex_lock(km,timeout);
		if(ret == 0){
			BT_INFO("%s(thd:%p) k_mutex_lock succeed\r\n",__FUNCTION__,thd);
			k_sleep(K_MSEC(1000));
			k_mutex_unlock(km);
			k_sleep(K_MSEC(10));
		}else{
			BT_INFO("%s(thd:%p) k_mutex_lock faild\r\n",__FUNCTION__,thd);
		}
	}
	rtos_delete_thread(NULL);
}

static void ksem_thread_function( void* arg)
{
	struct k_sem *km = (struct k_sem *)arg;
	k_timeout_t timeout;
	void* *thd =  rtos_get_current_thread();
	int ret;

	BT_INFO("%s(thd:%p) run\r\n",__FUNCTION__,thd);
	timeout.ticks = 5 * 1000;
	while(1) {
		ret = k_sem_take(km,timeout);
		if(ret == 0) {
			BT_INFO("%s(thd:%p) k_sem_take succeed\r\n",__FUNCTION__,thd);
		}else{
			BT_INFO("%s(thd:%p) k_sem_take faild\r\n",__FUNCTION__,thd);
		}
	}
	rtos_delete_thread(NULL);
}

static void zk_thread_entry(void *p1, void *p2, void *p3)
{
	struct k_thread *kthd = (struct k_thread *)p1;

	BT_INFO("%s(thd:%p) run\r\n",__FUNCTION__,kthd);
	while(1) {
		k_sleep(K_MSEC(2000));
		BT_INFO("%s(thd:%p)\r\n",__FUNCTION__,kthd);
	}
}

void z_rtos_test_command(int argc, char **argv)
{
	if ( strcmp(argv[1], "mutex_init") == 0 ) {
		if(k_mutex_is_uninitialized( &_k_mut ) != 0) {
			k_mutex_init( &_k_mut );
		}
		void* qthread = {0};
		rtos_create_thread( &qthread,
							7,
							"zmt",
							m_thread_function,
							1024,
							&_k_mut);
	}else if ( strcmp(argv[1], "sem_init") == 0 ) {
		if(k_sem_is_uninitialized( &_ksem ) != 0) {
			k_sem_init( &_ksem, 1, 10);
		}
		void* qthread = {0};
		rtos_create_thread( &qthread,
							7,
							"kst",
							ksem_thread_function,
							1024,
							&_ksem);
	}else if ( strcmp(argv[1], "sem_set") == 0 ) {
		if(k_sem_is_uninitialized( &_ksem ) == 0) {
			k_sem_give( &_ksem );
			BT_INFO("k_sem_give --> _ksem\r\n");
		}else{
			BT_INFO("sem uninit\r\n");
		}
	}else if ( strcmp(argv[1], "rtos") == 0 ) {
		k_thread_create(&__zthread[0],
					(k_thread_stack_t*)NULL,
					1024 * 1,
					zk_thread_entry,
					&__zthread[0],NULL,NULL,
					8,
					0,
					K_NO_WAIT
				);
	}else if ( strcmp(argv[1], "rtos_delay") == 0 ) {
		k_thread_create(&__zthread[1],
					(k_thread_stack_t*)NULL,
					1024 * 1,
					zk_thread_entry,
					&__zthread[1],NULL,NULL,
					8,
					0,
					((k_timeout_t) {1000})
				);
	}
}



#endif

///Eof

