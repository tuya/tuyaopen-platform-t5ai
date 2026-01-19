/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr.h"
#include "sys/z_mem_slab.h"
#include "sys/z_wait_q.h"
#include "sys/__assert.h"

#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PUBLIC_LOCK
static beken_mutex_t lock = NULL;
#endif
struct device;

#ifdef CONFIG_OBJECT_TRACING
struct k_mem_slab *_trace_list_k_mem_slab;
#endif	/* CONFIG_OBJECT_TRACING */

#ifndef CHECKIF
#if defined(CONFIG_ASSERT_ON_ERRORS)
	#define CHECKIF(expr) \
		__ASSERT_NO_MSG(!(expr));   \
		if (0)
	#elif defined(CONFIG_NO_RUNTIME_CHECKS)
	#define CHECKIF(...) \
		if (0)
	#else
	#define CHECKIF(expr) \
		if (expr)
	#endif
#endif

/**
 * @brief Initialize kernel memory slab subsystem.
 *
 * Perform any initialization of memory slabs that wasn't done at build time.
 * Currently this just involves creating the list of free blocks for each slab.
 *
 * @return N/A
 */
static int create_free_list(struct k_mem_slab *slab)
{
	uint32_t j;
	char *p;

	/* blocks must be word aligned */
	CHECKIF(((slab->block_size | (uintptr_t)slab->buffer) &
				(sizeof(void *) - 1)) != 0) {
		return -EINVAL;
	}
#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PRIVATE_LOCK
	if(slab->lock == NULL) {
		rtos_init_mutex(&slab->lock);
	}
	rtos_lock_mutex(&slab->lock);
#endif

	slab->free_list = NULL;
	p = slab->buffer;
	if(slab->sem == NULL) {
		rtos_init_semaphore(&slab->sem,slab->num_blocks);
	}

	for (j = 0U; j < slab->num_blocks; j++) {
		*(char **)p = slab->free_list;
		slab->free_list = p;
		p += slab->block_size;
		rtos_set_semaphore(&slab->sem);
	}
#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PRIVATE_LOCK
	rtos_unlock_mutex(&slab->lock);
#endif

	return 0;
}

#if Z_MEM_SLAB_SYS_CFG

#if CONFIG_ACLSEMI_BTC_FEATURE_INCLUDED

int32_t ethermind_bt_k_mem_slab_callback(struct k_mem_slab *param, void *arg1, void *arg2, void *arg3, void *arg4)
{
	int rc = 0;
#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PRIVATE_LOCK
	if(param->lock == NULL) {
		rtos_init_mutex(&param->lock);
	}
#endif
	rc = create_free_list(param);
	if (rc < 0)
	{
		return rc;
	}
#if 0
	SYS_TRACING_OBJ_INIT(k_mem_slab, slab);
	z_object_init(slab);
#endif

	return 0;
}

#endif

/**
 * @brief Complete initialization of statically defined memory slabs.
 *
 * Perform any initialization that wasn't done at build time.
 *
 * @return N/A
 */
int init_mem_slab_module(void)
{
	int rc = 0;

#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PUBLIC_LOCK
	if(lock == NULL){
		rtos_init_mutex(&lock);
	}
#endif
#if CONFIG_ACLSEMI_BTC_FEATURE_INCLUDED
	extern int32_t mesh_foreach_k_mem_slab_cb(int32_t (*cb)(struct k_mem_slab *param, void *arg1, void *arg2, void *arg3, void *arg4),
            void *arg1, void *arg2, void *arg3, void *arg4);
	rc = mesh_foreach_k_mem_slab_cb(ethermind_bt_k_mem_slab_callback, NULL, NULL, NULL, NULL);

	if(rc < 0)
	{
		goto out;
	}

#else
	Z_STRUCT_SECTION_FOREACH(k_mem_slab, slab) {
		#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PRIVATE_LOCK
		if(slab->lock == NULL) {
			rtos_init_mutex(&slab->lock);
		}
		#endif
		rc = create_free_list(slab);
		if (rc < 0) {
			goto out;
		}
#if 0
		SYS_TRACING_OBJ_INIT(k_mem_slab, slab);
		z_object_init(slab);
#endif
	}
#endif
out:
	return rc;
}
#endif

