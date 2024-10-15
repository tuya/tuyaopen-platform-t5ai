
#ifndef _bk3000_CHIP_H_
#define _bk3000_CHIP_H_

#include "bl_config.h"
#define ENABLE   1
#define DISABLE  0




// AMBA Address Mapping
/////////////// CHANGE /////////////////
// AHB
#if (CHIP_BK3281)
#define BK3000_PMU_BASE_ADDR              0x00800000
#define BK3000_FLASH_BASE_ADDR            0x00801000

#define BK3000_ICU_BASE_ADDR              0x00F00000
#define BK3000_UART_BASE_ADDR             0x00F08000

#define BK3000_SPI_BASE_ADDR              0x00F38000
#define BK3000_GPIO_BASE_ADDR             0x00F40000
#define BK3000_WDT_BASE_ADDR              0x00F48000

#define REG_XVR_BASE_ADDR              	  0x00800000
#define ANA_XVR_NUM 16

#define BK3000_PMU_ROM_FLASH_SECT        				(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0xb*4)))


//watchdog

#define BK3000_WDT_CONFIG                    		(*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+0*4)))
// PMU define
#define BK3000_PMU_CONFIG          						  (*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0*4)))
#define BK3000_PMU_PERI_PWDS        						(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 1*4)))
#define BK3000_PMU_PERINT_ENABLE        				(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 4*4)))




#define BK3000_PMU_INTC_ENABLE        					(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 6*4)))
#define BK3000_PMU_RESET_REASON_GET          		(*((volatile unsigned int *)(BK3000_GPIO_BASE_ADDR + 0x3f*4)))
#define DEEP_SLEEP_RESTART		(0x1 << 0)


#define bit_PMU_UART_PWD                  (1<<1)
#define bit_PMU_SPI_PWD                   (1<<7)
#define bit_PMU_WDT_PWD                   (1<<9)
#define bit_PMU_ICU_PWD                   (1<<20)
#define bit_PMU_FLASH_PWD                 (1<<21)

#endif

#if (CHIP_BK3266)
#define BK3000_PMU_BASE_ADDR              0x00800000
#define BK3000_FLASH_BASE_ADDR            0x00801000

#define BK3000_ICU_BASE_ADDR              0x00F10000
#define BK3000_UART_BASE_ADDR             0x00F08000

#define BK3000_SPI_BASE_ADDR              0x00F38000
#define BK3000_GPIO_BASE_ADDR             0x00F40000
#define BK3000_WDT_BASE_ADDR              0x00F48000

#define REG_XVR_BASE_ADDR                 0x00F00000
#define ANA_XVR_NUM 16

#define BK3000_PMU_ROM_FLASH_SECT        				(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x7*4)))

//watchdog

#define BK3000_WDT_CONFIG                    		(*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+0*4)))
// PMU define
#define BK3000_PMU_CONFIG          						  (*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0*4)))
#define BK3000_PMU_PERI_PWDS        						(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 1*4)))
#define BK3000_PMU_PERINT_ENABLE        				(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 4*4)))
#define BK3000_PMU_RESET_REASON_GET          		(*((volatile unsigned int *)(BK3000_GPIO_BASE_ADDR + 0x3f*4)))
#define DEEP_SLEEP_RESTART		(0x1 << 0)




#define sft_PMU_CPU_CLK_DIV               1
#define sft_PMU_CPU_CLK_SEL               8
#define PMU_AHB_CLK_ALWAYS_ON             (1<<10)

#define bit_PMU_UART_PWD                  (1<<2)

#define bit_PMU_SPI_PWD                   (1<<8)

#define bit_PMU_LPO_CLK_DISABLE           (1<<23)
#define bit_PMU_LPO_CLK_STEAL_ENABLE      (1<<24)
#define bit_PMU_LPO_CLK_SEL_XTAL          (1<<25)
#define bit_PMU_CLK_FREQ_SEL              (1<<31)

#endif


