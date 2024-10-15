/*
 * BK_HCI_protocol.h
 *
 *  Created on: 2017-5-8
 *      Author: gang.cheng
 */

#ifndef BK_HCI_PROTOCOL_H_
#define BK_HCI_PROTOCOL_H_

#include "type.h"
#include "bl_bk_reg.h"





typedef enum        // by gwf
{
    OK = 0,
    ERROR = -1
} STATUS;

typedef enum        // by gwf
{
    NO = 0,
    YES = 1
} ASK;

enum
{
	FLASH_OPERATE_CONTINUE,
	FLASH_OPERATE_END,
	FLASH_OPERATE_INVALID
};

typedef enum boot_flag_t {
    BOOT_FLAG_INVALID  	= 0,
    BOOT_FLAG_PRIMARY  	= 1,
    BOOT_FLAG_SECONDARY = 2,
}BOOT_FLAG;

typedef enum
{
    FLASH_PROTECT_NONE,
    FLASH_PROTECT_ALL,
} PROTECT_TYPE;


#define HCI_EVENT_HEAD_LENGTH       0x03
#define HCI_COMMAND_HEAD_LENGTH     0x04


/*
 * HCI transport type bytes
 */
/*
enum {
    TRA_HCIT_COMMAND = 1,
    TRA_HCIT_ACLDATA = 2,
    TRA_HCIT_SCODATA = 3,
    TRA_HCIT_EVENT   = 4
};
*/

#define VENDOR_SPECIFIC_DEBUG_OGF   0x3F
#define BEKEN_OCF                   0xE0
#define HCI_COMMAND_COMPLETE_EVENT  0x0E

#define BOOT_FLAG_CTRL_MAGIC        (0x4C725463)


enum {
    LINK_CHECK_CMD              = 0x00,
    REGISTER_WRITE_CMD          = 0x01,
    BL2_LINK_CHECK_CMD          = 0x02,
    REGISTER_READ_CMD           = 0x03,

    CMD_ERROR_EVENT				= 0X04,
    SYS_REBOOT_CMD     			 = 0X0E,

    CHANGE_UART_SAMPRATE_CMD      = 0X0F,

    FLASH_IMAGE_CEC_CHECK		 = 0x10,

    FLASH_CBUS_DOWNLOAD         = 0x11,
    FLASH_CBUS_END              = 0x12,

    SYS_RESET_CMD		 = 0x70,
    // Bluetooth Tx/Rx Test
    TX_PN9_CMD                  = 0xA0,
    RX_PN9_CMD                  = 0xA1,
    CEVA_BR_SEND_CMD            = 0xA2,
    CEVA_BR_RECV_CMD            = 0xA3,
    BLE_ADVERTISER_CMD          = 0xA4,
    CEVA_FHS_SEND_CMD           = 0xA5,

    FALSH_STAY_ROM_CMD           = 0xAA,

    // Beken Digital Test
    TEST_CMD_CHIP_ID            = 0xB0,
    TEST_CMD_RF_TXRX            = 0xB1,
    TEST_CMD_DEEPSLEEP          = 0xB2,
    TEST_CMD_SUBDEEPSLEEP       = 0xB3,
    TEST_CMD_DIGITAL_OP         = 0xB4,
    TEST_CMD_DMIPS              = 0xB5,
    TEST_CMD_BR_TXRX            = 0xB6,
    TEST_CMD_LE_TXRX            = 0xB7,
    TEST_CMD_BK24_TXRX          = 0xB8,
    TEST_CMD_LPO_BT_LINK        = 0xB9,
    TEST_CMD_ADC                = 0xBA,
    TEST_CMD_FLASH_OP           = 0xBB,
    TEST_CMD_SPI                = 0xBC,
    TEST_CMD_I2C                = 0xBD,
    TEST_CMD_WDT                = 0xBE,
    TEST_CMD_3DS                = 0xBF,

    LOOP_MODE_CMD               = 0xCC,

