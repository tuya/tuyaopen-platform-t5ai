#pragma once

#include <driver/psram_types.h>

inline uint32_t common_mod(uint32_t val, uint32_t div)
{
	BK_ASSERT(div);
	return ((val) % (div));
}

#define COMMON_MOD(val, div) common_mod(val, div)

struct fb_block_free
{
	/// Used to check if memory block has been corrupted or not
	uint32_t corrupt_check;
	/// Size of the current free block (including delimiter)
	uint32_t free_size;
	/// Next free block pointer
	struct fb_block_free* next;
	/// Previous free block pointer
	struct fb_block_free* previous;
};

/// Used memory block delimiter structure (size must be word multiple)
typedef struct
{
	/// Used to check if memory block has been corrupted or not
	uint32_t corrupt_check;
	/// Size of the current used block (including delimiter)
	uint32_t size;
} fb_block_used;

typedef struct
{
	/// Root pointer = pointer to first element of heap linked lists
	struct fb_block_free * heap[PSRAM_HEAP_MAX];
	/// Size of heaps
	uint32_t heap_size[PSRAM_HEAP_MAX];
} fb_mem_heap_t;

// init psram mem to different heap block
void bk_psram_frame_buffer_init(void);

void *bk_psram_frame_buffer_malloc(psram_heap_type_t type, uint32_t size);

void bk_psram_frame_buffer_free(void* mem_ptr);

