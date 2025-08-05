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
UINT_T tkl_system_atomic_inc(UINT_T volatile *val);

/**
 * @brief decrement one from value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @return  value before decrement
 */
UINT_T tkl_system_atomic_dec(UINT_T volatile *val);

/**
 * @brief add count to value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @param[in]   count: Value to be add to val
 * @return  previous *val value.
 */
UINT_T tkl_system_atomic_add(UINT_T volatile *val, UINT_T count);

/**
 * @brief subtract count from value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @param[in]   count: Value to be subtract from val
 * @return  previous *val value.
 */
UINT_T tkl_system_atomic_sub(UINT_T volatile *val, UINT_T count);

/**
 * @brief atomic exchange
 *
 * @param[in]   pdst: dest memory
 * @param[in]   psrc: source memory
 * @return  dest original value
 */
VOID_T *tkl_system_atomic_swap(VOID_T * volatile *pdst, VOID_T * psrc);


/**
 * @brief atomic compare and set val
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @param[in]   compare: value of be compared
 * @return  Unsigned integer of value 1 or 0. 1 for swapped, 0 for not swapped.
 */
BOOL_T tkl_system_atomic_cmp_and_set(UINT_T volatile *pdst, UINT_T val, UINT_T compare);

/**
 * @brief atomic compare and exchange
 *
 * @param[in]   pdst: dest memory
 * @param[in]   psrc: source memory
 * @param[in]   compare: value of be compared
 * @return  Unsigned integer of value 1 or 0. 1 for swapped, 0 for not swapped.
 */
BOOL_T tkl_system_atomic_cmp_and_swap(VOID_T * volatile *pdst, VOID_T * psrc, VOID_T * pcmp);

/**
 * @brief atomic AND
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
UINT_T tkl_system_atomic_and(UINT_T volatile *pdst, UINT_T val);

/**
 * @brief atomic OR
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
UINT_T tkl_system_atomic_or(UINT_T volatile *pdst, UINT_T val);

/**
 * @brief atomic NAND
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
UINT_T tkl_system_atomic_nand(UINT_T volatile *pdst, UINT_T val);

/**
 * @brief atomic XOR
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
UINT_T tkl_system_atomic_xor(UINT_T volatile *pdst, UINT_T val);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