#if (CHIP_BK3288)
#define BK3000_SYS_BASE_ADDR              0x01000000
#define BK3000_UART_BASE_ADDR             0x01808000
//#define BK3000_FLASH_BASE_ADDR            0x00801000
#define BK3000_SPI_BASE_ADDR              0x01820000
#define BK3000_WDT_BASE_ADDR              0x01890000
#define BK3000_GPIO_BASE_ADDR             0x01000c00
#define REG_XVR_BASE_ADDR                 0x01910000
#define ANA_XVR_NUM 16
#define BK3000_PMU_ROM_FLASH_SECT        				(*((volatile unsigned int *)(BK3000_SYS_BASE_ADDR + 0xe*4)))
#define BK3000_PMU_BASE_ADDR             0x01000800

//watchdog
#define BK3000_WDT_CONFIG                    		(*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+0*4)))
// PMU define
#define BK3000_PMU_RESET_REASON_GET          		(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x1*4)))
#define DEEP_SLEEP_RESTART		(0x1 << 0)



//Peri clock pwd
#define addSYSTEM_Reg0x5                                        *((volatile unsigned long *) (0x01000000+0x5*4))
#define UART0CLK_PWD                (0x1U << 0 )
#define SPI0CLK_PWD                 (0x1U << 3 )
//#define WDTCLK_PWD                  (0x1U << 18)
#define SET_PERIxCLK(bit)           addSYSTEM_Reg0x5 &= ~bit
#define CLR_PERIxCLK(bit)           addSYSTEM_Reg0x5 |= bit

//addSYSTEM_Reg0x9
#define addSYSTEM_Reg0x9                                        *((volatile unsigned long *) (0x01000000+0x9*4))
#define INT_ID_UART0   (0x1U << 22 )
#define VIC_UART_ISR_INDEX            22


#define addSYSTEM_Reg0x1d                                        *((volatile unsigned long *) (0x01000000+0x1d*4))
#define set_spi_rw_flash                                   addSYSTEM_Reg0x1d  |= 0x4
#define set_flashctrl_rw_flash                             addSYSTEM_Reg0x1d  &= ~0x4

#define addSYSTEM_Reg0x21                                        *((volatile unsigned long *) (0x01000000+0x21*4))
#define clr_ram_sd(bit)           addSYSTEM_Reg0x21  &= ~bit
#define set_ram_sd(bit)            addSYSTEM_Reg0x21 |= bit
#define uart0_ram_sd   (0x1U << 0 )
#define spi0_ram_sd    (0x1U << 3 )



#endif


#if (CHIP_BK3633)

#define BK3000_SYS_BASE_ADDR              0x00800000
#define BK3000_UART_BASE_ADDR             0x00806300
//#define BK3000_FLASH_BASE_ADDR            0x00801000
#define BK3000_SPI_BASE_ADDR              0x00806200
#define BK3000_WDT_BASE_ADDR               0x00806000
#define BK3000_AWDT_BASE_ADDR              0x00800c00
#define BK3000_GPIO_BASE_ADDR             0x00800a00
#define REG_XVR_BASE_ADDR                 0x00806e00
#define ANA_XVR_NUM 16
#define BK3000_PMU_BASE_ADDR              0x00800800

//watchdog
#define BK3000_WDT_CONFIG                    		(*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+0*4)))

#define AON_WDT_FEED                    		(*((volatile unsigned long *)(BK3000_AWDT_BASE_ADDR+1*4)))
#define AON_WDT_PERIOD                  		(*((volatile unsigned long *)(BK3000_AWDT_BASE_ADDR+2*4)))


// PMU define
#define BK3000_PMU_RESET_REASON_GET          		(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x1*4)))
#define DEEP_SLEEP_RESTART_BIT		(0x1 << 0)
// 1: Boot from deep_sleep  0: Boot from POR 
#define DEEP_SLEEP_RESTART   ((BK3000_PMU_RESET_REASON_GET & DEEP_SLEEP_RESTART_BIT ) != 0)

