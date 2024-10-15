#ifndef ZEPHYR_INCLUDE_Z_GCC_H_
#define ZEPHYR_INCLUDE_Z_GCC_H_

#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif

#ifndef ARG_UNUSED
#define ARG_UNUSED(x) (void)(x)
#endif

#define popcount(x) __builtin_popcount(x)

#define CODE_UNREACHABLE __builtin_unreachable()

#endif
