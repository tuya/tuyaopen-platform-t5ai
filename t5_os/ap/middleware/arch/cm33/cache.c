// Copyright 2023-2028 Beken
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

#include <stdint.h>
#include "armstar.h"
#include "cache.h"
#include "mpu.h"
#include "sdkconfig.h"

void sram_dcache_map(void)
{
}

int show_cache_config_info(void)
{
	return 0;
}

/* Redmine #8131: only the PSRAM task-stack heap (Write-Back) - and, when
 * CONFIG_SRAM_CACHE_ENABLE is set, the shared SRAM - are mapped cacheable in the
 * MPU. The broad flush/invalidate call sites (DMA, mailbox, IPC) also pass
 * non-cacheable SRAM/device/PSRAM-heap addresses; restrict the maintenance to
 * addresses that actually back cache lines so those calls are safe no-ops. */
__attribute__((section(".iram"))) static int dcache_range_cacheable(const void *va, long size)
{
    uint32_t start;
    uint32_t end;

    if ((va == (void *)0) || (size <= 0)) {
        return 0;
    }
    start = (uint32_t)va;
    end = start + (uint32_t)size;

    /* Write-Back cacheable PSRAM task-stack heap. */
    {
        uint32_t sh = (uint32_t)CONFIG_AP_PSRAM_STACK_HEAP_ADDR;
        uint32_t she = sh + (uint32_t)CONFIG_AP_PSRAM_STACK_HEAP_SIZE;
        if ((start < she) && (end > sh)) {
            return 1;
        }
    }

#if CONFIG_SRAM_CACHE_ENABLE
    /* Shared SRAM is cacheable only when explicitly enabled. */
    if ((start < 0x28100000UL) && (end > 0x28000000UL)) {
        return 1;
    }
#endif

    return 0;
}

__attribute__((section(".iram"))) void flush_dcache(void *va, long size)
{
    if (!dcache_range_cacheable(va, size)) {
        return;
    }
    if (SCB->CLIDR & SCB_CLIDR_DC_Msk) {
        SCB_CleanDCache_by_Addr(va, size);
        __DSB();
        SCB_InvalidateDCache_by_Addr(va, size);
    }
}

/* Discard stale consumer-side lines without writing them back over the
 * producer's latest physical-memory contents. */
__attribute__((section(".iram"))) void invalidate_dcache(void *va, long size)
{
    if (!dcache_range_cacheable(va, size)) {
        return;
    }
    if (SCB->CLIDR & SCB_CLIDR_DC_Msk) {
        SCB_InvalidateDCache_by_Addr(va, size);
    }
}

void flush_all_dcache(void)
{
    if (SCB->CLIDR & SCB_CLIDR_DC_Msk) {
        SCB_CleanInvalidateDCache();
    }
}

void enable_dcache(int enable)
{
    if (enable == 0) {
        SCB_DisableDCache();
    } else {
        SCB_EnableDCache();
    }
}

void invalidate_icache(void)
{
    SCB_InvalidateICache();
}
// eof

