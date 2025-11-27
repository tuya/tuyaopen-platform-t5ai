// Copyright 2025-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef _VFS_STREAM_H_
#define _VFS_STREAM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   VFS Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    audio_stream_type_t     type;           /*!< Stream type */
    int                     buf_sz;         /*!< Audio Element Buffer size */
    int                     out_block_size; /*!< Size of output block */
    int                     out_block_num;  /*!< Number of output block */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} vfs_stream_cfg_t;


#define VFS_STREAM_BUF_SIZE            (2 * 1024)
#define VFS_STREAM_TASK_STACK          (3072)
#define VFS_STREAM_TASK_CORE           (0)
#define VFS_STREAM_TASK_PRIO           (4)
#define VFS_STREAM_OUT_BLOCK_SIZE      (2 * 1024)
#define VFS_STREAM_OUT_BLOCK_NUM       (1)


#define DEFAULT_VFS_STREAM_CONFIG() {             \
    .type = AUDIO_STREAM_NONE,                    \
    .buf_sz = VFS_STREAM_BUF_SIZE,                \
    .out_block_size = VFS_STREAM_OUT_BLOCK_SIZE,  \
    .out_block_num = VFS_STREAM_OUT_BLOCK_NUM,    \
    .task_stack = VFS_STREAM_TASK_STACK,          \
    .task_core = VFS_STREAM_TASK_CORE,            \
    .task_prio = VFS_STREAM_TASK_PRIO,            \
}

/**
 * @brief      Create a handle to an Audio Element to stream data from vfs file system to another Element
 *             or get data from other elements written to vfs file system, depending on the configuration
 *             the stream type, either AUDIO_STREAM_READER or AUDIO_STREAM_WRITER.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t vfs_stream_init(vfs_stream_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
