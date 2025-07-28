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

/* Cortex M33 allows breakpoints to be set to a specific program address without
 * manually adding breakpoint instructions inside the program code;
 * armstar can support 8 fpb breakpoint.
 * ATTENTION:armstar/armv8-m does not support flash patch feature;
 */
uint32_t fpb_set_program_breakpoint(uint32_t program_addr);

// eof

