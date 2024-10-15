#include "hal_misc.h"
#include "soc.h"
#include "pal_log.h"
#include "hal_platform.h"

/*
 * CPU     FLASH     1ms loop cnt   1ms loop + one time reg read
 * -----  --------  -------------- -----------------------------
 * 26M    26M        3000           1700
 * 120M   60M        10000          5300
 * */

void hal_loop_delay(uint32_t loop_cnt)
{
	volatile uint32_t loop = 0;
	while(loop++ < loop_cnt);
}

//TODO fix me, change to armino way after
static void SetFlashWire1()
{
    uint32_t        rdata;

    //CRMR
    rdata  = addFLASH_Reg0xa;
    rdata &= ~(bitFLASH_Reg0xa_mode_sel | bitFLASH_Reg0xa_fwren_flash_cpu | bitFLASH_Reg0xa_wrsr_data);
    rdata |= (0x8   << posFLASH_Reg0xa_mode_sel         ) |
             (0x0   << posFLASH_Reg0xa_fwren_flash_cpu  ) |
             (0x0   << posFLASH_Reg0xa_wrsr_data        ) ;
    addFLASH_Reg0xa = rdata;

    rdata  = addFLASH_Reg0x15;
    rdata |= (0x16 << posFLASH_Reg0x15_op_type_sw );
    addFLASH_Reg0x15 = rdata;

    rdata  = addFLASH_Reg0x4;
    rdata &= bitFLASH_Reg0x4_wp_value;
    rdata |= (0x1     << posFLASH_Reg0x4_op_sw ) ;
    addFLASH_Reg0x4 = rdata;

    while(addFLASH_Reg0x4 & bitFLASH_Reg0x4_busy_sw);

    #if 0
    rdata  = addFLASH_Reg0xa;
    rdata &= ~(bitFLASH_Reg0xa_mode_sel | bitFLASH_Reg0xa_fwren_flash_cpu | bitFLASH_Reg0xa_wrsr_data);
    rdata |= (0x0   << posFLASH_Reg0xa_mode_sel         ) |
             (0x0   << posFLASH_Reg0xa_fwren_flash_cpu  ) |
             (0x0   << posFLASH_Reg0xa_wrsr_data        ) ;
    addFLASH_Reg0xa = rdata;

    rdata  = addFLASH_Reg0x4;
    rdata &= bitFLASH_Reg0x4_wp_value;
    rdata |= (0x7     << posFLASH_Reg0x4_op_type_sw ) |
             (0x1     << posFLASH_Reg0x4_op_sw      ) ;
    addFLASH_Reg0x4 = rdata;

    while(addFLASH_Reg0x4 & bitFLASH_Reg0x4_busy_sw);
    #endif
}

#define ANA_REG(id) (0x44010000 + ((0x40 + id) << 2))

// Wait for maximum 60ms, CPU 26M/Flash 26M, 1ms loops about 1700 times
// 1700 * 6 = 10200
#define MAX_LOOP_CNT 102000
static inline void ana_wait_finish(uint32_t id)
{
	volatile uint32_t loop_cnt = 0;

#if CONFIG_MPW0
	while((REG_READ(0x44010000 + (0xb << 2)) & (1 << (8 + (id)))) && (loop_cnt++ < MAX_LOOP_CNT)); only for MPW0
#else
	while((REG_READ(0x44010000 + (0x3a << 2)) & (1 << (id))) && (loop_cnt++ < MAX_LOOP_CNT));
#endif
}

static inline void ana_reg_write(uint32_t id, uint32_t v)
{
	REG_WRITE(ANA_REG(id), v);
	ana_wait_finish(id);
}

static inline uint32_t ana_reg_read(uint32_t id)
{
	return REG_READ(ANA_REG(id));
}

static inline void ana_reg_or(uint32_t id, uint32_t data)
{
	REG_OR(ANA_REG(id), data);
	ana_wait_finish(id);
}

static inline void ana_reg_and(uint32_t id, uint32_t data)
{
	REG_OR(ANA_REG(id), data);
	ana_wait_finish(id);
}

/* From Wangjian, method to enable 7236 DPLL:
  * ana_reg5 set bit5
  * ana_reg0 set to 0x81185B57
  * ana_reg0 clear bit19
  * ana_reg0 set bit19
  */
