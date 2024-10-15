#ifndef ZEPHYR_INCLUDE_Z_FFS_H_
#define ZEPHYR_INCLUDE_Z_FFS_H_

static inline unsigned int find_msb_set(uint32_t op)
{
	if (op == 0) {
		return 0;
	}

	return 32 - __builtin_clz(op);
}

static inline unsigned int find_lsb_set(uint32_t op)
{
	return __builtin_ffs(op);
}

#endif
