#ifndef _ZEPHYR_RTOS_H_
#define _ZEPHYR_RTOS_H_

#include <os/os.h>
#include <errno.h>
#include "sys/dlist.h"
#include "sys/slist.h"

#ifndef __syscall
#define __syscall
#endif

/* concatenate the values of the arguments into one */
#define _DO_CONCAT(x, y) x ## y
#define _CONCAT(x, y) _DO_CONCAT(x, y)

#ifndef __COUNTER__
/* XCC (GCC-based compiler) doesn't support __COUNTER__
 * but this should be good enough
 */
#define __COUNTER__ __LINE__
#endif

///#define __used		__attribute__((__used__))
//#define __used

#ifndef BUILD_ASSERT
/* Compile-time assertion that makes the build to fail.
 * Common implementation swallows the message.
 */
#define BUILD_ASSERT(EXPR, MSG...) \
	enum _CONCAT(__build_assert_enum, __COUNTER__) { \
		_CONCAT(__build_assert, __COUNTER__) = 1 / !!(EXPR) \
	}
#endif

///#define __alignof(type)   __attribute__((aligned(sizeof(type))))
///#define __alignof(type) sizeof(void*)
#define Z_DECL_ALIGN(type) __aligned(__alignof(type)) type

#define Z_STRUCT_SECTION_ITERABLE_ALTERNATE(out_type, struct_type, name) \
	Z_DECL_ALIGN(struct struct_type) name \
	__in_section(_##out_type, static, name) __used

/*
 * Convenience helper combining __in_section() and Z_DECL_ALIGN().
 * The section name is the struct type prepended with an underscore.
 * The subsection is "static" and the subsubsection is the variable name.
 *
 * In the linker script, create output sections for these using
 * Z_ITERABLE_SECTION_ROM or Z_ITERABLE_SECTION_RAM.
 */
#define Z_STRUCT_SECTION_ITERABLE(struct_type, name) \
	Z_DECL_ALIGN(struct struct_type) name \
	__in_section(_##struct_type, static, name) __used

#define STRUCT_SECTION_ITERABLE(struct_type, name) Z_STRUCT_SECTION_ITERABLE(struct_type, name)

struct __packed z_thread_stack_element {
	unsigned int data;
};

typedef struct z_thread_stack_element k_thread_stack_t;

#define K_THREAD_STACK_SIZEOF(a)   NULL
#define K_KERNEL_STACK_DEFINE(sym, size) k_thread_stack_t sym[1] = {{size}}
#define K_KERNEL_STACK_SIZEOF(sym) ((sym)[0].data)
#define K_PRIO_COOP(x) ((BEKEN_APPLICATION_PRIORITY + (x)))

#define K_NO_WAIT      ((k_timeout_t) {BEKEN_NO_WAIT}) ///os_no_wait_timeout ////BEKEN_NO_WAIT
#define K_FOREVER      ((k_timeout_t) {BEKEN_NEVER_TIMEOUT}) ///os_never_timeout  ///BEKEN_NEVER_TIMEOUT

///#define UINT64_MAX     0xFFFFFFFFFFFFFFFFU

#define MSEC_PER_SEC 1000U


#define Z_TIMEOUT_MS(t)  ((k_timeout_t){ (t) })

/**
 * @brief Generate timeout delay from milliseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a ms milliseconds to perform the requested operation.
 *
 * @param ms Duration in milliseconds.
 *
 * @return Timeout delay value.
 */
#define K_MSEC(ms)     Z_TIMEOUT_MS(ms)

/**
 * @brief Generate timeout delay from seconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a s seconds to perform the requested operation.
 *
 * @param s Duration in seconds.
 *
 * @return Timeout delay value.
 */
#define K_SECONDS(s)  K_MSEC((s) * MSEC_PER_SEC)
#define K_MINUTES(m) K_SECONDS((m) * 60)
#define K_HOURS(h) K_MINUTES((h) * 60)

#define SYS_FOREVER_MS (-1)
#define SYS_TIMEOUT_MS(ms) ((ms) == SYS_FOREVER_MS ? K_FOREVER : K_MSEC(ms))

#define K_TIMEOUT_EQ(a, b) ((a).ticks == (b).ticks)
#define Z_TIMEOUT_TICKS(t) ((k_timeout_t) { .ticks = (t) })

#define _XXXX1 _YYYY,
#define Z_IS_ENABLED3(ignore_this, val, ...) val
#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args 1, 0)
#define Z_IS_ENABLED1(config_macro) Z_IS_ENABLED2(_XXXX##config_macro)
#define IS_ENABLED(config_macro) Z_IS_ENABLED1(config_macro)

#define Z_STRINGIFY(x) #x
#define STRINGIFY(s) Z_STRINGIFY(s)

typedef struct k_timeout{
	unsigned int ticks;
} k_timeout_t;

extern const k_timeout_t os_never_timeout;
extern const k_timeout_t os_no_wait_timeout;

typedef void (*k_thread_entry_t)(void *p1, void *p2, void *p3);
struct k_thread {
	void* thd;

	k_thread_entry_t entry;
	void *p1;
	void *p2;
	void *p3;

	k_timeout_t delay;
	const char *name;
};


typedef struct k_thread *k_tid_t;


/**
 * @brief Create a thread.
 *
 * This routine initializes a thread, then schedules it for execution.
 *
 * The new thread may be scheduled for immediate execution or a delayed start.
 * If the newly spawned thread does not have a delayed start the kernel
 * scheduler may preempt the current thread to allow the new thread to
 * execute.
 *
 * Thread options are architecture-specific, and can include K_ESSENTIAL,
 * K_FP_REGS, and K_SSE_REGS. Multiple options may be specified by separating
 * them using "|" (the logical OR operator).
 *
 * Stack objects passed to this function must be originally defined with
 * either of these macros in order to be portable:
 *
 * - K_THREAD_STACK_DEFINE() - For stacks that may support either user or
 *   supervisor threads.
 * - K_KERNEL_STACK_DEFINE() - For stacks that may support supervisor
 *   threads only. These stacks use less memory if CONFIG_USERSPACE is
 *   enabled.
 *
 * The stack_size parameter has constraints. It must either be:
 *
 * - The original size value passed to K_THREAD_STACK_DEFINE() or
 *   K_KERNEL_STACK_DEFINE()
 * - The return value of K_THREAD_STACK_SIZEOF(stack) if the stack was
 *   defined with K_THREAD_STACK_DEFINE()
 * - The return value of K_KERNEL_STACK_SIZEOF(stack) if the stack was
 *   defined with K_KERNEL_STACK_DEFINE().
 *
 * Using other values, or sizeof(stack) may produce undefined behavior.
 *
 * @param new_thread Pointer to uninitialized struct k_thread
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 * @param delay Scheduling delay, or K_NO_WAIT (for no delay).
 *
 * @return ID of new thread.
 *
 */
__syscall k_tid_t k_thread_create(struct k_thread *new_thread,
				  k_thread_stack_t *stack,
				  size_t stack_size,
				  k_thread_entry_t entry,
				  void *p1, void *p2, void *p3,
				  int prio, uint32_t options, k_timeout_t delay);

void k_thread_start(k_tid_t thread);

extern int k_thread_name_set(struct k_thread *thread, const char *value);
extern const char *k_thread_name_get(struct k_thread *thread);
extern int k_thread_name_copy(k_tid_t thread_id, char *buf, size_t size);
extern void** k_current_get(void);

extern void k_sleep(k_timeout_t timeout);
extern long long k_now_ms();

static inline int64_t k_uptime_get(void)
{
    return (int64_t)rtos_get_time();
}

static inline uint32_t k_uptime_get_32(void)
{
    return (uint32_t)rtos_get_time();
}

static inline int64_t k_uptime_delta(uint64_t *reftime)
{
	int64_t uptime, delta;

	uptime = k_uptime_get();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

/**
 * @cond INTERNAL_HIDDEN
 */

struct k_sem {
	beken_semaphore_t sem;
	sys_dlist_t poll_events;
	unsigned short pend_count;
	unsigned short init_count;
	unsigned short maxCount;
};

extern int k_sem_init(struct k_sem *sem, unsigned int initial_count,
			  unsigned int limit);
extern int k_sem_reset(struct k_sem *sem);
extern int k_sem_take(struct k_sem *sem, k_timeout_t timeout);
extern int k_sem_give(struct k_sem *sem);
extern unsigned int k_sem_count_get(struct k_sem *sem);
extern int k_sem_delete(struct k_sem *sem);
extern int k_sem_is_uninitialized(struct k_sem *sem);

#define Z_SEM_INITIALIZER(obj, initial_count, count_limit) \
	{ \
	.sem = NULL, \
	_POLL_EVENT_OBJ_INIT(obj) \
	.init_count = initial_count, \
	.maxCount = count_limit, \
	.pend_count = 0U, \
	}

#define K_SEM_DEFINE(name, initial_count, count_limit) \
	STRUCT_SECTION_ITERABLE(k_sem, name) = \
		Z_SEM_INITIALIZER(name, initial_count, count_limit); \
	BUILD_ASSERT(((count_limit) != 0) && \
		((initial_count) <= (count_limit)));

/**
* Mutex Structure
* @ingroup mutex_apis
*/
typedef beken_mutex_t _mutex_t;
struct k_mutex {
	_mutex_t    mutex;
    sys_dlist_t poll_events;
};

extern void k_mutex_init(struct k_mutex *mutex);
extern int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout);
extern void k_mutex_unlock(struct k_mutex *mutex);

extern int k_yield(void);

typedef void (*k_timer_expiry_t)(void *timer);
///typedef void (*k_timer_expiry_t)(struct k_timer *timer);
typedef struct k_timer
{
    beken_timer_t          timer;
    timer_handler_t handler;
	void *args;
    uint32_t          timeout;
    uint32_t          start_ms;
} k_timer_t;

extern void k_timer_init(k_timer_t *timer, k_timer_expiry_t handle, void *args);
extern void k_timer_start(k_timer_t *timer, uint32_t timeout);
extern void k_timer_stop(k_timer_t *timer);

extern void *k_malloc(uint32_t size);
extern void k_free(void* data);

extern void fz_mutex_lock_init(void);
extern unsigned int irq_lock(void);
extern void irq_unlock(unsigned int key);

extern int Z_WORK_CMP_TIME(uint32_t a, uint32_t b);
extern unsigned int Z_WORK_SUB_TIME_DELT(uint32_t a, uint32_t b);

struct k_heap {
	///struct sys_heap heap;
	//_wait_q_t wait_q;
	///struct k_spinlock lock;
	char data;
};

extern void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
			k_timeout_t timeout);
extern void k_heap_free(struct k_heap *h, void *mem);
static inline void *k_heap_alloc(struct k_heap *h, size_t bytes,
				 k_timeout_t timeout)
{
	return k_heap_aligned_alloc(h, sizeof(void *), bytes, timeout);
}

#define K_TICKS_FOREVER ((unsigned int) -1)
#define Z_TICK_ABS(t) (K_TICKS_FOREVER - 1 - (t))

extern uint32_t z_tick_get(void);
extern uint32_t z_timeout_end_calc(k_timeout_t timeout);

/**
 * @brief Get a pointer to a container structure from an element
 *
 * Example:
 *
 *	struct foo {
 *		int bar;
 *	};
 *
 *	struct foo my_foo;
 *	int *ptr = &my_foo.bar;
 *
 *	struct foo *container = CONTAINER_OF(ptr, struct foo, bar);
 *
 * Above, @p container points at @p my_foo.
 *
 * @param ptr pointer to a structure element
 * @param type name of the type that @p ptr is an element of
 * @param field the name of the field within the struct @p ptr points to
 * @return a pointer to the structure that contains @p ptr
 */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, field) \
	((type *)(((char *)(ptr)) - offsetof(type, field)))
