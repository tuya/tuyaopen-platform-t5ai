/** @file
 *  @brief Bluetooth HCI driver API.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_DRIVER_H_
#define ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_DRIVER_H_

/**
 * @brief HCI drivers
 * @defgroup bt_hci_driver HCI drivers
 * @ingroup bluetooth
 * @{
 */

#include "bluetooth/hci.h"
#include "stdbool.h"

//#include "bluetooth/buf.h"
//#include "bluetooth/hci_vs.h"
//#include "zephyr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Possible types of buffers passed around the Bluetooth stack */
enum bt_buf_type {
	/** HCI command */
	BT_BUF_CMD,
	/** HCI event */
	BT_BUF_EVT,
	/** Outgoing ACL data */
	BT_BUF_ACL_OUT,
	/** Incoming ACL data */
	BT_BUF_ACL_IN,
	/** Outgoing ISO data */
	BT_BUF_ISO_OUT,
	/** Incoming ISO data */
	BT_BUF_ISO_IN,
	/** H:4 data */
	BT_BUF_H4,
};

enum {
	/* The host should never send HCI_Reset */
	BT_QUIRK_NO_RESET = (1 << 0),
	/* The controller does not auto-initiate a DLE procedure when the
	 * initial connection data length parameters are not equal to the
	 * default data length parameters. Therefore the host should initiate
	 * the DLE procedure after connection establishment. */
	BT_QUIRK_NO_AUTO_DLE = (1 << 1),
};

#define IS_BT_QUIRK_NO_AUTO_DLE(bt_dev) ((bt_dev)->drv->quirks & BT_QUIRK_NO_AUTO_DLE)

/* @brief The HCI event shall be given to bt_recv_prio */
#define BT_HCI_EVT_FLAG_RECV_PRIO (1 << 0)
/* @brief  The HCI event shall be given to bt_recv. */
#define BT_HCI_EVT_FLAG_RECV      (1 << 1)

/** Possible values for the 'bus' member of the bt_hci_driver struct */
enum bt_hci_driver_bus {
	BT_HCI_DRIVER_BUS_VIRTUAL       = 0,
	BT_HCI_DRIVER_BUS_USB           = 1,
	BT_HCI_DRIVER_BUS_PCCARD        = 2,
	BT_HCI_DRIVER_BUS_UART          = 3,
	BT_HCI_DRIVER_BUS_RS232         = 4,
	BT_HCI_DRIVER_BUS_PCI           = 5,
	BT_HCI_DRIVER_BUS_SDIO          = 6,
	BT_HCI_DRIVER_BUS_SPI           = 7,
	BT_HCI_DRIVER_BUS_I2C           = 8,
	BT_HCI_DRIVER_BUS_IPM           = 9,
};

void hci_driver_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_DRIVER_H_ */
