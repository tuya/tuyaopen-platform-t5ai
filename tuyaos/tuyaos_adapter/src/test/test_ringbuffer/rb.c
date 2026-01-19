/****************************************************************************
 * @file rb.c
 * @brief Used to ringbuffer
 * @version 0.1
 * @date 2021-08-09
 *
 * @copyright Copyright (c) tuya.inc 2021
 ****************************************************************************/


/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "rb.h"
#include <stdint.h>
#include <string.h>
#include "tkl_memory.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define min(a, b) ((a) < (b)? (a): (b))
#define max(a, b) ((a) > (b)? (a): (b))

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef __DYNAMIC_MALLOC__
static inline uint32_t roundup_pow_of_two(uint32_t len)
{
    int i, mask;

    if ((len & (len - 1)) == 0)
        return len;

    for (i = 31; i >= 0; i--) {
        mask = 1 << i;
        if ((len & mask) == mask)
            break;
    }

    return (mask << 1);
}
#endif // __DYNAMIC_MALLOC__


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tkl_rb_init
 *
 * Description:
 *      initialize
 *
 * Input Parameters:
 *   r      - xx
 *   len    - yy
 *   esize  - zz
 *
 * Returned Value:
 *   The positive non-zero number of bytes read on success, 0 on if an
 *   end-of-file condition, or a negated errno value on any failure.
 *
 ****************************************************************************/

int tkl_rb_init(struct ringbuffer *r, uint32_t len, uint32_t esize)
{
    if (r == NULL || len == 0 || esize == 0)
        return -1;

    r->esize = esize;
    r->in = r->out = 0;

#ifdef __DYNAMIC_MALLOC__
    r->size = roundup_pow_of_two(esize * len);
    r->data = (uint8_t *)tkl_system_psram_malloc(r->size);
    if (r->data == NULL)
        return -1;
#else // !__DYNAMIC_MALLOC__
    r->size = RB_BUF_LEN / esize;
    // r->size must be 2^n, for example, 128, 256, 512, 1024...
    if ((r->size & (r->size - 1)) != 0)
        return -1;
#endif // !__DYNAMIC_MALLOC__

    r->mask = r->size - 1;

    return 0;
}

void tkl_rb_deinit(struct ringbuffer *r)
{
    if (r == NULL)
        return;

    r->in = r->out = 0;
#ifdef __DYNAMIC_MALLOC__
    if (r->data != NULL)
        tkl_system_psram_free(r->data);
    r->data = NULL;
#endif // __DYNAMIC_MALLOC__
}

void tkl_rb_clean(struct ringbuffer *r)
{
    if (r == NULL)
        return;

    r->in = r->out = 0;
}

uint32_t tkl_rb_size(struct ringbuffer *r)
{
    return (r->size);
}

uint32_t tkl_rb_in(struct ringbuffer *r, const uint8_t *buf, uint32_t len)
{
    uint32_t l;
    uint32_t left = tkl_rb_unused(r);

    len = min(len, left);

    l = min(len, r->size - (r->in & r->mask));

    memcpy(r->data + (r->in & r->mask), buf, l);
    memcpy(r->data, buf + l, len - l);

    /* lock? */
    r->in += len;
    return len;
}

uint32_t tkl_rb_out(struct ringbuffer *r, void *buf, uint32_t len)
{
    uint32_t l;
    uint32_t avail = tkl_rb_avail(r);

    len = min(len, avail);

    l = min(len, r->size - (r->out & r->mask));

    memcpy(buf, r->data + (r->out & r->mask), l);
    memcpy(buf + l, r->data, len -l);

    /* lock? */
    r->out += len;
    return len;
}

/* space used in ringbuffer */
uint32_t tkl_rb_avail(struct ringbuffer *r)
{
    return (r->in - r->out);
}

/* space left in ringbuffer */
uint32_t tkl_rb_unused(struct ringbuffer *r)
{
    return (r->size - (r->in - r->out));
}

uint32_t tkl_rb_is_empty(struct ringbuffer *r)
{
    return (r->in == r->out);
}

uint32_t tkl_rb_is_full(struct ringbuffer *r)
{
    return (tkl_rb_avail(r) > r->mask);
}

void tkl_rb_peek(struct ringbuffer *r, void *buf, uint32_t len)
{
    uint32_t l;
    uint32_t avail = tkl_rb_avail(r);

    len = min(len, avail);

    l = min(len, tkl_rb_size(r) - (r->out & r->mask));

    memcpy(buf, r->data + (r->out & r->mask), l);
    memcpy(buf + l, r->data, len -l);
}


#if defined __TEST_ON_LINUX__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_LEN         RB_BUF_LEN
#define TEST_TIME       0x7FFFFFFF
#define BACK_OFF_TIME   50

static struct ringbuffer rb;
static uint32_t statistics_in = 0;
static uint32_t statistics_out = 0;
static uint8_t check_error = 0;

// #define TRACE   printf
#define TRACE(...)

#pragma pack(push)
#pragma pack(1)
struct head {
    uint16_t sync;
    uint16_t len;
    uint32_t crc;
};
struct dataflow {
    struct head head;
    uint8_t  buf[BUF_LEN];
};
#pragma pack(pop)

static uint32_t check_sum(unsigned char *buffer, unsigned int size)
{
    unsigned int sum = 0;

    if (buffer == NULL || size == 0)
        return 0;

    for (int i = 0; i < size; i++)
        sum += buffer[i];

    return sum;
}

