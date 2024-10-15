#ifndef _Z_MEM_SLAB_H_
#define _Z_MEM_SLAB_H_

///#define CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION

#define Z_MEM_SLAB_SYS_CFG        1

#define MEM_SLAB_NONE_LOCK        0
#define MEM_SLAB_PUBLIC_LOCK      1
#define MEM_SLAB_PRIVATE_LOCK     2

#define CFG_MEM_SLAB_LOCK         MEM_SLAB_PUBLIC_LOCK

#include "z_wait_q.h"
#include "stdint.h"

#ifndef __noinit
#define __noinit
#endif

/**
* @cond INTERNAL_HIDDEN
*/

struct k_mem_slab {
	_wait_q_t wait_q;
	uint32_t num_blocks;
	size_t block_size;
	char *buffer;
	char *free_list;
	uint32_t num_used;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	uint32_t max_used;
#endif
#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PRIVATE_LOCK
	void *lock;
#endif
	void* sem;
	uint32_t wait_num;
};


#define Z_MEM_SLAB_INITIALIZER(obj, slab_buffer, slab_block_size, \
			       slab_num_blocks) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.num_blocks = slab_num_blocks, \
	.block_size = slab_block_size, \
	.buffer = slab_buffer, \
	.free_list = NULL, \
	.num_used = 0, \
	}

/**
 * @defgroup mem_slab_apis Memory Slab APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Statically define and initialize a memory slab.
 *
 * The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer is aligned to a
 * @a slab_align -byte boundary. To ensure that each memory block is similarly
 * aligned to this boundary, @a slab_block_size must also be a multiple of
 * @a slab_align.
 *
 * The memory slab can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct k_mem_slab <name>; @endcode
 *
 * @param name Name of the memory slab.
 * @param slab_block_size Size of each memory block (in bytes).
 * @param slab_num_blocks Number memory blocks.
 * @param slab_align Alignment of the memory slab's buffer (power of 2).
 */
#define K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align) \
	char __noinit __aligned(WB_UP(slab_align)) \
	   _k_mem_slab_buf_##name[(slab_num_blocks) * WB_UP(slab_block_size)]; \
	Z_STRUCT_SECTION_ITERABLE(k_mem_slab, name) = \
		Z_MEM_SLAB_INITIALIZER(name, _k_mem_slab_buf_##name, \
					WB_UP(slab_block_size), slab_num_blocks)

extern int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
		    size_t block_size, uint32_t num_blocks);

extern int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, struct k_timeout timeout);
extern void k_mem_slab_free(struct k_mem_slab *slab, void **mem);

static inline uint32_t k_mem_slab_num_used_get(struct k_mem_slab *slab)
{
	return slab->num_used;
}

static inline uint32_t k_mem_slab_max_used_get(struct k_mem_slab *slab)
{
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	return slab->max_used;
#else
	ARG_UNUSED(slab);
	return 0;
#endif
}

static inline uint32_t k_mem_slab_num_free_get(struct k_mem_slab *slab)
{
	return slab->num_blocks - slab->num_used;
}

int init_mem_slab_module(void);

#endif

///Eof

