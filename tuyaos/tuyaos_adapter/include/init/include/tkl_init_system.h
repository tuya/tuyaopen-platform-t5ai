/**
* @file tkl_init_system.h
* @brief Common process - tkl init system object descption
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_INIT_SYSTEM_H__
#define __TKL_INIT_SYSTEM_H__

#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_semaphore.h"
#include "tkl_output.h"
#include "tkl_queue.h"
#include "tkl_ota.h"
#include "tkl_sleep.h"

#ifdef ENABLE_FILE_SYSTEM
#include "tkl_fs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief the description of tuya kernel adapter layer system api
 *
 */


typedef struct {
    //! system (cpu)
    VOID_T              (*reset)                    (VOID_T);
    INT_T               (*get_free_heap_size)       (VOID_T);
    SYS_TICK_T          (*get_tick_count)           (VOID_T);
    SYS_TIME_T          (*get_millisecond)          (VOID_T);
    INT_T               (*get_random)               (UINT_T range);
    TUYA_RESET_REASON_E (*get_reset_reason)        (CHAR_T** describe);
    VOID                (*sleep)                    (CONST UINT_T num_ms);
    OPERATE_RET         (*cpu_sleep_mode_set)       (BOOL_T enable, TUYA_CPU_SLEEP_MODE_E mode);
    OPERATE_RET         (*get_cpu_info)             (TUYA_CPU_INFO_T **cpu_ary, INT_T *cpu_cnt);
    //! output     
    VOID_T              (*log_output)               (CONST CHAR_T *log, ...);
    OPERATE_RET         (*log_close)                (VOID_T);
    OPERATE_RET         (*log_open)                 (VOID_T);
    //! ota
    OPERATE_RET         (*ota_get_ability)          (UINT_T *image_size, TUYA_OTA_TYPE_E *type);
    OPERATE_RET         (*ota_start)                (UINT_T image_size, TUYA_OTA_TYPE_E type, TUYA_OTA_PATH_E path);
    OPERATE_RET         (*ota_process)              (TUYA_OTA_DATA_T *pack, UINT_T* remain_len);
    OPERATE_RET         (*ota_end)                  (BOOL_T reset);
    //! memory
    VOID_T*             (*malloc)                   (CONST size_t size);
    VOID_T              (*free)                     (VOID_T* ptr);
    VOID_T*             (*calloc)                   (size_t nitems, size_t size);
    VOID_T*             (*realloc)                  (VOID_T* ptr, size_t size);
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM==1)
    VOID_T*             (*psram_malloc)             (CONST size_t size);
    VOID_T              (*psram_free)               (VOID_T* ptr);
    VOID_T*             (*psram_calloc)             (size_t nitems, size_t size);
    VOID_T*             (*psram_realloc)            (VOID_T* ptr, size_t size);
    INT_T               (*psarm_get_free_heap_size) (VOID_T);
#endif

    //! thread
    OPERATE_RET         (*thread_create)            (TKL_THREAD_HANDLE* thread, CONST CHAR_T* name, CONST UINT_T stack_size, CONST UINT_T priority, CONST THREAD_FUNC_T func, VOID_T* CONST arg);
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM==1)
    OPERATE_RET         (*thread_create_in_psram)   (TKL_THREAD_HANDLE* thread, CONST CHAR_T* name, CONST UINT_T stack_size, CONST UINT_T priority, CONST THREAD_FUNC_T func, VOID_T* CONST arg);