//uart enable
#define addSYSTEM_Reg0x3          *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x3*4))
#define UART0CLK_PWD_BIT          (0x1U << 0 )
#define UART0_ENABLE    (addSYSTEM_Reg0x3) &= ~UART0CLK_PWD_BIT 
#define UART0_DISABLE   (addSYSTEM_Reg0x3) |=  UART0CLK_PWD_BIT  

//spi enable
//#define addSYSTEM_Reg0x5            *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x5*4))
#define SPI0CLK_PWD_BIT             (0x1U << 13 )	
#define SPI0_ENABLE    (addSYSTEM_Reg0x3) &= ~SPI0CLK_PWD_BIT 
#define SPI0_DISABLE   (addSYSTEM_Reg0x3) |=  SPI0CLK_PWD_BIT  

//uart int enable
 
//#define addSYSTEM_Reg0x11       (*((volatile unsigned int *)(BK3000_SYS_BASE_ADDR + 0x11*4)))
//#define IRQ_BIT           (0x01<< 0)
//#define IRQ_ENABLE       addSYSTEM_Reg0x11 |= IRQ_BIT
//#define IRQ_DISABLE      addSYSTEM_Reg0x11 &= ~IRQ_BIT

#define INT_STATUS   (*((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x12*4)))
#define UART0_INT_INDEX         (0x1U << 4 )	
#define UART0_INT_STATUS   (INT_STATUS & UART0_INT_INDEX )
#define IRQ_INT_CLEAR      INT_STATUS = INT_STATUS

#define PER_INT_ENABLE   (*((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x10*4)))
#define UART0_INT_ENABLE   PER_INT_ENABLE |=  UART0_INT_INDEX
#define UART0_INT_DISABLE  PER_INT_ENABLE &=  ~UART0_INT_INDEX

// flash_rw_sel & rebooot
#define addSYSTEM_Reg0xf    (*((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0xf*4)))
#define  SPI_FLASH_SEL      (0x1U << 1 )	 
#define  SET_SPI_RW_FLASH          addSYSTEM_Reg0xf |= SPI_FLASH_SEL 
#define  SET_FLASHCTRL_RW_FLASH    addSYSTEM_Reg0xf &= ~SPI_FLASH_SEL 

#define  BOOT_BIT      (0x1U << 0 )	
#define  REBOOT        addSYSTEM_Reg0xf &= ~BOOT_BIT  




#define UART_CLOCK_FREQ      16000000


#endif




#if (CHIP_BK7256)

#define BK3000_SYS_BASE_ADDR              0x44010000
#define BK3000_UART_BASE_ADDR             0x44820000
#define BK3000_SPI_BASE_ADDR              0x44860000
#define BK3000_WDT_BASE_ADDR               0x44800000
#define BK3000_GPIO_BASE_ADDR             0x44000400
#define BK3000_AWDT_BASE_ADDR             0x44000600
#define REG_XVR_BASE_ADDR                 0x4a800000
#define ANA_XVR_NUM                       16
#define BK3000_PMU_BASE_ADDR              0x44000000

//watchdog
#define BK3000_WDT_CONFIG                    		(*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+0*4)))

#define AON_WDT_CONFIG                     		(*((volatile unsigned long *)(BK3000_AWDT_BASE_ADDR+0*4)))
//#define AON_WDT_FEED                    		(*((volatile unsigned long *)(BK3000_AWDT_BASE_ADDR+1*4)))
//#define AON_WDT_PERIOD                  		(*((volatile unsigned long *)(BK3000_AWDT_BASE_ADDR+2*4)))


// PMU define
#define BK3000_PMU_RESET_REASON_GET          		(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x3*4)))
#define DEEP_SLEEP_RESTART_BIT		(0x1 << 0)
// 1: Boot from deep_sleep  0: Boot from POR
#define DEEP_SLEEP_RESTART   ((BK3000_PMU_RESET_REASON_GET & DEEP_SLEEP_RESTART_BIT ) != 0)


//uart enable _ok
#define addSYSTEM_Reg0xc                            *((volatile unsigned long *) (0x44010000+0xc*4))
#define UART0_ENABLE                                addSYSTEM_Reg0xc |= 0x4
#define UART0_DISABLE                               addSYSTEM_Reg0xc &= ~0x4