int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
		    size_t block_size, uint32_t num_blocks)
{
	int rc = 0;

	slab->num_blocks = num_blocks;
	slab->block_size = block_size;
	slab->buffer = buffer;
	slab->num_used = 0U;
	slab->wait_num = 0U;

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	slab->max_used = 0U;
#endif
#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PUBLIC_LOCK
	if(lock == NULL){
		rtos_init_mutex(&lock);
	}
#elif defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PRIVATE_LOCK
	if(slab->lock == NULL) {
		rtos_init_mutex(&slab->lock);
	}
#endif

	rc = create_free_list(slab);
	if (rc < 0) {
		goto out;
	}
	z_waitq_init(&slab->wait_q);
#if 0
	SYS_TRACING_OBJ_INIT(k_mem_slab, slab);

	z_object_init(slab);
#endif

out:
	return rc;
}

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, struct k_timeout timeout)
{
	beken_mutex_t* _lock = NULL;
#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PUBLIC_LOCK
	if ( lock == NULL ) {
		rtos_init_mutex(&lock);
	}
	_lock = &lock;
#elif defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PRIVATE_LOCK
	if(slab->lock == NULL) {
		rtos_init_mutex(&slab->lock);
	}
	_lock = &slab->lock;
#else
	_lock = NULL;
#endif
	int result;
	if(_lock){
		rtos_lock_mutex(_lock);
	}
	if(slab->sem == NULL) {
		rtos_init_semaphore(&slab->sem,slab->num_blocks);
	}

	if ((slab->free_list != NULL) && (!slab->wait_num)){
	get_mem:
		/* take a free block */
		*mem = slab->free_list;
		slab->free_list = *(char **)(slab->free_list);
		slab->num_used++;
		if(slab->free_list == NULL){
			do{
				if(slab->free_list != NULL){
					break;
				}
			}while(rtos_get_semaphore(&slab->sem,0) == kNoErr);
		}else{
			rtos_get_semaphore(&slab->sem,0);
		}
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
		slab->max_used = MAX(slab->num_used, slab->max_used);
#endif

		result = 0;
	}else {
		slab->wait_num++;
	retry:
		if(_lock) {
			rtos_unlock_mutex(_lock);
		}
		result = rtos_get_semaphore(&slab->sem,timeout.ticks);
		if (result == 0) {
			if(_lock) {
				rtos_lock_mutex(_lock);
			}
			if (slab->free_list != NULL) {
				goto get_mem;
			} else {
				goto retry;
			}
		}else{
			*mem = NULL;
			result = -ENOMEM;

			return result;
		}
	}

	if(_lock){
		rtos_unlock_mutex(_lock);
	}
	return result;
}

void k_mem_slab_free(struct k_mem_slab *slab, void **mem)
{
	beken_mutex_t* _lock;
#if defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PUBLIC_LOCK
	if ( lock == NULL ) {
		rtos_init_mutex(&lock);
	}
	_lock = &lock;
#elif defined(CFG_MEM_SLAB_LOCK) && CFG_MEM_SLAB_LOCK == MEM_SLAB_PRIVATE_LOCK
	if ( slab->lock == NULL ) {
		rtos_init_mutex(&slab->lock);
	}
	_lock = &slab->lock;
#else
	_lock = NULL;
#endif
	if ( _lock ) {
		rtos_lock_mutex(_lock);
	}
	if(slab->sem == NULL) {
		rtos_init_semaphore(&slab->sem,slab->num_blocks);
	}

	**(char ***) mem = slab->free_list;
	slab->free_list = *(char **) mem;
	slab->num_used--;
	rtos_set_semaphore(&slab->sem);

	if(_lock){
		rtos_unlock_mutex(_lock);
	}

}