#endif


extern int char2hex(char c, uint8_t *x);
extern int hex2char(uint8_t x, char *c);
extern size_t bin2hex(const uint8_t *buf, size_t buflen, char *hex, size_t hexlen);

struct _poller{
	void* thd;
};

/*
 * Itterator for structure instances gathered by Z_STRUCT_SECTION_ITERABLE().
 * The linker must provide a _<struct_type>_list_start symbol and a
 * _<struct_type>_list_end symbol to mark the start and the end of the
 * list of struct objects to iterate over.
 */
#define Z_STRUCT_SECTION_FOREACH(struct_type, iterator) \
	extern struct struct_type _CONCAT(_##struct_type, _list_start)[]; \
	extern struct struct_type _CONCAT(_##struct_type, _list_end)[]; \
	for (struct struct_type *iterator = \
			_CONCAT(_##struct_type, _list_start); \
	     ({ __ASSERT(iterator <= _CONCAT(_##struct_type, _list_end), \
			 "unexpected list end location"); \
		iterator < _CONCAT(_##struct_type, _list_end); }); \
	     iterator++)

#define STRUCT_SECTION_FOREACH(struct_type, iterator) \
	Z_STRUCT_SECTION_FOREACH(struct_type, iterator)

/**
 * @brief Value of @p x rounded up to the next multiple of @p align,
 *		  which must be a power of 2.
 */
#define ROUND_UP(x, align)                                   \
		(((unsigned long)(x) + ((unsigned long)(align) - 1)) & \
		 ~((unsigned long)(align) - 1))

	/**
	 * @brief Value of @p x rounded down to the previous multiple of @p
	 *		  align, which must be a power of 2.
	 */
#define ROUND_DOWN(x, align)                                 \
		((unsigned long)(x) & ~((unsigned long)(align) - 1))

	/** @brief Value of @p x rounded up to the next word boundary. */
#define WB_UP(x) ROUND_UP(x, sizeof(void *))

	/** @brief Value of @p x rounded down to the previous word boundary. */
#define WB_DN(x) ROUND_DOWN(x, sizeof(void *))

void freertos_zephyr_port_init(void);
size_t hex2bin(const char *hex, size_t hexlen, uint8_t *buf, size_t buflen);

void k_sched_lock(void);
void k_sched_unlock(void);

#endif