//spi enable _ok
#define SPI0_ENABLE                            addSYSTEM_Reg0xc |= 0x2
#define SPI0_DISABLE                            addSYSTEM_Reg0xc &= ~0x2


//uart int enable

//#define addSYSTEM_Reg0x11       (*((volatile unsigned int *)(BK3000_SYS_BASE_ADDR + 0x11*4)))
//#define IRQ_BIT           (0x01<< 0)
//#define IRQ_ENABLE       addSYSTEM_Reg0x11 |= IRQ_BIT
//#define IRQ_DISABLE      addSYSTEM_Reg0x11 &= ~IRQ_BIT

//#define INT_STATUS   (*((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x12*4)))
//#define UART0_INT_INDEX         (0x1U << 4 )
//#define UART0_INT_STATUS   (INT_STATUS & UART0_INT_INDEX )
//#define IRQ_INT_CLEAR      INT_STATUS = INT_STATUS

//#define PER_INT_ENABLE   (*((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x10*4)))
//#define UART0_INT_ENABLE   PER_INT_ENABLE |=  UART0_INT_INDEX
//#define UART0_INT_DISABLE  PER_INT_ENABLE &=  ~UART0_INT_INDEX

////////////////////ok


#define PER_INT_ENABLE   *((volatile unsigned long *) (0x44010000+0x20*4))
#define UART0_INT_INDEX_NUM          4
#define UART0_INT_INDEX         (0x1U << 4 )
#define UART0_INT_ENABLE   PER_INT_ENABLE |=  UART0_INT_INDEX
#define UART0_INT_DISABLE  PER_INT_ENABLE &=  ~UART0_INT_INDEX

/////////////////

// flash_rw_sel & rebooot   ok


#define addSYSTEM_Reg0x3                                        *((volatile unsigned long *) (0x44010000+0x3*4))
#define REBOOT                                                  addSYSTEM_Reg0x3 &= ~0x1
#define  SPI_FLASH_SEL             (0x1U << 9 )
#define  SET_SPI_RW_FLASH          addSYSTEM_Reg0x3 |= SPI_FLASH_SEL
#define  SET_FLASHCTRL_RW_FLASH    addSYSTEM_Reg0x3 &= ~SPI_FLASH_SEL


#define UART_CLOCK_FREQ      26000000

#endif




#if (CHIP_BK7236)

#define BK3000_SYS_BASE_ADDR              0x44010000
#define BK3000_UART_BASE_ADDR             0x44820000
#define BK3000_SPI_BASE_ADDR              0x44870000
#define BK3000_WDT_BASE_ADDR              0x44800000
#define BK3000_GPIO_BASE_ADDR             0x44000400
#define BK3000_AWDT_BASE_ADDR             0x44000600
#define REG_XVR_BASE_ADDR                 0x4a800000
#define ANA_XVR_NUM                       16
#define BK3000_PMU_BASE_ADDR              0x44000000

//watchdog
#define BK3000_WDT_CONTROL                   (*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+2*4)))
#define BK3000_WDT_CONFIG                   (*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+4*4)))

#define AON_WDT_CONFIG                      (*((volatile unsigned long *)(BK3000_AWDT_BASE_ADDR+0*4)))
//#define AON_WDT_FEED                      (*((volatile unsigned long *)(BK3000_AWDT_BASE_ADDR+1*4)))
//#define AON_WDT_PERIOD                    (*((volatile unsigned long *)(BK3000_AWDT_BASE_ADDR+2*4)))


// PMU define

#define BK3000_PMU_RESET_REASON_GET          		(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x0*4)))

