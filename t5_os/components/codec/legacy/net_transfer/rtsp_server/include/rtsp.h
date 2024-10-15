/*
 * @Author: cijliu
 * @Date: 2021-02-04 16:43:10
 * @LastEditTime: 2021-02-20 16:39:00
 */
#ifndef __RTSP_H__
#define __RTSP_H__
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#define DEFAULT_STRING_MIN_LEN  (32)
#define DEFAULT_STRING_MAX_LEN  (256)
#define DEFAULT_RTSP_PORT       (554)
#define DEFAULT_RTSP_SUPPORT_METHOD     "DESCRIBE,SETUP,TEARDOWN,PLAY,PAUSE,ANNOUNCE,RECORD,GET_PARAMETER,SET_PARAMETER"
typedef enum {
    OPTIONS = 0,
	DESCRIBE,
	SETUP,
	PLAY,
	RECORD,
	PAUSE,
	TEARDOWN,
	ANNOUNCE,
	SET_PARAMETER,
	GET_PARAMETER,
	REDIRECT,
	BUTT,
} rtsp_method_enum_t;

typedef struct {
    char prefix[DEFAULT_STRING_MIN_LEN];
    char ip[DEFAULT_STRING_MIN_LEN];
    uint32_t port;
    char uri[DEFAULT_STRING_MAX_LEN];
} rtsp_url_t;

typedef struct {
    rtsp_method_enum_t method;
    rtsp_url_t url;
} rtsp_request_t;

typedef struct {
    uint8_t is_tcp;
    uint8_t is_multicast;
    union 
    {
        uint32_t rtp_port;
        uint32_t client_port;
    };
    union 
    {
        uint32_t rtcp_port;
        uint32_t server_port;
    };
    
    
} rtsp_transport_t;
typedef struct {
    uint32_t status;
    uint32_t cseq;
    char session[DEFAULT_STRING_MIN_LEN];
    char desc[DEFAULT_STRING_MIN_LEN];
    char datetime[DEFAULT_STRING_MIN_LEN];

    uint32_t timeout;
    uint32_t rtp_seq;
    uint64_t rtptime;
    
    rtsp_transport_t tansport;
    rtsp_request_t request;
} rtsp_rely_t;

typedef struct {
    rtsp_request_t request;
    uint32_t cseq;
    rtsp_transport_t tansport;
#ifdef __RTSP_USER_AGENT
    char ua[DEFAULT_STRING_MAX_LEN];
#endif
#ifdef __RTSP_RANGE
    char range[DEFAULT_STRING_MAX_LEN];
#endif
#ifdef __RTSP_ACCEPT
    char accept[DEFAULT_STRING_MAX_LEN];
#endif
    char session[DEFAULT_STRING_MIN_LEN];
} rtsp_msg_t;



#endif