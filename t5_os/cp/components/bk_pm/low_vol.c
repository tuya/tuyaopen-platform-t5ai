// Copyright 2020-2023 Beken
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

#include <modules/pm.h>

/*=====================DEFINE  SECTION  START=====================*/

/*=====================DEFINE  SECTION  END=======================*/

/*=====================VARIABLE  SECTION  START===================*/
static volatile  uint64_t s_pm_module_lv_sleep_state               = 0;


/*=====================VARIABLE  SECTION  END=====================*/

/*================FUNCTION DECLARATION  SECTION  START============*/


/*================FUNCTION DECLARATION  SECTION  END===============*/
uint64_t bk_pm_module_lv_sleep_state_get(pm_dev_id_e module)
{
	return s_pm_module_lv_sleep_state & (0x1ULL << module);
}
__attribute__((section(".itcm_sec_code"))) bk_err_t bk_pm_module_lv_sleep_state_set()
{
	s_pm_module_lv_sleep_state = 0xFFFFFFFFFFFFFFFF;
	return BK_OK;
}
bk_err_t bk_pm_module_lv_sleep_state_clear(pm_dev_id_e module)
{
	s_pm_module_lv_sleep_state &= ~(0x1ULL << module);
	return BK_OK;
}

bk_err_t pm_debug_lv_state()
{
	BK_LOGI(NULL,"pm_module_lv_sleep_state:0x%llx\r\n",s_pm_module_lv_sleep_state);
	return BK_OK;
}