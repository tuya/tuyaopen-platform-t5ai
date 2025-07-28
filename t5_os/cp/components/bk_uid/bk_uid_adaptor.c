// Copyright 2020-2021 Beken
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

#include "os/os.h"
#include "bk_uid_adaptor.h"
#include <modules/uidlib.h>

#define UID_OSI_VERSION              0x00000001

static const struct uid_osi_funcs_t uid_osi_funcs = {
    ._version = UID_OSI_VERSION,
    .size = sizeof(struct uid_osi_funcs_t), 
    /* log */
    ._log = bk_printf_ext,
};

bk_err_t bk_uid_adaptor_init(void)
{
    bk_err_t ret = BK_OK;

    if (uid_osi_funcs_init((void*)&uid_osi_funcs) != 0)
    {
        return BK_FAIL;
    }

    return ret;
}