#define AON_PMU_REG01          				(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x1*4)))
#define AON_PMU_REG01_BIT_XTAL_MODE                     (0x1 << 21)
#define AON_PMU_IS_XTAL_40M                             ((AON_PMU_REG01 & AON_PMU_REG01_BIT_XTAL_MODE) == 1)
#define AON_PMU_REG41          				(*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x41*4)))
#define AON_PMU_REG41_BIT_XTAL_SEL                      (0x1 << 14)
#define AON_PMU_SET_XTAL_26M                            AON_PMU_REG41 &= ~AON_PMU_REG41_BIT_XTAL_SEL
#define AON_PMU_SET_XTAL_40M                            AON_PMU_REG41 |= AON_PMU_REG41_BIT_XTAL_SEL
#define AON_PMU_IS_XTAL_SEL_40M                         ((AON_PMU_REG41 & AON_PMU_REG41_BIT_XTAL_SEL) == 1)

#define AON_PMU_REG03            		            (*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x3*4)))

#define AON_PMU_REG03_BIT_BOOT_FLAG                 0x1
#define AON_PMU_REG03_SET_BF_PRIMARY                AON_PMU_REG03 |= AON_PMU_REG03_BIT_BOOT_FLAG
#define AON_PMU_REG03_CLR_BF_PRIMARY                AON_PMU_REG03 &= ~AON_PMU_REG03_BIT_BOOT_FLAG

#define DEEP_SLEEP_RESTART_BIT		(0x1 << 1)
// 1: Boot from deep_sleep  0: Boot from POR
#define DEEP_SLEEP_RESTART   ((BK3000_PMU_RESET_REASON_GET & DEEP_SLEEP_RESTART_BIT ) != 0)


//uart enable _ok
#define addSYSTEM_Reg0xc                            *((volatile unsigned long *) (0x44010000+0xc*4))
#define UART0_ENABLE                                addSYSTEM_Reg0xc |= 0x4
#define UART0_DISABLE                               addSYSTEM_Reg0xc &= ~0x4
#define UART0_RESET                                 REG_APB3_UART_RST = 1


//spi enable _ok
#define SPI0_ENABLE                            addSYSTEM_Reg0xc |= 0x2
#define SPI0_DISABLE                            addSYSTEM_Reg0xc &= ~0x2

#define PER_INT_ENABLE   *((volatile unsigned long *) (0x44010000+0x20*4))
#define UART0_INT_INDEX_NUM          4
#define UART0_INT_INDEX         (0x1U << 4 )
#define UART0_INT_ENABLE   PER_INT_ENABLE |=  UART0_INT_INDEX
#define UART0_INT_DISABLE  PER_INT_ENABLE &=  ~UART0_INT_INDEX

/////////////////

// flash_rw_sel & rebooot   ok


#define addSYSTEM_Reg0x2                                        *((volatile unsigned long *) (0x44010000+0x2*4))
#define REBOOT                                                  addSYSTEM_Reg0x2 &= ~0x1
#define  SPI_FLASH_SEL             (0x1U << 9 )
#define  SET_SPI_RW_FLASH          addSYSTEM_Reg0x2 |= SPI_FLASH_SEL
#define  SET_FLASHCTRL_RW_FLASH    addSYSTEM_Reg0x2 &= ~SPI_FLASH_SEL


uint32_t uart_get_clock_freq(void);
#define UART_CLOCK_FREQ      uart_get_clock_freq()
#define UART_CLOCK_FREQ_26M  26000000
#define UART_CLOCK_FREQ_40M  40000000

#endif

////////////////// CHANGE /////////////////////

//////////////////////////////////////
// Pherial Register's Define  

//GPIO
#define BK3000_GPIO_0_CONFIG                 (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+0*4)))
#define BK3000_GPIO_1_CONFIG                 (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+1*4)))

#define BK3000_GPIO_6_CONFIG                 (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+6*4)))
#define BK3000_GPIO_7_CONFIG                 (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+7*4)))
#define BK3000_GPIO_8_CONFIG                 (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+8*4)))
#define BK3000_GPIO_9_CONFIG                 (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+9*4)))

#define BK3000_GPIO_10_CONFIG                (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+10*4)))
#define BK3000_GPIO_11_CONFIG                (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+11*4)))

