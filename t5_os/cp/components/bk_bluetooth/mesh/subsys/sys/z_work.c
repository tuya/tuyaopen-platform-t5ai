#include "zephyr.h"
#include "sys/z_work.h"
#include "sys/__assert.h"

#define KER_QUEUE_FOREVER               K_FOREVER

#ifndef ARG_UNUSED
#define ARG_UNUSED(x) (void)(x)
#endif

extern struct k_work_q k_sys_work_q;

#define WORKQUEUE_THREAD_NAME	"mesh_wq_u"
extern void zephyr_work_q_thread_function( void* arg);
void k_work_q_start(struct k_work_q *work_q, k_thread_stack_t *stack,
		    size_t stack_size, int prio)
{
    irq_lock();
	k_queue_init(&work_q->queue);

#if 0
	(void)k_thread_create( &work_q->thread,
							stack,
							stack_size,
							z_work_q_main,
							work_q,NULL, NULL,
							prio,
							0,
							K_NO_WAIT);


	k_thread_name_set(&work_q->thread, WORKQUEUE_THREAD_NAME);
#else
	rtos_create_thread(&work_q->thread,
				prio,///BEKEN_APPLICATION_PRIORITY,
				WORKQUEUE_THREAD_NAME,
				zephyr_work_q_thread_function,
				(stack_size > 512)?stack_size:512,
				(void*)work_q);
#endif

	irq_unlock(0);
}


static void k_work_submit_to_queue(struct k_work_q *work_q, struct k_work *work)
{
	sys_snode_t *node = NULL;
	struct k_work *delayed_work = NULL;
	struct k_work *prev_delayed_work = NULL;

	if (!atomic_test_and_set_bit(work->flags, K_WORK_STATE_PENDING)) {
		SYS_SLIST_FOR_EACH_NODE(&work_q->queue.data_q, node) {
			delayed_work = (struct k_work *)node;
			if ((work->timeout + work->start_ms) < (delayed_work->start_ms + delayed_work->timeout))
				break;
			prev_delayed_work = delayed_work;
		}

		sys_slist_insert(&work_q->queue.data_q,
		(sys_snode_t *)prev_delayed_work, (sys_snode_t *)work);

		rtos_set_semaphore(&work_q->queue.sem);
	} else {
		BT_INFO("%s work_q: %p atomic_test_and_set_bit failed\r\n",__FUNCTION__,work_q);
	}
}

void k_work_rm_from_queue(struct k_work_q *work_q, struct k_work *work)
{
	k_queue_remove(&work_q->queue, work);
}

int k_work_init(struct k_work *work, k_work_handler_t handler)
{
	atomic_clear_bit(work->flags, K_WORK_STATE_PENDING);
	work->handler = handler;
	work->start_ms = 0;
	work->timeout = 0;
	return 0;
}

void k_work_submit(struct k_work *work)
{
	k_delayed_work_submit((struct k_delayed_work *)work, (k_timeout_t){0});
}

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler)
{
	k_work_init(&work->work, handler);
}
void k_work_init_delayable(struct k_work_delayable *dwork, k_work_handler_t handler)
{
	k_work_init(&dwork->work, handler);
}

int k_delayed_work_submit(struct k_delayed_work *work, k_timeout_t delay)
{
	int err = 0;
	unsigned int key = irq_lock();

	if (atomic_test_bit(work->work.flags, K_WORK_STATE_PENDING))
		k_delayed_work_cancel(work);

	work->work.start_ms = k_uptime_get_32();
	work->work.timeout = delay.ticks;
	k_work_submit_to_queue(&k_sys_work_q, (struct k_work *)work);

	irq_unlock(key);
	return err;
}

int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_delayed_work_submit(dwork, delay);
}

int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_delayed_work_submit(dwork, delay);
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
	unsigned int key = irq_lock();
	work->work.timeout = 0;
	atomic_clear_bit(work->work.flags, K_WORK_STATE_PENDING);
	k_work_rm_from_queue(&k_sys_work_q, (struct k_work *)work);
	sys_slist_find_and_remove(&k_sys_work_q.kw_data_q, (sys_snode_t *)work);
	rtos_set_semaphore(&k_sys_work_q.queue.sem);
	irq_unlock(key);

	return 0;
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
	return k_delayed_work_cancel(dwork);
}

bool k_delayed_work_pending(struct k_delayed_work *work)
{
	return (k_delayed_work_remaining_get(work) > 0) ||
		k_work_pending(&work->work);
}

bool k_work_delayable_is_pending(struct k_work_delayable *work)
{
	return k_delayed_work_pending(work);
}

int k_delayed_work_remaining_get(struct k_delayed_work *work)
{
	int remain;

	if (work == NULL)
		return 0;

	remain = work->work.timeout - (k_uptime_get_32() - work->work.start_ms);
	if (remain < 0)
		remain = 0;

	return remain;
}

uint32_t k_work_delayable_remaining_get(const struct k_work_delayable *dwork)
{
	return k_delayed_work_remaining_get((struct k_work_delayable *)dwork);
}

/////////////////////////// Test demo ///////////////////////////////////////
#if defined(Z_WORK_TEST_CFG) && Z_WORK_TEST_CFG
#include "string.h"
#include "stdlib.h"

static struct k_delayed_work _kd_work[8] = {0};
static struct k_work _kwork = {0};

static void _k_work_handler(struct k_work *work)
{
	BT_INFO("---->k_work(%p) handler,os tick:%d\r\n",work,k_uptime_get_32());
	if(work->timeout) {
		BT_INFO("---->k_work(%p) handler,start tick:%d\r\n",work,work->start_ms);
		BT_INFO("---->k_work(%p) handler,timeout:%d\r\n",work,work->timeout);
	}
}

void z_work_test_command(int argc, char **argv)
{
	if ( strcmp(argv[1], "kwork_init") == 0 ) {
		if(_kwork.handler == NULL){
			_kwork.handler = &_k_work_handler;
		}
		BT_INFO("k_work submit\r\n");
		k_work_submit(&_kwork);
	}else if ( strcmp(argv[1], "kdwork_init") == 0 ) {
		if(_kd_work[0].work.handler == NULL){
			_kd_work[0].work.handler = &_k_work_handler;
			_kd_work[1].work.handler = &_k_work_handler;
			_kd_work[2].work.handler = &_k_work_handler;
			_kd_work[3].work.handler = &_k_work_handler;
			_kd_work[4].work.handler = &_k_work_handler;
			_kd_work[5].work.handler = &_k_work_handler;
			_kd_work[6].work.handler = &_k_work_handler;
			_kd_work[7].work.handler = &_k_work_handler;
		}
		BT_INFO("k_delayed_work submit\r\n");
		k_delayed_work_submit(&_kd_work[5], (k_timeout_t){9000});
		k_delayed_work_submit(&_kd_work[0], (k_timeout_t){5000});
		k_delayed_work_submit(&_kd_work[1], (k_timeout_t){4000});
		k_delayed_work_submit(&_kd_work[3], (k_timeout_t){3000});
		k_delayed_work_submit(&_kd_work[2], (k_timeout_t){6000});
		k_delayed_work_submit(&_kd_work[4], (k_timeout_t){7000});
		k_delayed_work_submit(&_kd_work[6], (k_timeout_t){7000});
		k_delayed_work_submit(&_kd_work[7], (k_timeout_t){2000});
	}else if ( strcmp(argv[1], "kdwork_cancel") == 0 ) {
		BT_INFO("kdwork cancel %p\r\n",&_kd_work[2]);
		k_delayed_work_cancel(&_kd_work[2]);
	}
}

#endif

///Eof

