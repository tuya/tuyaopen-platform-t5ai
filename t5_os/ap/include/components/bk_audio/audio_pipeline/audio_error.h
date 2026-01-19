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

#define BK_ERR_ADK_BASE                        0x80000

#define BK_ERR_ADK_NO_ERROR                    BK_OK
#define BK_ERR_ADK_FAIL                        BK_FAIL

#define BK_ERR_ADK_UNKNOWN                     BK_ERR_ADK_BASE - 0
#define BK_ERR_ADK_ALREADY_EXISTS              BK_ERR_ADK_BASE - 1
#define BK_ERR_ADK_MEMORY_LACK                 BK_ERR_ADK_BASE - 2
#define BK_ERR_ADK_INVALID_URI                 BK_ERR_ADK_BASE - 3
#define BK_ERR_ADK_INVALID_PATH                BK_ERR_ADK_BASE - 4
#define BK_ERR_ADK_INVALID_PARAMETER           BK_ERR_ADK_BASE - 5
#define BK_ERR_ADK_NOT_READY                   BK_ERR_ADK_BASE - 6
#define BK_ERR_ADK_NOT_SUPPORT                 BK_ERR_ADK_BASE - 7
#define BK_ERR_ADK_NOT_FOUND                   BK_ERR_ADK_BASE - 8
#define BK_ERR_ADK_TIMEOUT                     BK_ERR_ADK_BASE - 9
#define BK_ERR_ADK_INITIALIZED                 BK_ERR_ADK_BASE - 10
#define BK_ERR_ADK_UNINITIALIZED               BK_ERR_ADK_BASE - 11
#define BK_ERR_ADK_INVALID_ARG                 BK_ERR_ADK_BASE - 12
#define BK_ERR_ADK_NO_MEM                      BK_ERR_ADK_BASE - 13
#define BK_ERR_ADK_INVALID_STATE               BK_ERR_ADK_BASE - 14




#define AUDIO_CHECK(TAG, a, action, msg) if (!(a)) {                                       \
        BK_LOGE(TAG,"%s:%d (%s): %s\n", __FILENAME__, __LINE__, __FUNCTION__, msg);       \
        action;                                                                   \
    }

#define AUDIO_MEM_CHECK(TAG, a, action)  AUDIO_CHECK(TAG, a, action, "Memory exhausted")

#define AUDIO_NULL_CHECK(TAG, a, action) AUDIO_CHECK(TAG, a, action, "Got NULL Pointer")

#define AUDIO_ERROR(TAG, str) BK_LOGE(TAG, "%s:%d (%s): %s", __FILENAME__, __LINE__, __FUNCTION__, str)

#ifdef __cplusplus
}
#endif

#endif
