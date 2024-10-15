#ifndef _Z_WORK_H_
#define _Z_WORK_H_

//#include <os/os.h>
#include "atomic.h"
#include "z_queue.h"

struct k_work;
typedef void (*k_work_handler_t)(struct k_work *work);


enum {
	K_WORK_STATE_PENDING,	/* Work item pending state */
};

struct k_work {
	void *_reserved;		/* Used by k_queue implementation. */
	k_work_handler_t handler;
	atomic_t flags[1];
	uint32_t start_ms;
	uint32_t timeout;
};

struct k_work_q {
	struct k_queue queue;
	sys_slist_t kw_data_q;
	void* thread;
};

/*delay work define*/
struct k_delayed_work {
    struct k_work work;
};


#define k_work_delayable k_delayed_work


struct z_work_flusher {
	struct k_work work;
	struct k_sem sem;
};

struct z_work_canceller {
	sys_snode_t node;
	struct k_work *work;
	struct k_sem sem;
};

struct k_work_sync {
	union {
		struct z_work_flusher flusher;
		struct z_work_canceller canceller;
	};
};

/**
 * INTERNAL_HIDDEN @endcond
 */

#define Z_WORK_INITIALIZER(work_handler) \
	{ \
	._reserved = NULL, \
	.handler = work_handler, \
	.flags = { 0 }, \
	.start_ms = 0,  \
	.timeout = 0,   \
	}

static inline bool k_work_pending(struct k_work *work)
{
	return atomic_test_bit(work->flags, K_WORK_STATE_PENDING);
}


extern void k_work_q_start(struct k_work_q *work_q, k_thread_stack_t *stack,
		    size_t stack_size, int prio);
extern int k_work_init(struct k_work *work, k_work_handler_t handler);
extern void k_work_submit(struct k_work *work);
extern void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler);
extern int k_delayed_work_submit(struct k_delayed_work *work, k_timeout_t delay);

extern int k_delayed_work_cancel(struct k_delayed_work *work);
extern bool k_delayed_work_pending(struct k_delayed_work *work);
extern int k_delayed_work_remaining_get(struct k_delayed_work *work);
extern int k_sys_work_q_init(const void *dev);


int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay);
int k_work_cancel_delayable(struct k_work_delayable *dwork);
int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay);
void k_work_init_delayable(struct k_work_delayable *dwork, k_work_handler_t handler);
uint32_t k_work_delayable_remaining_get(const struct k_work_delayable *dwork);
bool k_work_delayable_is_pending(struct k_work_delayable *work);

static inline struct k_work_delayable *k_work_delayable_from_work(struct k_work *work)
{
	return CONTAINER_OF(work, struct k_work_delayable, work);
}

static inline uint32_t k_ticks_to_ms_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	//return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ms, true, true, false, false);
	return t;
}

#endif

