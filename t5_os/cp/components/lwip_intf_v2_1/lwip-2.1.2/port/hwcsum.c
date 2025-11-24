#include <sdkconfig.h>
#include <soc/soc.h>

#ifdef CONFIG_LWIP_HW_IP_CHECKSUM
#if CONFIG_BK_HW_IP_CHECKSUM
#ifdef CONFIG_SPE
#define IPCHKSUM_BASE                       (0x4980DE00)
#else
#define IPCHKSUM_BASE                       (0x5980DE00)
#endif

/* ip checksum base address */
#define IPCHKSUM_CHECKSUM_BASE_ADDR         (IPCHKSUM_BASE + 0*4)

/* ip checksum length */
#define IPCHKSUM_REG0X1_ADDR                (IPCHKSUM_BASE + 1*4)
#define IPCHKSUM_CHECKSUM_LENGTH_POSI       (0)
#define IPCHKSUM_CHECKSUM_LENGTH_MASK       (0xFFFF)

/* ip checksum start/enable */
#define IPCHKSUM_REG0X2_ADDR                (IPCHKSUM_BASE + 2*4)
#define IPCHKSUM_CHECKSUM_START             (1 << 0)

/* ip checksum data valid */
#define IPCHKSUM_REG0X3_ADDR                (IPCHKSUM_BASE + 3*4)
#define IPCHKSUM_IP_CHECKSUM_VALID          (1 << 0)

/* ip checksum data */
#define IPCHKSUM_REG0X4_ADDR                (IPCHKSUM_BASE + 4*4)
#define IPCHKSUM_IP_CHECKSUM_POSI           (0)
#define IPCHKSUM_IP_CHECKSUM_MASK           (0xFFFF)

/* ip checksum interrupt status/write 1 clear */
#define IPCHKSUM_REG0X5_ADDR                (IPCHKSUM_BASE + 5*4)
#define IPCHKSUM_CHECKSUM_INT_STATUS        (1 << 0)

static void ipchksum_active(bool enable)
{
    REG_WRITE(IPCHKSUM_REG0X2_ADDR, enable);
}

static void ipchksum_set_base_addr(uint32_t val)
{
    REG_WRITE(IPCHKSUM_CHECKSUM_BASE_ADDR, val);
}

static void ipchksum_set_length(uint16_t val)
{
    REG_WRITE(IPCHKSUM_REG0X1_ADDR, val);
}

static bool ipchksum_get_valid_flag(void)
{
    return (REG_READ(IPCHKSUM_REG0X3_ADDR) & 1);
}

static uint16_t ipchksum_get_data(void)
{
    return (REG_READ(IPCHKSUM_REG0X4_ADDR) & IPCHKSUM_IP_CHECKSUM_MASK);
}

static void ipchksum_clr_int(bool enable)
{
    if (enable)
        REG_WRITE(IPCHKSUM_REG0X5_ADDR, 1);
}

static void ipchksum_start(uint32_t addr, uint16_t len)
{
    ipchksum_set_base_addr(addr);
    ipchksum_set_length(len);

    ipchksum_active(1);
}

/*
 * calculate ip checksum.
 * @addr: buf address, aligned address allowed
 * @len:  buf lenght
 *
 * return ip checksum. Mutex may be used to avoid concurrent access.
 */
uint16_t ipchksum_get_result(uint32_t addr, uint16_t len)
{
    ipchksum_start(addr, len);
    while (!ipchksum_get_valid_flag());
    ipchksum_clr_int(1);
    return ipchksum_get_data();
}
#endif // CONFIG_BK_HW_IP_CHECKSUM

static bool hw_ipcsum_holded = false;
extern uint16_t lwip_standard_chksum(const void *dataptr, int len);
bool hsu_ip_checksum(uint32_t addr, uint16_t len, uint16_t *checksum);

/*
 * hw ip checksum verison of lwip_standard_chksum.
 * TODO: use mutex or spinlock instead of disable interrupt in SMP enviroment.
 */
uint16_t hw_ipcksum_standard_chksum(const void *dataptr, int len)
{
    GLOBAL_INT_DECLARATION();
    uint16_t csum;

    GLOBAL_INT_DISABLE();
    if (hw_ipcsum_holded) {
        GLOBAL_INT_RESTORE();
        goto sw;
    }

    GLOBAL_INT_RESTORE();
    hw_ipcsum_holded = true;
#if CONFIG_BK_HW_IP_CHECKSUM
    csum = ipchksum_get_result((uint32_t)dataptr, (uint16_t)len);
#else
    hsu_ip_checksum((uint32_t)dataptr, (uint16_t)len, &csum);
#endif
    hw_ipcsum_holded = false;

    return csum;

sw:
    // LWIP_LOGV("use lwip csum\n");
    csum = lwip_standard_chksum(dataptr, len);
    return csum;
}

#if CONFIG_IPCHKSUM_UNIT_TEST
static uint8_t csum_buf[4096];
uint16_t ipchksum_get_result(uint32_t addr, uint16_t len);
uint16_t lwip_standard_chksum(const void *dataptr, int len);

// return true if unit test passed, else false.
bool ipchksum_unit_test()
{
    uint16_t hw_csum, sw_csum;

    // init csum buf
    for (int i = 0; i < sizeof(csum_buf); i++) {
        csum_buf[i] = i & 0xFF;
    }

    // caculate csum by hw
    GPIO_UP(6);
    hw_csum = hw_ipcksum_standard_chksum(csum_buf, 4096);
    GPIO_DOWN(6);

    // caculate csum by sw
    GPIO_UP(7);
    sw_csum = lwip_standard_chksum(csum_buf, 4096);
    GPIO_DOWN(7);
    BK_LOGD(NULL, "1: csum: %x/%x\n", hw_csum, sw_csum);
    if (hw_csum != sw_csum)
        return false;

    // caculate csum by hw, addr not aligned
    GPIO_UP(6);
    hw_csum = hw_ipcksum_standard_chksum(&csum_buf[1], 4095);
    GPIO_DOWN(6);

    // caculate csum by sw, addr not aligned
    GPIO_UP(7);
    sw_csum = lwip_standard_chksum(&csum_buf[1], 4095);
    GPIO_DOWN(7);
    BK_LOGD(NULL, "2: csum: %x/%x\n", hw_csum, sw_csum);
    if (hw_csum != sw_csum)
        return false;

    // caculate csum by hw, addr not aligned, network length
    GPIO_UP(6);
    hw_csum = hw_ipcksum_standard_chksum(&csum_buf[1], 1460);
    GPIO_DOWN(6);

    // caculate csum by sw, addr not aligned, network length
    GPIO_UP(7);
    sw_csum = lwip_standard_chksum(&csum_buf[1], 1460);
    GPIO_DOWN(7);
    BK_LOGD(NULL, "3: csum: %x/%x\n", hw_csum, sw_csum);

    if (hw_csum != sw_csum)
        return false;

    return true;
}
#endif // CONFIG_IPCHKSUM_UNIT_TEST
#endif // CONFIG_LWIP_HW_IP_CHECKSUM

