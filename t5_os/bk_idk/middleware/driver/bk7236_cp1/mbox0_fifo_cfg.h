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

#pragma once

#define MBOX0_CHN0_FIFO_LEN     2
#define MBOX0_CHN1_FIFO_LEN     4
#define MBOX0_CHN2_FIFO_LEN     2

#define MBOX0_CHN0_FIFO_START     0
#define MBOX0_CHN1_FIFO_START     (MBOX0_CHN0_FIFO_START + MBOX0_CHN0_FIFO_LEN)
#define MBOX0_CHN2_FIFO_START     (MBOX0_CHN1_FIFO_START + MBOX0_CHN1_FIFO_LEN)

//struct mbox0_fifo_cfg_t:
#define MBOX0_FIFO_CFG_TABLE   \
{   \
	{MBOX0_CHN0_FIFO_START, MBOX0_CHN0_FIFO_LEN}, \
	{MBOX0_CHN1_FIFO_START, MBOX0_CHN1_FIFO_LEN}, \
	{MBOX0_CHN2_FIFO_START, MBOX0_CHN2_FIFO_LEN}, \
}

