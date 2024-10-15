/*
 * Simple XZ decoder command line tool
 *
 * Author: Lasse Collin <lasse.collin@tukaani.org>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

/*
 * This is really limited: Not all filters from .xz format are supported,
 * only CRC32 is supported as the integrity check, and decoding of
 * concatenated .xz streams is not supported. Thus, you may want to look
 * at xzdec from XZ Utils if a few KiB bigger tool is not a problem.
 */
#ifndef _BSPATCH_H_
#define _BSPATCH_H_

#include <stdint.h>
#include "bsdiff_flash.h"

typedef struct bspatch_stream
{
	void* opaque;
	int (*read)(const struct bspatch_stream* stream, void* buffer, int length, void* arg);
} bspatch_stream_t;

int bspatch_core(uint32_t new_bin_addr, uint32_t new_size, 
                 uint32_t old_bin_addr, uint32_t old_size, 
                 uint32_t mode, uint32_t step, uint32_t buf_size, 
                 bspatch_stream_t* patch, bspatch_stream_t* patch1, 
                 bsdiff_mag_t *mag,
                 void* arg);

#endif