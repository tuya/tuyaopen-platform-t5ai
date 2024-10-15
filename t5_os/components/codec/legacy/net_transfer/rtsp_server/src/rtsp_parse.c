/*
 * @Author: cijliu
 * @Date: 2021-02-04 16:42:54
 * @LastEditTime: 2021-02-26 16:40:06
 */

#include "rtsp.h"
#include "os/os.h"
#define RTSP_METHOD_PARSE(msg, val)             do{ if(strcmp(opt, #val) == 0)      return val;         }while(0)
#define FUNC_CHECK(func, num)                   do{ if(num != func)                 return -1;          }while(0)

#define RTSP_LINE(msg, key, line)                       \
do{                                                     \
    char *str = strstr(msg, key);                       \
    if(str == NULL){                                    \
        return -1;                                      \
    }                                                   \
    FUNC_CHECK(sscanf(str,"%[^\n]",line),1);            \
}while(0)

#define RTSP_PARSE(msg, key,num,args...)                \
do{                                                     \
    char line[DEFAULT_STRING_MAX_LEN];                  \
    RTSP_LINE(msg, key,line);                           \
    num = sscanf(line, ##args);               \
}while(0)


// static const char *method[] = {
//     "OPTIONS",
//     "DESCRIBE",
// 	"SETUP",
// 	"PLAY",
// 	"RECORD",
// 	"PAUSE",
// 	"TEARDOWN",
// 	"ANNOUNCE",
// 	"SET_PARAMETER",
// 	"GET_PARAMETER",
// 	"REDIRECT",
// 	"BUTT",
// };


static rtsp_method_enum_t rtsp_parse_method(const char *opt)
{
    RTSP_METHOD_PARSE(opt, OPTIONS);
    RTSP_METHOD_PARSE(opt, DESCRIBE);
    RTSP_METHOD_PARSE(opt, SETUP);
    RTSP_METHOD_PARSE(opt, PLAY);
    RTSP_METHOD_PARSE(opt, RECORD);
    RTSP_METHOD_PARSE(opt, PAUSE);
    RTSP_METHOD_PARSE(opt, TEARDOWN);
    RTSP_METHOD_PARSE(opt, ANNOUNCE);
    RTSP_METHOD_PARSE(opt, SET_PARAMETER);
    RTSP_METHOD_PARSE(opt, GET_PARAMETER);
    RTSP_METHOD_PARSE(opt, REDIRECT);
    return BUTT;
}



static int rtsp_parse_session(const char *msg, char *session)
{
	uint8_t parse_num = 0;
    RTSP_PARSE(msg, "Session", parse_num, "Session: %s\r\n", session);
	if (parse_num == 1) {
		return 0;
	}
    return -1;
}


static int rtsp_parse_cseq(const char *msg, uint32_t *cseq)
{
	uint8_t parse_num = 0;
    RTSP_PARSE(msg, "CSeq", parse_num, "CSeq: %d\r\n", cseq);
    if (parse_num == 1) {
		return 0;
	}
    return -1;
}

static int rtsp_parse_transport(const char *msg, rtsp_transport_t *trans)
{
    char tcpip[DEFAULT_STRING_MIN_LEN];
    char cast[DEFAULT_STRING_MIN_LEN];
	uint8_t parse_num = 0;
    RTSP_PARSE(msg, "Transport", parse_num, "Transport: %[^;];%[^;];client_port=%d-%d\r\n", tcpip, cast, &trans->rtp_port, &trans->rtcp_port);
    trans->is_tcp = (strcmp(tcpip, "RTP/AVP/TCP") == 0) ? 1:0;
    trans->is_multicast = (strcmp(cast, "multicast") == 0) ? 1:0;
	// os_printf("*************** \n %s \r\n", msg);
	// os_printf("tcp transport : %d \r\n", trans->is_tcp);
	if (trans->is_tcp == 1 && parse_num == 2) {
		return 0;
	}
	if (trans->is_tcp == 0 && parse_num == 4) {
		return 0;
	}
    
    return -1;

}

static int rtsp_parse_request(const char *msg, rtsp_request_t *req)
{
    char line[DEFAULT_STRING_MAX_LEN];
    char method[DEFAULT_STRING_MIN_LEN];
    char addr[DEFAULT_STRING_MIN_LEN];
    int port = 0;
    FUNC_CHECK(sscanf(msg,"%[^\n]",line), 1);
    FUNC_CHECK(sscanf(line, "%s %[^'/']//%[^'/']/%s RTSP/1.0\r\n", method, req->url.prefix, addr, req->url.uri), 4);
    FUNC_CHECK(sscanf(addr, "%[^':']:%d",req->url.ip,&port), 2);
    req->url.port = port == 0 ? DEFAULT_RTSP_PORT:port;
    req->method = rtsp_parse_method(method);
    return 0;

}

int rtsp_parse_msg(const char *msg, rtsp_msg_t *rtsp)
{
	os_printf("parse msg \r\n");
    memset(rtsp, 0, sizeof(rtsp_msg_t));
    FUNC_CHECK(rtsp_parse_request(msg, &rtsp->request), 0);
    FUNC_CHECK(rtsp_parse_cseq(msg, &rtsp->cseq), 0);

    if(rtsp->request.method == SETUP){
		os_printf("parse SETUP \r\n");
        FUNC_CHECK(rtsp_parse_transport(msg, &rtsp->tansport), 0);
    }
    if(rtsp->request.method == PLAY){
		os_printf("parse PLAY \r\n");
        FUNC_CHECK(rtsp_parse_session(msg, rtsp->session), 0);
    }
    
    
    return 0;
}
