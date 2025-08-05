#pragma once

#include <stdint.h>
#include <limits.h>
#include <stddef.h>

#define AV_INPUT_BUFFER_PADDING_SIZE 64
#define HAVE_FAST_64BIT 0

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

#define av_assert2(...)

typedef struct GetBitContext
{
    const uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
    int size_in_bits_plus8;
} GetBitContext;

union unaligned_32 { uint32_t l; } __attribute__((packed)) av_alias;


#define AV_BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define AV_BSWAP32C(x) (AV_BSWAP16C(x) << 16 | AV_BSWAP16C((x) >> 16))
#define AV_BSWAP64C(x) (AV_BSWAP32C(x) << 32 | AV_BSWAP32C((x) >> 32))

#define AV_BSWAPC(s, x) AV_BSWAP##s##C(x)

#ifndef av_bswap16
static inline const uint16_t av_bswap16(uint16_t x)
{
    x = (x >> 8) | (x << 8);
    return x;
}

#endif

#ifndef av_bswap32
static inline const uint32_t av_bswap32(uint32_t x)
{
    return AV_BSWAP32C(x);
}

#endif

static inline int init_get_bits(GetBitContext *s, const uint8_t *buffer,
                                int bit_size)
{
    int buffer_size;
    int ret = 0;

    if (bit_size >= INT_MAX - FFMAX(7, AV_INPUT_BUFFER_PADDING_SIZE * 8) || bit_size < 0 || !buffer)
    {
        bit_size    = 0;
        buffer      = NULL;
        ret         = -1;

        return ret;
    }

    buffer_size = (bit_size + 7) >> 3;

    s->buffer             = buffer;
    s->size_in_bits       = bit_size;
    s->size_in_bits_plus8 = bit_size + 8;
    s->buffer_end         = buffer + buffer_size;
    s->index              = 0;

    return ret;
}

static inline int init_get_bits8(GetBitContext *s, const uint8_t *buffer,
                                 int byte_size)
{
    if (byte_size > INT_MAX / 8 || byte_size < 0)
    {
        byte_size = -1;
    }

    return init_get_bits(s, buffer, byte_size * 8);
}


static inline int get_bits_count(const GetBitContext *s)
{
    return s->index;
}

static inline int get_bits_left(GetBitContext *gb)
{
    return gb->size_in_bits - get_bits_count(gb);
}

static inline unsigned int get_bits(GetBitContext *s, int n)
{
    register unsigned int tmp;
    unsigned int re_index = (s)->index;
    unsigned int __attribute__((unused)) re_cache;

    re_cache = av_bswap32((((const union unaligned_32 *) (((s))->buffer + (re_index >> 3)))->l)) << (re_index & 7) >> (32 - 32);
    tmp = (((uint32_t)(re_cache)) >> (32 - (n)));
    re_index += (n);
    (s)->index = re_index;

    return tmp;
}

static inline unsigned int get_bits_long(GetBitContext *s, int n)
{
    av_assert2(n >= 0 && n <= 32);

    if (!n)
    {
        return 0;
    }
    else if (!HAVE_FAST_64BIT
             //|| av_builtin_constant_p(n <= MIN_CACHE_BITS)) && n <= MIN_CACHE_BITS
            )
    {
        return get_bits(s, n);
    }
    else
    {
#if HAVE_FAST_64BIT
        unsigned tmp;
        OPEN_READER(re, s);
        UPDATE_CACHE_32(re, s);
        tmp = SHOW_UBITS(re, s, n);
        LAST_SKIP_BITS(re, s, n);
        CLOSE_READER(re, s);
        return tmp;
#else
#ifdef BITSTREAM_READER_LE
        unsigned ret = get_bits(s, 16);
        return ret | (get_bits(s, n - 16) << 16);
#else
        unsigned ret = get_bits(s, 16) << (n - 16);
        return ret | get_bits(s, n - 16);
#endif
#endif
    }
}

static inline void skip_bits(GetBitContext *s, int n)
{
    s->index += n;
}

static inline void skip_bits_long(GetBitContext *s, int n)
{
    s->index += n;
}
