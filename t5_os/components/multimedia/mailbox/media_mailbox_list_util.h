// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "bk_list.h"

typedef struct
{
    uint32_t event;
    beken_semaphore_t sem;
    uint32_t result;
    uint32_t type;
    uint32_t param;
    uint8_t src;
    uint8_t dest;
    uint8_t count;
    uint8_t ack_flag;
} media_mailbox_msg_t;

typedef struct
{
    LIST_HEADER_T list;
    media_mailbox_msg_t *msg;
} media_mailbox_list_t;

enum
{
    APP_MODULE = 0,
    MAJOR_MODULE,
    MINOR_MODULE,
} ;

enum
{
	MAILBOX_MESSAGE_SEND = 0,
	MAILBOX_MESSAGE_ACK,
};


enum
{
    MAILBOX_MSG_TYPE_REQ = 0,
    MAILBOX_MSG_TYPE_RSP,
    MAILBOX_MSG_TYPE_NOTIFY,
    MAILBOX_MSG_TYPE_ABORT,
} ;
void media_mailbox_list_clear(LIST_HEADER_T *list, uint8_t flag);
media_mailbox_msg_t *media_mailbox_list_get_node(beken_semaphore_t msg, LIST_HEADER_T *list);
media_mailbox_msg_t *media_mailbox_list_del_node(beken_semaphore_t msg, LIST_HEADER_T *list);
media_mailbox_msg_t *media_mailbox_list_del_node_by_event(uint32_t event, LIST_HEADER_T *list);
media_mailbox_msg_t *media_mailbox_list_pop(LIST_HEADER_T *list);
bk_err_t media_mailbox_list_push(media_mailbox_msg_t *tmp, LIST_HEADER_T *list);

bk_err_t msg_send_req_to_media_app_mailbox_sync(uint32_t event, uint32_t in_param, uint32_t *out_param);
bk_err_t msg_send_rsp_to_media_app_mailbox(media_mailbox_msg_t *msg, uint32_t result);
bk_err_t msg_send_notify_to_media_app_mailbox(media_mailbox_msg_t *msg);

bk_err_t msg_send_req_to_media_major_mailbox_sync(uint32_t event, uint32_t dest, uint32_t in_param, uint32_t *out_param);
bk_err_t msg_send_rsp_to_media_major_mailbox(media_mailbox_msg_t *msg, uint32_t result, uint32_t dest);
bk_err_t msg_send_notify_to_media_major_mailbox(media_mailbox_msg_t *msg, uint32_t dest);


bk_err_t msg_send_req_to_media_minor_mailbox_sync(uint32_t event, uint32_t dest, uint32_t in_param, uint32_t *out_param);
bk_err_t msg_send_rsp_to_media_minor_mailbox(media_mailbox_msg_t *msg, uint32_t result, uint32_t dest);
bk_err_t msg_send_notify_to_media_minor_mailbox(media_mailbox_msg_t *msg, uint32_t dest);

bk_err_t media_major_start_up_notify(void);
bk_err_t media_major_shut_down_notify(void);
bk_err_t media_app_mailbox_check_cpu1_state(void);
void media_app_mailbox_cpu1_startup_request(void);
void media_app_mailbox_cpu1_sthutdown_request(void);
