/*
 * @Author: cijliu
 * @Date: 2021-02-01 14:09:37
 * @LastEditTime: 2021-02-26 16:39:45
 */
#include "rtsp.h"
#include "rtsp_parse.h"
#include "rtsp_handler.h"
#include "os/os.h"
#include "os/mem.h"
#define EVENT_PARSE(msg)                \
rtsp_msg_t _rtsp;                       \
rtsp_parse_msg(msg, &_rtsp);            \
switch(_rtsp.request.method){
    
#define EVENT_END(func)                 \
default:                                \
    func(_rtsp);                        \
    break;                              \
}                                       \
return _rtsp;                                     


#define EVENT(m,func,enable)            \
{                                       \
    case m:{                            \
        if(enable)func(_rtsp);          \
        break;                          \
    }                                   \
}

#define RELY_ADD_COND(offset, m,v,msg, args...)         \
do{                                             \
    if(m == v){                                 \
        char str[1024]={0};                     \
        snprintf(str, 2048, ##args);                   \
        offset += snprintf(msg + offset, 512, "%s" ,str);          \
    }                                           \
}while(0)

#define RELY_ADD(offset, msg, args...)      RELY_ADD_COND(offset, 0, 0, msg, ##args)   

void event_error_handler(rtsp_msg_t rtsp)
{
    printf("undefine rtsp type\n");
}
void event_default_handler(rtsp_msg_t rtsp)
{
    //printf("undefine rtsp type\n");
}
rtsp_msg_t rtsp_msg_load(const char *msg)
{
    EVENT_PARSE(msg)
    EVENT(OPTIONS,        rtsp_options_handler,0);
    EVENT(DESCRIBE,       rtsp_describe_handler,0);
    EVENT(SETUP,          rtsp_setup_handler,0);
    EVENT(PLAY,           rtsp_play_handler,0);
    EVENT(RECORD,         rtsp_record_handler,0);
    EVENT(PAUSE,          rtsp_pause_handler,0);
    EVENT(TEARDOWN,       rtsp_teardown_handler,0);
    EVENT(ANNOUNCE,       rtsp_announce_handler,0);
    EVENT(SET_PARAMETER,  rtsp_set_parameter_handler,0);
    EVENT(GET_PARAMETER,  rtsp_get_parameter_handler,0);
    EVENT(REDIRECT,       rtsp_redirect_handler,0);
    EVENT_END(event_error_handler);
}

static int rtsp_sdp_generate(char *sdp)
{
	char sdp_temp[300]="v=0\n"
		"o=- 16409863082207520751 16409863082207520751 IN IP4 192.168.250.10\n"
		"c=IN IP4 192.168.250.10\n"
		"s=media presentation \n"
		"t=0 0\n"
		"b=CT:650\n"
		//"a=range:npt=0-1.4\n"
		"a=recvonly\n"
		"m=video 0 RTP/AVP 96\n"
		"a=rtpmap:96 H264/90000\n"
		"a=fmtp:96 packetization-mode=1;profile-level-id=420015;\n"
		"a=framerate:25";
	
	
	os_memcpy(sdp, sdp_temp, 300);
	int sdp_len = strlen(sdp);
	os_printf("@@@@@@@@sdp len : %d \r\n", sdp_len);
	os_printf("******1****** %x %s \r\n", sdp, sdp);
	return sdp_len;
}

int rtsp_rely_dumps(rtsp_rely_t rely, char *msg, uint32_t len)
{
    rtsp_method_enum_t method = rely.request.method;
    if(len < 1024) return -1;

	int offset = 0;
    memset(msg, 0, len);
    RELY_ADD(offset, msg,                           "RTSP/1.0 %d %s\r\n", rely.status, rely.desc);
	RELY_ADD(offset, msg, "CSeq: %d\r\n", rely.cseq);
	RELY_ADD(offset, msg,                           "Date: %s\r\n",rely.datetime);
    RELY_ADD_COND(offset, method, OPTIONS, msg,     "Public: %s\r\n", DEFAULT_RTSP_SUPPORT_METHOD);
	RELY_ADD_COND(offset, method, DESCRIBE, msg, "Content-type: application/sdp\r\n");
	RELY_ADD_COND(offset, ((method == SETUP)|(method == PLAY)|(method == TEARDOWN)), 1, msg,
                                            "Session: %s;timeout=%d\r\n", rely.session, rely.timeout == 0?60:rely.timeout);
	os_printf("====tcp mode==== : %d \r\n", rely.tansport.is_tcp);
	if (rely.tansport.is_tcp)
	{
		RELY_ADD_COND(offset, method, SETUP, msg,			"Transport: RTP/AVP/TCP;unicast;interleaved=0-1;ssrc=24e4e500;mode=%s\r\n", "play");
	} else {
		RELY_ADD_COND(offset, method, SETUP, msg,       "Transport: RTP/AVP%s;%scast;client_port=%d-%d;server_port=%d-%d\r\n", 
											rely.tansport.is_tcp ? "/TCP" : "",
											rely.tansport.is_multicast ? "multi" : "uni",
											rely.tansport.client_port,
											rely.tansport.client_port + 1,
											rely.tansport.server_port,
											rely.tansport.server_port + 1);
	}
    
    RELY_ADD_COND(offset, method, PLAY, msg,        "Range: npt=0.000-\r\n"); 
    RELY_ADD_COND(offset, method, PLAY, msg,        "RTP-Info: url=rtsp://%s:%d/%s;seq=%d;rtptime=%ld\r\n", 
                                            rely.request.url.ip,
                                            rely.request.url.port,
                                            rely.request.url.uri,
                                            rely.rtp_seq,
                                            rely.rtptime);

	int sdp_len = 0;
	char *sdp = NULL;
	if (method == DESCRIBE) {
		os_printf("sdp generate  \r\n");
		sdp = (char *)os_malloc(512);
		if (sdp==NULL) {
			os_printf("malloc faile \r\n");
		}
		sdp_len = rtsp_sdp_generate(sdp);
		os_printf("******2****** %x %s \r\n", sdp, sdp);
	}
	os_printf("*******3***** %x %s \r\n", sdp, sdp);
    RELY_ADD(offset, msg,                           "Content-Length: %d\r\n\r\n", sdp_len);
    RELY_ADD_COND(offset, method, DESCRIBE, msg,    "%s", sdp);
	os_printf("******4****** %x %s \r\n", sdp, sdp);
    //printf("msg:%ld\n%s\n",strlen(msg),msg);
	if (sdp_len > 0) {
		os_free(sdp);
	}
    return offset;
    
}
rtsp_rely_t get_rely(rtsp_msg_t rtsp)
{
    rtsp_rely_t rely;
    memset(&rely, 0, sizeof(rtsp_rely_t));
    sprintf(rely.desc,"OK");
    sprintf(rely.session,"%p",&rtsp);
    rely.request = rtsp.request;
    rely.status  = 200;
    rely.cseq = rtsp.cseq;
    memcpy(rely.session,rtsp.session, strlen(rtsp.session));
    rely.tansport = rtsp.tansport;
    return rely;
}