void hal_enable_pll(void)
{
	ana_reg_or(5, 0x20);
	ana_reg_write(0, 0x81185B57);
	ana_reg_and(0, 0xfff7ffff);
	ana_reg_or(0, 0x00080000);
}

void hal_enable_high_freq(void)
{
	addSYSTEM_Reg0x8 &= ~0x03F ;     // sel 480M PLL ; div4
	addSYSTEM_Reg0x8 |= 0x033 ;     // sel 480M PLL ; div4
	hal_set_flash_clock(CLOCK_PLL_60M);
}

void hal_disable_pll(void)
{
}

void hal_set_flash_clock(flash_clock_t clock)	////speed: 0-XTAL 26MHz, 1-PLL 60MHz
{
	SET_FLASHCTRL_RW_FLASH;

	SetFlashWire1();

	if(clock == CLOCK_PLL_60M)
	{
		// Configure flash to 120M
		set_SYSTEM_Reg0x9_ckdiv_flash(0x1);// flash clk div2
		set_SYSTEM_Reg0x9_cksel_flash(0x2);// sel 120M
	}
	else
	{
		set_SYSTEM_Reg0x9_cksel_flash(00); //flash div =0 (26M )
		set_SYSTEM_Reg0x9_ckdiv_flash(00);// flash clk div1
	}
}

void dump_sys_regs(void)
{
#if CONFIG_FPGA
	PAL_LOG_INFO("flash 0xa=%x\r\n", REG_READ(0x44030000 + (0xa << 2)));
	PAL_LOG_INFO("sys r8=%x\r\n", REG_READ(0x44010000 + (0x8 << 2)));
	PAL_LOG_INFO("sys r9=%x\r\n", REG_READ(0x44010000 + (0x9 << 2)));
	PAL_LOG_INFO("sys ra=%x\r\n", REG_READ(0x44010000 + (0xa << 2)));
	PAL_LOG_INFO("sys r10=%x\r\n", REG_READ(0x44010000 + (0x10 << 2)));

	PAL_LOG_INFO("sys clkena 0xc=%x\r\n", REG_READ(0x44010000 + (0xc << 2)));
	PAL_LOG_INFO("ana 0x0=%x\r\n", REG_READ(0x44010000 + (0x40 << 2)));
	PAL_LOG_INFO("ana 0x1=%x\r\n", REG_READ(0x44010000 + (0x41 << 2)));
	PAL_LOG_INFO("ana 0x2=%x\r\n", REG_READ(0x44010000 + (0x42 << 2)));
	PAL_LOG_INFO("ana 0x3=%x\r\n", REG_READ(0x44010000 + (0x43 << 2)));
	PAL_LOG_INFO("ana 0x4=%x\r\n", REG_READ(0x44010000 + (0x44 << 2)));
	PAL_LOG_INFO("ana 0x5=%x\r\n", REG_READ(0x44010000 + (0x45 << 2)));
	PAL_LOG_INFO("ana 0x8=%x\r\n", REG_READ(0x44010000 + (0x48 << 2)));
	PAL_LOG_INFO("ana 0x9=%x\r\n", REG_READ(0x44010000 + (0x49 << 2)));
	PAL_LOG_INFO("ana 0xa=%x\r\n", REG_READ(0x44010000 + (0x4a << 2)));
	PAL_LOG_INFO("ana 0xb=%x\r\n", REG_READ(0x44010000 + (0x4b << 2)));
	PAL_LOG_INFO("ana 0xc=%x\r\n", REG_READ(0x44010000 + (0x4c << 2)));
	PAL_LOG_INFO("ana 0xd=%x\r\n", REG_READ(0x44010000 + (0x4d << 2)));
#else
	PAL_LOG_INFO("tag=%x\r\n", REG_READ(CORTEX_M33_REG_DAUTHCTRL));
	PAL_LOG_INFO("sys r8=%x\r\n", REG_READ(0x44010000 + (0x8 << 2)));
	PAL_LOG_INFO("sys r9=%x\r\n", REG_READ(0x44010000 + (0x9 << 2)));
	PAL_LOG_INFO("sys ra=%x\r\n", REG_READ(0x44010000 + (0xa << 2)));
	PAL_LOG_INFO("ana r0=%x\r\n", REG_READ(0x44010000 + (0x40 << 2)));
	PAL_LOG_INFO("ana r5=%x\r\n", REG_READ(0x44010000 + (0x45 << 2)));
#endif
}
