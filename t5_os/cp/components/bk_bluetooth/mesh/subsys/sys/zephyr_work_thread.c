#include "sys/z_work.h"
#include "zephyr/zephyr_rtos.h"
#include "sys/__assert.h"
#include "zephyr.h"

#define KER_QUEUE_FOREVER               K_FOREVER

#ifndef ARG_UNUSED
#define ARG_UNUSED(x) (void)(x)
#endif


struct k_work_q k_sys_work_q = {
	0
};

#define WORKQUEUE_THREAD_NAME	"mesh_wq"


static void z_work_q_main(void *work_q_ptr, void *p2, void *p3)
{
	struct k_work_q *work_q = (struct k_work_q *)work_q_ptr;
	k_timeout_t timeout_ms;
	sys_slist_t *kw_data_q = &work_q->kw_data_q;
	unsigned int key;

	sys_slist_init(kw_data_q);
	timeout_ms.ticks = BEKEN_NEVER_TIMEOUT;
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	BT_INFO("%s work_q: %p\r\n", __FUNCTION__, work_q);

	while (1) {
		struct k_work *work;
		k_work_handler_t handler;

		work = k_queue_get(&work_q->queue, timeout_ms);
		do{
			key = irq_lock();
			if (( work == NULL ) && (sys_slist_is_empty(kw_data_q))){
				timeout_ms.ticks = BEKEN_NEVER_TIMEOUT;
				irq_unlock(key);
				break;
			}
			if( work == NULL ) {
				work = (struct k_work *)sys_slist_get(kw_data_q);
				if ( work == NULL ) {
					timeout_ms.ticks = BEKEN_NEVER_TIMEOUT;
					irq_unlock(key);
					break;
				}
			}

			uint32_t now = k_uptime_get_32();
			if ( work->timeout && !Z_WORK_CMP_TIME(now,work->timeout + work->start_ms) ) {
				sys_snode_t *node = NULL;
				struct k_work *delayed_work = NULL;
				struct k_work *prev_delayed_work = NULL;
				SYS_SLIST_FOR_EACH_NODE(kw_data_q, node) {
		            delayed_work = (struct k_work *)node;
		            if ((work->timeout + work->start_ms) < (delayed_work->start_ms + delayed_work->timeout)) {
		                break;
		            }
		            prev_delayed_work = delayed_work;
		        }
				sys_slist_insert(kw_data_q,(sys_snode_t *)prev_delayed_work, (sys_snode_t *)work);
			#if 0
				SYS_SLIST_FOR_EACH_NODE(&kw_data_q, node) {
					delayed_work = (struct k_work *)node;
					BT_INFO("list show:%d mS\r\n",delayed_work->timeout + delayed_work->start_ms);
				}
			#endif
				delayed_work = (struct k_work *)sys_slist_peek_head( kw_data_q );
				if(delayed_work){
					now = k_uptime_get_32();
					timeout_ms.ticks = Z_WORK_SUB_TIME_DELT((delayed_work->timeout + delayed_work->start_ms),now);
				}else{
					timeout_ms.ticks = 1;
				}
				irq_unlock(key);
				break;
			}
			handler = work->handler;
			__ASSERT(handler != NULL, "handler must be provided, work point %p handler point %p", &work, &work->handler);
			irq_unlock(key);

			/* Reset pending state so it can be resubmitted by handler */
			if (atomic_test_and_clear_bit(work->flags, K_WORK_STATE_PENDING)) {
				if ( handler ) {
					handler(work);
				}
			}

			/* Make sure we don't hog up the CPU if the FIFO never (or
			 * very rarely) gets empty.
			 */
			k_yield();

			key = irq_lock();
			struct k_work * next_work = (struct k_work *)sys_slist_peek_head( kw_data_q );
			if(next_work){
				now = k_uptime_get_32();
				timeout_ms.ticks = Z_WORK_SUB_TIME_DELT((next_work->timeout + next_work->start_ms),now);
			}else{
				timeout_ms.ticks = BEKEN_NEVER_TIMEOUT;
			}
			irq_unlock(key);
		}while(0);
	}
	rtos_delete_thread(NULL);
}


void zephyr_work_q_thread_function( void* arg)
{
	z_work_q_main((void*)arg,NULL,NULL);
}

int k_sys_work_q_init(const void *dev)
{
	ARG_UNUSED(dev);

	BT_INFO("");
	irq_lock();
	if ( k_sys_work_q.thread == NULL ) {
		rtos_create_thread(&k_sys_work_q.thread,
			BEKEN_APPLICATION_PRIORITY,
			WORKQUEUE_THREAD_NAME,
			zephyr_work_q_thread_function,
			3 * 1024,
			&k_sys_work_q);
	}
	irq_unlock(0);

	return 0;
}


