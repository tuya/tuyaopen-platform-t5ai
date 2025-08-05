/**
* @file tkl_init_network.h
* @brief Common process - tkl init network description
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_INIT_NETWORK_H__
#define __TKL_INIT_NETWORK_H__

#include "tkl_network.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief the description of tuya kernel adapter layer network
 *
 */
typedef struct {
    TUYA_ERRNO            (*tkl_get_errno)          (VOID);
    OPERATE_RET           (*tkl_fd_set1)            (CONST INT_T fd, TUYA_FD_SET_T* fds);
    OPERATE_RET           (*tkl_fd_clear)           (CONST INT_T fd, TUYA_FD_SET_T* fds);
    OPERATE_RET           (*tkl_fd_isset)           (CONST INT_T fd, TUYA_FD_SET_T* fds);
    OPERATE_RET           (*tkl_fd_zero)            (TUYA_FD_SET_T* fds);
    INT_T                 (*tkl_select)             (CONST INT_T maxfd, TUYA_FD_SET_T *readfds, TUYA_FD_SET_T *writefds, TUYA_FD_SET_T *errorfds, CONST UINT_T ms_timeout);
    TUYA_ERRNO            (*tkl_close)              (CONST INT_T fd);
    INT_T                 (*tkl_socket_create)      (CONST TUYA_PROTOCOL_TYPE_E type);
    INT_T                 (*tkl_socket_create_v6)   (CONST TUYA_PROTOCOL_TYPE_E type);
    TUYA_ERRNO            (*tkl_connect)            (CONST INT_T fd, CONST TUYA_IP_ADDR_T addr, CONST UINT16_T port);
    TUYA_ERRNO            (*tkl_connect_raw)        (CONST INT_T fd, VOID *p_socket, CONST INT_T len);
    TUYA_ERRNO            (*tkl_bind)               (CONST INT_T fd, CONST TUYA_IP_ADDR_T addr, CONST UINT16_T port);
    TUYA_ERRNO            (*tkl_listen)             (CONST INT_T fd, CONST INT_T backlog);
    TUYA_ERRNO            (*tkl_send)               (CONST INT_T fd, CONST VOID *buf, CONST UINT_T nbytes);
    TUYA_ERRNO            (*tkl_send_to)            (CONST INT_T fd, CONST VOID *buf, CONST UINT_T nbytes, CONST TUYA_IP_ADDR_T addr, CONST UINT16_T port);
    TUYA_ERRNO            (*tkl_recv)               (CONST INT_T fd, VOID *buf, CONST UINT_T nbytes);
    TUYA_ERRNO            (*tkl_recvfrom)           (CONST INT_T fd, VOID *buf, CONST UINT_T nbytes, TUYA_IP_ADDR_T *addr, UINT16_T *port);
    TUYA_ERRNO            (*tkl_accept)             (CONST INT_T fd, TUYA_IP_ADDR_T *addr, UINT16_T *port);
    INT_T                 (*tkl_recv_nd_size)       (CONST INT_T fd, VOID *buf, CONST UINT_T buf_size, CONST UINT_T nd_size);
    OPERATE_RET           (*tkl_socket_bind)        (CONST INT_T fd, CONST CHAR_T *ip);
    OPERATE_RET           (*tkl_set_block)          (CONST INT_T fd, CONST BOOL_T block);
    OPERATE_RET           (*tkl_set_cloexec)        (CONST INT_T fd);
    OPERATE_RET           (*tkl_get_socket_ip)      (CONST INT_T fd, TUYA_IP_ADDR_T *addr);
    INT_T                 (*tkl_get_nonblock)       (CONST INT_T fd);
    OPERATE_RET           (*tkl_gethostbyname)      (CONST CHAR_T *domain, TUYA_IP_ADDR_T *addr);
    TUYA_IP_ADDR_T        (*tkl_str2addr)           (CONST CHAR_T *ip_str);
    CHAR_T*               (*tkl_addr2str)           (CONST TUYA_IP_ADDR_T ipaddr);
    OPERATE_RET           (*tkl_setsockopt)         (CONST INT_T fd, CONST TUYA_OPT_LEVEL level, CONST TUYA_OPT_NAME optname, CONST VOID_T *optval, CONST INT_T optlen);
    OPERATE_RET           (*tkl_getsockopt)         (CONST INT_T fd, CONST TUYA_OPT_LEVEL level, CONST TUYA_OPT_NAME optname, VOID_T *optval, INT_T *optlen);
    OPERATE_RET           (*tkl_set_timeout)        (CONST INT_T fd, CONST INT_T ms_timeout, CONST TUYA_TRANS_TYPE_E type);
    OPERATE_RET           (*tkl_set_bufsize)        (CONST INT_T fd, CONST INT_T buf_size, CONST TUYA_TRANS_TYPE_E type);
    OPERATE_RET           (*tkl_set_reuse)          (CONST INT_T fd);
    OPERATE_RET           (*tkl_disable_nagle)      (CONST INT_T fd);
    OPERATE_RET           (*tkl_set_broadcast)      (CONST INT_T fd);
    OPERATE_RET           (*tkl_set_keepalive)      (INT_T fd, CONST BOOL_T alive, CONST UINT_T idle, CONST UINT_T intr, CONST UINT_T cnt);
    OPERATE_RET           (*tkl_getsockname)        (INT_T fd, TUYA_IP_ADDR_T *addr, UINT16_T *port);
    OPERATE_RET           (*tkl_getpeername)        (INT_T fd, TUYA_IP_ADDR_T *addr, UINT16_T *port);
    OPERATE_RET           (*tkl_sethostname)        (CONST CHAR_T *hostname);
    OPERATE_RET           (*tkl_check_ipv4_ipv6)    (BOOL_T *has_ipv4, BOOL_T *has_ipv6);
} TKL_NETWORK_DESC_T;

/**
 * @brief register wired description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TKL_NETWORK_DESC_T* tkl_network_desc_get(VOID_T);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_INIT_WIRED_H__

