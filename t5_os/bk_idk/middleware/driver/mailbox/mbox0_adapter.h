#ifndef __MBOX0_ADAPTER_H__
#define __MBOX0_ADAPTER_H__

#include <driver/mailbox_types.h>
#include <common/bk_err.h>
#include "mbox0_drv.h"

bk_err_t bk_mailbox_init(void);
bk_err_t bk_mailbox_deinit(void);
bk_err_t bk_mailbox_send(mailbox_data_t *data, mailbox_endpoint_t src, mailbox_endpoint_t dst, void *arg);
bk_err_t bk_mailbox_recv_callback_register(mailbox_endpoint_t src, mailbox_endpoint_t dst, mailbox_callback_t callback);
bk_err_t bk_mailbox_recv_callback_unregister(mailbox_endpoint_t src, mailbox_endpoint_t dst);

#endif