#define sft_GPIO_INPUT                      0
#define sft_GPIO_OUTPUT                     1
#define sft_GPIO_INPUT_ENABLE               2
#define sft_GPIO_OUTPUT_ENABLE              3
#define sft_GPIO_PULL_MODE                  4
#define sft_GPIO_PULL_ENABLE                5

#define sft_GPIO_FUNCTION_ENABLE            6

#define sft_GPIO_INPUT_MONITOR              7

 /* defination of registers
*/
#define REG_APB3_UART_RST                   (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x02*4)))
#define REG_APB3_UART_CFG                   (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x04*4)))
#define REG_APB3_UART_FIFO_THRESHOLD        (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x05*4)))
#define REG_APB3_UART_FIFO_STATUS           (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x06*4)))
#define REG_APB3_UART_DATA                  (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x07*4)))
#define REG_APB3_UART_INT_ENABLE            (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x08*4)))
#define REG_APB3_UART_INT_FLAG              (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x09*4)))
#define REG_APB3_UART_FC_CFG                (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x0A*4)))
#define REG_APB3_UART_WAKEUP_ENABLE         (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x0B*4)))

#define UART_WRITE_BYTE(v) (REG_APB3_UART_DATA=v)
#define UART_READ_BYTE()  ((REG_APB3_UART_DATA>>8)&0xff)

#define bit_UART_TX_FIFO_FULL                (1<<16)

#define bit_UART_TX_FIFO_EMPTY               (1<<17)
#define bit_UART_RX_FIFO_FULL                (1<<18)
#define bit_UART_RX_FIFO_EMPTY               (1<<19)
#define bit_UART_TX_WRITE_READY              (1<<20)
#define bit_UART_RX_READ_READY               (1<<21)

#define sft_UART_CONF_TX_ENABLE          0
#define sft_UART_CONF_RX_ENABLE          1
#define sft_UART_CONF_IRDA               2
#define sft_UART_CONF_UART_LEN           3
#define sft_UART_CONF_PAR_EN             5
#define sft_UART_CONF_PAR_MODE           6
#define sft_UART_CONF_STOP_LEN           7
#define sft_UART_CONF_CLK_DIVID          8

#define sft_UART_FIFO_CONF_TX_FIFO       0
#define sft_UART_FIFO_CONF_RX_FIFO       8
#define sft_UART_FIFO_CONF_DETECT_TIME   16


#define bit_UART_INT_TX_NEED_WRITE       ( 1 << 0 )
#define bit_UART_INT_RX_NEED_READ        ( 1 << 1 )
#define bit_UART_INT_RX_OVER_FLOW        ( 1 << 2 )
#define bit_UART_INT_RX_PAR_ERR          ( 1 << 3 )
#define bit_UART_INT_RX_STOP_ERR         ( 1 << 4 )
#define bit_UART_INT_TX_STOP_END         ( 1 << 5 )
#define bit_UART_INT_RX_STOP_END         ( 1 << 6 )
#define bit_UART_INT_RXD_WAKEUP          ( 1 << 7 )





#define UART_TX_FIFO_SIZE       128
#define UART_RX_FIFO_SIZE       128
#define UART_TX_FIFO_COUNT      (REG_APB3_UART_FIFO_STATUS&0xff)
#define UART_RX_FIFO_COUNT      ((REG_APB3_UART_FIFO_STATUS>>8)&0xff)
#define UART_TX_FIFO_FULL        (REG_APB3_UART_FIFO_STATUS&0x00010000)
#define UART_TX_FIFO_EMPTY       (REG_APB3_UART_FIFO_STATUS&0x00020000)
#define UART_RX_FIFO_FULL        (REG_APB3_UART_FIFO_STATUS&0x00040000)
#define UART_RX_FIFO_EMPTY       (REG_APB3_UART_FIFO_STATUS&0x00080000)
#define UART_TX_WRITE_READY      (REG_APB3_UART_FIFO_STATUS&0x00100000)
#define UART_RX_READ_READY       (REG_APB3_UART_FIFO_STATUS&0x00200000)

