/**
 * @file tkl_atomic.c
 * @brief the default weak implements of tuya os system api, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_atomic.h"
#include "FreeRTOS.h"
#include "task.h"
#include <os/os.h>
#include <components/system.h>
#include "tkl_memory.h"
#include "atomic.h"
#include "sdkconfig.h"

/**
 * @brief add one to value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @return  value before increment
 */
UINT_T tkl_system_atomic_inc(UINT_T volatile *val)
{
    return Atomic_Increment_u32(val);
}

/**
 * @brief decrement one from value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @return  value before decrement
 */
UINT_T tkl_system_atomic_dec(UINT_T volatile *val)
{
    return Atomic_Decrement_u32(val);
}

/**
 * @brief add count to value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @param[in]   count: Value to be add to val
 * @return  value before add.
 */
UINT_T tkl_system_atomic_add(UINT_T volatile *val, UINT_T count)
{
    return Atomic_Add_u32(val, count);
}

/**
 * @brief subtract count from value
 *
 * @param[in]   val: Pointer to memory location from where value is to be
 *                         loaded and written back to
 * @param[in]   count: Value to be subtract from val
 * @return  value before sub.
 */
UINT_T tkl_system_atomic_sub(UINT_T volatile *val, UINT_T count)
{
    return Atomic_Subtract_u32(val, count);
}

/**
 * @brief atomic exchange
 *
 * @param[in]   pdst: dest memory
 * @param[in]   psrc: source memory
 * @return  dest original value
 */
VOID_T *tkl_system_atomic_swap(VOID_T * volatile * pdst, VOID_T * psrc)
{
    return Atomic_SwapPointers_p32(pdst, psrc);
}

/**
 * @brief atomic compare and set val
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @param[in]   compare: value of be compared
 * @return  Unsigned integer of value true or false. true for swapped, false for not swapped.
 */
BOOL_T tkl_system_atomic_cmp_and_set(UINT_T volatile * pdst, UINT_T val, UINT_T compare)
{
    BOOL_T ret = 0;
    ret = Atomic_CompareAndSwap_u32(pdst, val, compare);
    return ret == ATOMIC_COMPARE_AND_SWAP_SUCCESS ? true : false;
}

/**
 * @brief atomic compare and exchange
 *
 * @param[in]   pdst: dest memory
 * @param[in]   psrc: source memory
 * @param[in]   compare: value of be compared
 * @return  Unsigned integer of value true or false. true for swapped, false for not swapped.
 */
BOOL_T tkl_system_atomic_cmp_and_swap(VOID_T * volatile * pdst, VOID_T * psrc, VOID_T * pcmp)
{
    BOOL_T ret = 0;
    ret = Atomic_CompareAndSwapPointers_p32(pdst, psrc, pcmp);
    return ret == ATOMIC_COMPARE_AND_SWAP_SUCCESS ? true : false;
}

/**
 * @brief atomic OR
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
UINT_T tkl_system_atomic_or(UINT_T volatile * pdst, UINT_T val)
{
    return Atomic_OR_u32(pdst, val);
}

/**
 * @brief atomic AND
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
UINT_T tkl_system_atomic_and(UINT_T volatile * pdst, UINT_T val)
{
    return Atomic_AND_u32(pdst, val);
}

/**
 * @brief atomic NAND
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
UINT_T tkl_system_atomic_nand(UINT_T volatile * pdst, UINT_T val)
{
    return Atomic_NAND_u32(pdst, val);
}

/**
 * @brief atomic XOR
 *
 * @param[in]   pdst: dest memory
 * @param[in]   val: set value
 * @return  dest original value.
 */
UINT_T tkl_system_atomic_xor(UINT_T volatile * pdst, UINT_T val)
{
    return Atomic_XOR_u32(pdst, val);
}