static void *phtread_in(void *arg)
{
    int i;
    volatile uint32_t nwrite, len, w_ofs;
    struct ringbuffer *r = (struct ringbuffer *)arg;
    struct dataflow df_in;

    df_in.head.sync = 0xaa55;

    while (1) {
        TRACE("-- %s %d\n", __func__, __LINE__);
        if ((check_error != 0) || tkl_rb_is_full(r)) {
            TRACE("-- %s %d\n", __func__, __LINE__);
            continue;
        }

        /* prepare data */
        memset(df_in.buf, 0, BUF_LEN);
        srand(time(NULL));
        df_in.head.len = (rand() % (BUF_LEN / 2)) + BUF_LEN / 2;
        for (i = 0; i < df_in.head.len; i++) {
            df_in.buf[i] = rand() % 0xf0 + 1;
        }
        //df_in.head.crc = crc32(df_in.buf, df_in.head.len, 0xff);
        df_in.head.crc = check_sum(df_in.buf, df_in.head.len);

        /* put data to ring buffer */
        nwrite = 0;
        w_ofs = 0;
        len = sizeof(struct head) + df_in.head.len;
        do {
            nwrite = tkl_rb_in(r, (uint8_t *)&df_in + w_ofs, len);
            if (nwrite == 0 || nwrite < len) {
                /* full, wait 500us */
                while (tkl_rb_is_full(r)) {
                    TRACE("-- %s %d\n", __func__, __LINE__);
                    usleep(BACK_OFF_TIME);
                }
            }
            /* write the remaining bytes into ringbuffer */
            len -= nwrite;
            w_ofs += nwrite;
        } while(len != 0);
        statistics_in += df_in.head.len;
    }

    return NULL;
}

static void *phtread_out(void *arg)
{
    int i;
    uint8_t sync;
    uint8_t sync_buf[BUF_LEN + 8];
    uint32_t nread, crc, avail_bytes;
    struct dataflow df_out;
    struct ringbuffer *r = (struct ringbuffer *)arg;

    while (1) {
        TRACE("-- %s %d\n", __func__, __LINE__);
        if (tkl_rb_avail(r) < sizeof(struct head)) {
            TRACE("-- %s %d\n", __func__, __LINE__);
            usleep(BACK_OFF_TIME);
        } else {
            TRACE("-- %s %d\n", __func__, __LINE__);
            memset(&sync_buf, 0, sizeof(sync_buf));

            avail_bytes = tkl_rb_avail(r);
            tkl_rb_peek(r, sync_buf, avail_bytes);
            for (i = 0; i < avail_bytes - 1; i++) {
                if (sync_buf[i] == 0x55 && \
                    sync_buf[i + 1] == 0xaa) {
                    break;
                }
            }
            if (i == (avail_bytes - 1)) {
                // discard
                printf("discard 1, len %d\n", avail_bytes);
                tkl_rb_out(r, sync_buf, avail_bytes);
                continue;
            }
            if (i != 0) {
                printf("discard 2, len %d\n", i);
                tkl_rb_out(r, sync_buf, i);
            }

            memset(&df_out, 0, sizeof(df_out));

            /* get head */
            while(tkl_rb_avail(r) < sizeof(struct head));
            nread = tkl_rb_out(r, (uint8_t *)&df_out.head, sizeof(struct head));
            if (nread != sizeof(struct head)) {
                printf("read head error, actually %d, expected %ld\n",
                        nread, sizeof(struct head));
            }

            while(tkl_rb_avail(r) < df_out.head.len);
            nread = tkl_rb_out(r, df_out.buf, df_out.head.len);

            /* crc check */
            //crc = crc32(df_out.buf, df_out.head.len, 0xff);
            crc = check_sum(df_out.buf, df_out.head.len);
            if (crc != df_out.head.crc) {
                check_error += 1;
            }
            statistics_out += df_out.head.len;
        }
    }

    printf("out thread exit\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    uint32_t cnt = 0;
    pthread_t pid_in;
    pthread_t pid_out;

    tkl_rb_init(&rb, 256, 1);
    printf("ring buffer size: %d\n", rb.size);
    usleep(100 * 1000);

    pthread_create(&pid_in, NULL, phtread_in, (void *)&rb);
    pthread_create(&pid_out, NULL, phtread_out, (void *)&rb);

    while (1) {
        cnt++;
        printf("time: %dh-%dm-%ds, in: %d, out: %d, speed: ibps %dk, obps %dk, error %d\n",
                cnt / 3600, (cnt % 3600) / 60, cnt % 60,
                statistics_in, statistics_out,
                (statistics_in / cnt) / 1024,
                (statistics_out / cnt) / 1024,
                check_error);

        if ((check_error != 0) || (cnt == TEST_TIME)) {
            printf("crc error cnt: %d, cnt: %d\n", check_error, cnt);
            break;
        }
        sleep(1);
    }

    printf("Cancel thread, %d\n", tkl_rb_avail(&rb));
    pthread_cancel(pid_in);
    pthread_cancel(pid_out);


    pthread_join(pid_in, NULL);
    pthread_join(pid_out, NULL);

    printf("threads has exited\n");

    sleep(1);
    tkl_rb_deinit(&rb);
    printf("Exit\n");
    return -1;
}
#endif // __TEST_ON_LINUX__