#define UART_BAUDRATE_3250000   3250000
#define UART_BAUDRATE_3000000   3000000
#define UART_BAUDRATE_2000000   2000000
#define UART_BAUDRATE_921600    921600  //
#define UART_BAUDRATE_460800    460800
#define UART_BAUDRATE_256000    256000
#define UART_BAUDRATE_230400    230400  //
#define UART_BAUDRATE_115200    115200  //default
#define UART_BAUDRATE_9600      9600  
#define UART_BAUDRATE_3000      3250  //

#define UART_BAUDRATE_DEFAULE    UART_BAUDRATE_115200 //UART_BAUDRATE_3000000
#define UART_CLOCK_FREQ_26M  26000000
#define UART_CLOCK_FREQ_48M  48000000
#define UART_CLOCK_FREQ_24M  24000000
#define UART_CLOCK_FREQ_52M  52000000



#define Beken_Write_Register(addr, data) (*((volatile u_int32 *)(addr))) = (u_int32)(data);
#define Beken_Read_Register(addr) (*(volatile u_int32 *)(addr));


#ifdef FLASH_DEBUG
//////////////////////////////////////
//FLASH
#define BIT_ADDRESS_SW                 0
#define BIT_OP_TYPE_SW                 24
#define BIT_OP_SW                      29
#define BIT_WP_VALUE                   30
#define BIT_BUSY_SW                    31

#define SET_ADDRESS_SW                 (0xFFFFFF << BIT_ADDRESS_SW)
#define SET_OP_TYPE_SW                 (0x1F     << BIT_OP_TYPE_SW)
#define SET_OP_SW                      (0x1      << BIT_OP_SW)
#define SET_WP_VALUE                   (0x1      << BIT_WP_VALUE)
#define SET_BUSY_SW                    (0x1      << BIT_BUSY_SW)

#define BIT_FLASH_CLK_CONF             0
#define BIT_MODE_SEL                   4
#define BIT_FWREN_FLASH_CPU            9
#define BIT_WRSR_DATA                  10
#define BIT_CRC_EN                     26

#define SET_FLASH_CLK_CONF             (0xF      << BIT_FLASH_CLK_CONF)
#define SET_MODE_SEL                   (0x1F     << BIT_MODE_SEL)
#define SET_FWREN_FLASH_CPU            (0x1      << BIT_FWREN_FLASH_CPU)
#define SET_WRSR_DATA                  (0xFFFF   << BIT_WRSR_DATA)
#define SET_CRC_EN                     (0x1      << BIT_CRC_EN)

#define BIT_SR_DATA_FLASH              0
#define BIT_CRC_ERR_COUNTER            8
#define BIT_DATA_FLASH_SW_SEL          16
#define BIT_DATA_SW_FLASH_SEL          19

#define SET_SR_DATA_FLASH              (0xFF     << BIT_SR_DATA_FLASH)
#define SET_CRC_ERR_COUNTER            (0xFF     << BIT_CRC_ERR_COUNTER)
#define SET_DATA_FLASH_SW_SEL          (0x7      << BIT_DATA_FLASH_SW_SEL)
#define SET_DATA_SW_FLASH_SEL          (0x7      << BIT_DATA_SW_FLASH_SEL)

#define reg_FLASH_OPERATE_SW           (*((volatile unsigned long *)    (BK3000_FLASH_BASE_ADDR+0*4)))
#define reg_FLASH_DATA_SW_FLASH        (*((volatile unsigned long *)    (BK3000_FLASH_BASE_ADDR+1*4)))
#define reg_FLASH_DATA_FLASH_SW        (*((volatile unsigned long *)    (BK3000_FLASH_BASE_ADDR+2*4)))
#define reg_FLASH_RDID_DATA_FLASH      (*((volatile unsigned long *)    (BK3000_FLASH_BASE_ADDR+4*4)))
#define reg_FLASH_SR_DATA_CRC_CNT      (*((volatile unsigned long *)    (BK3000_FLASH_BASE_ADDR+5*4)))
#define reg_FLASH_CONF                 (*((volatile unsigned long *)    (BK3000_FLASH_BASE_ADDR+7*4)))

#endif //

#endif
// end of file