#endif
    OPERATE_RET         (*thread_release)           (CONST TKL_THREAD_HANDLE thread);
    OPERATE_RET         (*thread_set_name)          (CONST CHAR_T* name);
    OPERATE_RET         (*thread_set_priority)      (TKL_THREAD_HANDLE thread, INT_T  priority);
    OPERATE_RET         (*thread_get_priority)      (TKL_THREAD_HANDLE thread, INT_T *priority);
    OPERATE_RET         (*thread_get_watermark)     (CONST TKL_THREAD_HANDLE thread, UINT_T* watermark);    
    OPERATE_RET         (*thread_get_id)            (TKL_THREAD_HANDLE* thread);
    OPERATE_RET         (*thread_is_self)           (TKL_THREAD_HANDLE thread, BOOL_T* is_self);
    OPERATE_RET         (*thread_diagnose)          (TKL_THREAD_HANDLE thread);
    INT_T               (*getheapsize)              (VOID_T);
    //! mutex
    OPERATE_RET         (*mutex_init)               (TKL_MUTEX_HANDLE *handle);
    OPERATE_RET         (*mutex_lock)               (CONST TKL_MUTEX_HANDLE handle);
    OPERATE_RET         (*mutex_trylock)            (CONST TKL_MUTEX_HANDLE handle);
    OPERATE_RET         (*mutex_unlock)             (CONST TKL_MUTEX_HANDLE handle);
    OPERATE_RET         (*mutex_release)            (CONST TKL_MUTEX_HANDLE handle);
    //! semaphore
    OPERATE_RET         (*sem_init)                 (TKL_SEM_HANDLE *handle, CONST UINT_T sem_cnt, CONST UINT_T sem_max);
    OPERATE_RET         (*sem_wait)                 (CONST TKL_SEM_HANDLE handle, CONST UINT_T timeout);
    OPERATE_RET         (*sem_post)                 (CONST TKL_SEM_HANDLE handle);
    OPERATE_RET         (*sem_release)              (CONST TKL_SEM_HANDLE handle);
    OPERATE_RET         (*sem_wait_timeout)         (CONST TKL_SEM_HANDLE handle, CONST UINT_T timeout);
    //! queue
    OPERATE_RET         (*queue_init)               (TKL_QUEUE_HANDLE *queue, CONST INT_T msgsize, CONST INT_T msgcount);
    VOID_T              (*queue_free)               (CONST TKL_QUEUE_HANDLE queue);
    OPERATE_RET         (*queue_post)               (CONST TKL_QUEUE_HANDLE queue, VOID_T *data, CONST UINT_T timeout);
    OPERATE_RET         (*queue_fetch)              (CONST TKL_QUEUE_HANDLE queue, VOID_T *msg, CONST UINT_T timeout);
} TKL_OS_T;

extern CONST TKL_OS_T TKL_OS;

/**
 * @brief get system os description
 *
 * @return TKL_OS instance
 */
TKL_OS_T* tkl_os_desc_get(VOID_T);

//#ifdef ENABLE_FILE_SYSTEM
/**
 *
 * @brief the description of tuya kernel adapter layer file system
 *
 */
typedef struct {
    OPERATE_RET         (*fs_mkdir)                 (CONST CHAR_T* path);
    OPERATE_RET         (*fs_mkdir_r)               (CONST CHAR_T* path);
    OPERATE_RET         (*fs_remove)                (CONST CHAR_T* path);
    OPERATE_RET         (*fs_remove_r)              (CONST CHAR_T* path);
    OPERATE_RET         (*fs_mode)                  (CONST CHAR_T* path, UINT_T* mode);
    OPERATE_RET         (*fs_is_exist)              (CONST CHAR_T* path, BOOL_T* is_exist);
    OPERATE_RET         (*fs_rename)                (CONST CHAR_T* path_old, CONST CHAR_T* path_new);
    OPERATE_RET         (*dir_open)                 (CONST CHAR_T* path, TUYA_DIR* dir);
    OPERATE_RET         (*dir_close)                (TUYA_DIR dir);
    OPERATE_RET         (*dir_read)                 (TUYA_DIR dir, TUYA_FILEINFO* info);
    OPERATE_RET         (*dir_name)                 (TUYA_FILEINFO info, CONST CHAR_T** name);
    OPERATE_RET         (*dir_is_dir)               (TUYA_FILEINFO info, BOOL_T* is_dir);
    OPERATE_RET         (*dir_is_regular)           (TUYA_FILEINFO info, BOOL_T* is_regular);
    TUYA_FILE           (*fopen)                    (CONST CHAR_T* path, CONST CHAR_T* mode);
    OPERATE_RET         (*fclose)                   (TUYA_FILE file);
    OPERATE_RET         (*fread)                    (VOID_T* buf, INT_T bytes, TUYA_FILE file);
    OPERATE_RET         (*fwrite)                   (VOID_T* buf, INT_T bytes, TUYA_FILE file);
    CHAR_T*             (*fgets)                    (CHAR_T* buf, INT_T len, TUYA_FILE file);
    OPERATE_RET         (*feof)                     (TUYA_FILE file);
    OPERATE_RET         (*fseek)                    (TUYA_FILE file, INT64_T offs, INT_T whence);
    INT64_T             (*ftell)                    (TUYA_FILE file);
    INT_T               (*fflush)                   (TUYA_FILE file);
    INT_T               (*fileno)                   (TUYA_FILE file);
    INT_T               (*fsync)                    (INT_T fd);
    INT_T               (*faccess)                  (CONST CHAR_T *filepath,IN INT_T mode);
    INT_T               (*fgetsize)                 (CONST CHAR_T *filepath);
    INT_T               (*fgetc)                    (TUYA_FILE file);
    INT_T               (*ftruncate)                (INT_T fd, UINT64_T length);
} TKL_FS_T;

extern CONST TKL_FS_T TKL_FS;

/**
 * @brief get file system description
 *
 * @return TKL_FS instance
 */
TKL_FS_T* tkl_fs_desc_get(VOID_T);
//#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_INIT_SYSTEM_H__


