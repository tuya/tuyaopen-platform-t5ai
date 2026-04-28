/**
* @file tkl_atomic.h
* @brief Common process - adpater some api which provide system
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_ATOMIC_H__
#define __TKL_ATOMIC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief add one to value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @return  value before increment
 */
uint32_t tkl_system_atomic_inc(uint32_t volatile *val);

/**
 * @brief decrement one from value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @return  value before decrement
 */
uint32_t tkl_system_atomic_dec(uint32_t volatile *val);

/**
 * @brief add count to value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @param[in]   count: Value to be add to val
 * @return  previous *val value.
 */
uint32_t tkl_system_atomic_add(uint32_t volatile *val, uint32_t count);

/**
 * @brief subtract count from value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @param[in]   count: Value to be subtract from val
 * @return  previous *val value.
 */
uint32_t tkl_system_atomic_sub(uint32_t volatile *val, uint32_t count);

/**
 * @brief atomic exchange
 *
 * @param[in]   pdst: dest memory
 * @param[in]   psrc: source memory
 * @return  dest original value
 */
void *tkl_system_atomic_swap(void * volatile *pdst, void * psrc);


/**
 * @brief atomic compare and set val
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @param[in]   compare: value of be compared
 * @return  Unsigned integer of value 1 or 0. 1 for swapped, 0 for not swapped.
 */
BOOL_T tkl_system_atomic_cmp_and_set(uint32_t volatile *pdst, uint32_t val, uint32_t compare);

/**
 * @brief atomic compare and exchange
 *
 * @param[in]   pdst: dest memory
 * @param[in]   psrc: source memory
 * @param[in]   compare: value of be compared
 * @return  Unsigned integer of value 1 or 0. 1 for swapped, 0 for not swapped.
 */
BOOL_T tkl_system_atomic_cmp_and_swap(void * volatile *pdst, void * psrc, void * pcmp);

/**
 * @brief atomic AND
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
uint32_t tkl_system_atomic_and(uint32_t volatile *pdst, uint32_t val);

/**
 * @brief atomic OR
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
uint32_t tkl_system_atomic_or(uint32_t volatile *pdst, uint32_t val);

/**
 * @brief atomic NAND
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
uint32_t tkl_system_atomic_nand(uint32_t volatile *pdst, uint32_t val);

/**
 * @brief atomic XOR
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
uint32_t tkl_system_atomic_xor(uint32_t volatile *pdst, uint32_t val);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

