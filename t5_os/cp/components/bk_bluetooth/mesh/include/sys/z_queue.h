#ifndef _Z_QUEUE_H_
#define _Z_QUEUE_H_

#include "zephyr/zp_config.h"
#include "zephyr/zephyr_rtos.h"

struct k_queue {
	sys_slist_t data_q;
	sys_dlist_t poll_events;

	///beken_semaphore_t lock;
	beken_semaphore_t sem;
};

struct alloc_node {
	sys_snode_t node;
	void *data;
};

extern void k_queue_init(struct k_queue *queue);;
extern void k_queue_cancel_wait(struct k_queue *queue);
extern void k_queue_append(struct k_queue *queue, void *data);
int32_t k_queue_alloc_append(struct k_queue *queue, void *data);
extern void k_queue_prepend(struct k_queue *queue, void *data);
extern int32_t k_queue_alloc_prepend(struct k_queue *queue, void *data);
extern void k_queue_insert(struct k_queue *queue, void *prev, void *data);;
extern int k_queue_append_list(struct k_queue *queue, void *head, void *tail);
///extern int k_queue_merge_slist(struct k_queue *queue, sys_slist_t *list);
extern void *k_queue_get(struct k_queue *queue, k_timeout_t timeout);

static inline bool k_queue_remove(struct k_queue *queue, void *data)
{
	return sys_slist_find_and_remove(&queue->data_q, (sys_snode_t *)data);
}

static inline void *k_queue_first_entry(struct k_queue *queue)
{
    return sys_slist_peek_head(&queue->data_q);
}

extern int k_queue_is_empty(struct k_queue *queue);

extern int k_queue_is_uninitialized(struct k_queue *queue);

#define _POLL_EVENT_OBJ_INIT(obj) \
    .poll_events = SYS_DLIST_STATIC_INIT(&obj.poll_events),

#define K_QUEUE_INITIALIZER(obj) {  \
    .data_q = SYS_SLIST_STATIC_INIT(&obj.data_q),  \
    _POLL_EVENT_OBJ_INIT(obj), \
    .sem = NULL, \
}

#define Z_QUEUE_INITIALIZER(obj) \
	{ \
	.data_q = SYS_SLIST_STATIC_INIT(&obj.data_q), \
	.poll_events = SYS_DLIST_STATIC_INIT(&obj.poll_events), \
	.sem = NULL,	\
	}

struct k_fifo {
	struct k_queue _queue;
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_FIFO_INITIALIZER(obj) \
	{ \
	._queue = Z_QUEUE_INITIALIZER(obj._queue) \
	}

/**
 * @defgroup fifo_apis FIFO APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a FIFO queue.
 *
 * This routine initializes a FIFO queue, prior to its first use.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return N/A
 */
#define k_fifo_init(fifo) \
	k_queue_init(&(fifo)->_queue)

/**
 * @brief Cancel waiting on a FIFO queue.
 *
 * This routine causes first thread pending on @a fifo, if any, to
 * return from k_fifo_get() call with NULL value (as if timeout
 * expired).
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return N/A
 */
#define k_fifo_cancel_wait(fifo) \
	k_queue_cancel_wait(&(fifo)->_queue)

/**
 * @brief Add an element to a FIFO queue.
 *
 * This routine adds a data item to @a fifo. A FIFO data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO.
 * @param data Address of the data item.
 *
 * @return N/A
 */
#define k_fifo_put(fifo, data) \
	k_queue_append(&(fifo)->_queue, data)

/**
 * @brief Add an element to a FIFO queue.
 *
 * This routine adds a data item to @a fifo. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
#define k_fifo_alloc_put(fifo, data) \
	k_queue_alloc_append(&(fifo)->_queue, data)

/**
 * @brief Atomically add a list of elements to a FIFO.
 *
 * This routine adds a list of data items to @a fifo in one operation.
 * The data items must be in a singly-linked list, with the first word of
 * each data item pointing to the next data item; the list must be
 * NULL-terminated.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO queue.
 * @param head Pointer to first node in singly-linked list.
 * @param tail Pointer to last node in singly-linked list.
 *
 * @return N/A
 */
#define k_fifo_put_list(fifo, head, tail) \
	k_queue_append_list(&(fifo)->_queue, head, tail)

/**
 * @brief Get an element from a FIFO queue.
 *
 * This routine removes a data item from @a fifo in a "first in, first out"
 * manner. The first word of the data item is reserved for the kernel's use.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param fifo Address of the FIFO queue.
 * @param timeout Waiting period to obtain a data item,
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
#define k_fifo_get(fifo, timeout) \
	k_queue_get(&(fifo)->_queue, timeout)

/**
 * @brief Query a FIFO queue to see if it has data available.
 *
 * Note that the data might be already gone by the time this function returns
 * if other threads is also trying to read from the FIFO.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the FIFO queue.
 *
 * @return Non-zero if the FIFO queue is empty.
 * @return 0 if data is available.
 */
#define k_fifo_is_empty(fifo) \
	k_queue_is_empty(&(fifo)->_queue)

#define k_fifo_is_uninitialized(fifo) \
	k_queue_is_uninitialized(&(fifo)->_queue)

/**
 * @brief Statically define and initialize a FIFO queue.
 *
 * The FIFO queue can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_fifo <name>; @endcode
 *
 * @param name Name of the FIFO queue.
 */
#define K_FIFO_DEFINE(name) \
	Z_STRUCT_SECTION_ITERABLE_ALTERNATE(k_queue, k_fifo, name) = \
		Z_FIFO_INITIALIZER(name)



struct k_lifo {
    struct k_queue _queue;
};

#define K_LIFO_INITIALIZER(obj) {  \
    ._queue = K_QUEUE_INITIALIZER(obj._queue) \
}

#define Z_LIFO_INITIALIZER(obj) \
	{ \
	._queue = Z_QUEUE_INITIALIZER(obj._queue) \
	}

#define k_lifo_init(lifo) k_queue_init((struct k_queue *)lifo)
#define k_lifo_put(lifo, data) k_queue_prepend((struct k_queue *)lifo, data)
#define k_lifo_get(lifo, timeout) k_queue_get((struct k_queue *)lifo, timeout)
#define k_lifo_is_uninitialized(lifo) \
	k_queue_is_uninitialized(&(lifo)->_queue)

#define K_LIFO_DEFINE(name) struct k_lifo name = K_LIFO_INITIALIZER(name)




#endif