    SYS_ERROR_RSP               = 0xEE,
    SYS_UNDEFINE_INT               = 0xEF,

    FLASH_OPERATE_CMD   	     = 0XF4,

} ;//BEKEN_CMD;

enum {
	FLASH_ADDR_WRITE_CMD          = 0X06,

   FLASH_4K_WRITE_CMD          = 0X07,

   FLASH_ADDR_READ_CMD           = 0X08,

   FLASH_4K_READ_CMD           = 0X09,

   FLASH_CHIP_ERRASE_CMD       = 0X0A,

   FLASH_4K_ERRASE_CMD       = 0X0B,

   FLASH_SR_READ_CMD       = 0x0c,

   FLASH_SR_WRITE_CMD      = 0x0D,

   FLASH_SPI_OP_CMD      = 0x0E,


   FLASH_SIZE_ERRASE_CMD       = 0X0F,






};// FLASH_operate


#define FLASH_4K_SIZE   0X1000
#define FLASH_32K_SIZE   0X8000
#define FLASH_64K_SIZE   0X10000

#define __PACKED_POST__  __attribute__((packed))

typedef struct {
    u8 code;             /**< 0x01: HCI Command Packet
                              0x02: HCI ACL Data Packet
                              0x03: HCI Synchronous Data Packet
                              0x04: HCI Event Packet */
    struct {
        u16 ocf : 10;    /**< OpCode Command Field */
        u16 ogf : 6;     /**< OpCode Group Field */
    } __PACKED_POST__ opcode;
    u8 total;
    u8 cmd;              /**< private command */
    u8 param[];
} __PACKED_POST__ HCI_COMMAND_PACKET;

typedef struct {
    u8 code;             /**< 0x01: HCI Command Packet
                              0x02: HCI ACL Data Packet
                              0x03: HCI Synchronous Data Packet
                              0x04: HCI Event Packet */
    u8 event;            /**< 0x00-0xFF: Each event is assigned a 1-Octet event code used to uniquely identify different types of events*/
    u8 total;            /**< Parameter Total Length */
    u8 param[];
} __PACKED_POST__ HCI_EVENT_PACKET;

typedef struct {
    u32 addr;
    u32 value;
} __PACKED_POST__ REGISTER_PARAM;

typedef struct {
	u8 cmd;
	u8 param[];
} __PACKED_POST__ HCI_RX_OPERATE;

typedef struct {
	u8 code;
	struct {
	        u16 ocf : 10;    /**< OpCode Command Field */
	        u16 ogf : 6;     /**< OpCode Group Field */
	    } __PACKED_POST__ opcode;
	//u8 total;
	u8 param[];
} __PACKED_POST__ HCI_TX_OPERATE;

typedef struct {
	u16 len;
    u8 operate;
  //  u32 addr;
    u8 param[];
} __PACKED_POST__ FLASH_OPERATE_REQ_PARAM;

typedef struct {
	u16 len;
    u8 operate;
    u8 param[];
} __PACKED_POST__ FLASH_OPERATE_RSP_PARAM;

typedef struct {
	u16 len;
    u8  operate;
    u32 addr;
    u8  param[];
} __PACKED_POST__ FLASH_OPERATE_ADDR_WRITE_REQ_PARAM;

typedef struct {
	u16 len;
    u8 operate;
    u8 status;

    u8 param[];
} __PACKED_POST__ FLASH_OPERATE_ADDR_WRITE_RSP_PARAM;


typedef struct {
    char *partition_name;
    u32 partition_offset;
    u32 partition_size;
} __PACKED_POST__ PARTITION_STRUCT;

void TRAhcit_UART_Rx(void);
uint8_t bl_get_boot_flag_value(void);
uint8_t bl_set_boot_flag_value(void);
uint8_t bl_set_aon_pmu_bit3_for_deepsleep(void);

#endif /* BK_HCI_PROTOCOL_H_ */
