// Copyright 2022-2023 Beken
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

#ifndef _AUDIO_ERROR_H_
#define _AUDIO_ERROR_H_

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define BK_ERR_ADF_BASE                        0x80000   /*!< Starting number of BK-ADF error codes */

#define BK_ERR_ADF_NO_ERROR                    BK_OK
#define BK_ERR_ADF_NO_FAIL                     BK_FAIL

#define BK_ERR_ADF_UNKNOWN                     BK_ERR_ADF_BASE + 0
#define BK_ERR_ADF_ALREADY_EXISTS              BK_ERR_ADF_BASE + 1
#define BK_ERR_ADF_MEMORY_LACK                 BK_ERR_ADF_BASE + 2
#define BK_ERR_ADF_INVALID_URI                 BK_ERR_ADF_BASE + 3
#define BK_ERR_ADF_INVALID_PATH                BK_ERR_ADF_BASE + 4
#define BK_ERR_ADF_INVALID_PARAMETER           BK_ERR_ADF_BASE + 5
#define BK_ERR_ADF_NOT_READY                   BK_ERR_ADF_BASE + 6
#define BK_ERR_ADF_NOT_SUPPORT                 BK_ERR_ADF_BASE + 7
#define BK_ERR_ADF_NOT_FOUND                   BK_ERR_ADF_BASE + 8
#define BK_ERR_ADF_TIMEOUT                     BK_ERR_ADF_BASE + 9
#define BK_ERR_ADF_INITIALIZED                 BK_ERR_ADF_BASE + 10
#define BK_ERR_ADF_UNINITIALIZED               BK_ERR_ADF_BASE + 11


#define BK_ERR_ADF_INVALID_ARG                 BK_ERR_ADF_BASE + 12
#define BK_ERR_ADF_NO_MEM                      BK_ERR_ADF_BASE + 13
#define BK_ERR_ADF_INVALID_STATE               BK_ERR_ADF_BASE + 14




#define AUDIO_CHECK(TAG, a, action, msg) if (!(a)) {                                       \
        BK_LOGE(TAG,"%s:%d (%s): %s", __FILENAME__, __LINE__, __FUNCTION__, msg);       \
        action;                                                                   \
        }

#define AUDIO_MEM_CHECK(TAG, a, action)  AUDIO_CHECK(TAG, a, action, "Memory exhausted")

#define AUDIO_NULL_CHECK(TAG, a, action) AUDIO_CHECK(TAG, a, action, "Got NULL Pointer")

#define AUDIO_ERROR(TAG, str) BK_LOGE(TAG, "%s:%d (%s): %s", __FILENAME__, __LINE__, __FUNCTION__, str)

#define BK_EXISTS   (BK_OK + 1)

#ifdef __cplusplus
}
#endif

#endif
