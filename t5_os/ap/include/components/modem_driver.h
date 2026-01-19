// Copyright 2024-2025 Beken
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
*                      Function Declarations
*******************************************************************************/

/**
 * @brief     Modem Driver initialization
 *
 * Initialize the bk modem driver, then it will build a connection to modem modules.
 * Make sure to connect to a modem mudule by usb in hardware before call this API.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_modem_init(uint8_t comm_proto, uint8_t comm_if);

/**
 * @brief     Modem Driver uninstallation
 *
 * Turn off the Modem driver, and it will disconnect to modem modules.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_modem_deinit(void);


