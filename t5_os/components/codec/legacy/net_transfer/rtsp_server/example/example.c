/*
 * @Author: cijliu
 * @Date: 2021-02-04 16:04:16
 * @LastEditTime: 2021-02-26 17:01:59
 */
#include <stdio.h>
#include "include/rtp.h"
#include "include/rtsp_parse.h"
#include "include/rtsp_handler.h"
#include "include/h264.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "pthread.h"
#include "include/net.h"
#define H264_FILENAME  "test.h264"
#define SOCKET_ERROR    (-1)
#define INVALID_SOCKET  (-1)
#define BUF_SIZE 2048
typedef struct {
    unsigned char *data;
    int len;
} file_t;
pthread_t rtp;
int g_pause = 0;
ip_t ip;
const char* rfc822_datetime_format(time_t time, char* datetime)
{
	int r;
	char *date = asctime(gmtime(&time));
	char mon[8], week[8];
	int year, day, hour, min, sec;
	sscanf(date, "%s %s %d %d:%d:%d %d",week, mon, &day, &hour, &min, &sec, &year);
	r = snprintf(datetime, 32, "%s, %02d %s %04d %02d:%02d:%02d GMT",
		week,day,mon,year,hour,min,sec);
	return r > 0 && r < 32 ? datetime : NULL;
}
uint32_t rtsp_get_reltime (void)
{
	struct timespec tp;
    uint64_t ts;
	clock_gettime(CLOCK_MONOTONIC, &tp);
    ts = (tp.tv_sec * 1000000+ tp.tv_nsec / 1000);
    //printf("usec:%d\n",ts);
	return ts;
}

int open_h264_file(char *filename, file_t * file)
{
    struct stat info;
    stat(filename, &info);
    FILE *fp=fopen(filename, "rb");
    file->data = (unsigned char *)malloc(info.st_size);
    memset(file->data, 0, info.st_size);
    fread(file->data, 1, info.st_size, fp);
    file->len = info.st_size;
    fclose(fp);
    return 0;
}
void close_h264_file(file_t *file)
{
    if(file->data){
        free(file->data);
        file->data = NULL;
    }
    
}

void *rtp_thread(void *args)
{
    ip_t* ipaddr = &ip;
    udp_t udp,rtcp;
    if(udp_server_init(&udp, 45504)){
        printf("udp server init fail.\n");
        return NULL;
    }
    if(udp_server_init(&rtcp, 45505)){
        printf("udp server init fail.\n");
        return NULL;
    }
    char *filename = H264_FILENAME;
    file_t file;
    uint32_t rtptime = 0;
    int idr = 0;
    rtp_header_t header;
    rtp_header_init(&header);
    header.seq = 0;
    header.ts = 0;
    open_h264_file(filename, &file);
    h264_nalu_t *nalu = h264_nal_packet_malloc(file.data, file.len);
    printf("rtp server init.\n");
    while(g_pause){
        h264_nalu_t *h264_nal = nalu;
        while(h264_nal && g_pause){
            if(h264_nal->type == H264_NAL_IDR || h264_nal->type == H264_NAL_PFRAME){
                if(rtptime == 0){
                    rtptime = rtsp_get_reltime();
                }
                else {
                    while((rtptime + 40000) > rtsp_get_reltime()){
                        usleep(100);
                    }
                    //printf("sleep:%d us\n", rtsp_get_reltime() - rtptime);
                    rtptime = rtsp_get_reltime();

                }
                idr = 1;
                rtp_packet_t* rtp_ptk = rtp_packet_malloc(&header, h264_nal->data, h264_nal->len);
                rtp_packet_t* cur = rtp_ptk;
                while(cur){
                    udp_server_send_msg(&udp, ipaddr->ip, ipaddr->port, (unsigned char*)cur->data, cur->len);
                    cur = cur->next;
                }
                rtp_packet_free(rtp_ptk);
            }
            else if((h264_nal->type  == H264_NAL_SPS || h264_nal->type == H264_NAL_PPS) && !idr){
                rtp_packet_t* cur = rtp_packet_malloc(&header, h264_nal->data, h264_nal->len);
                udp_server_send_msg(&udp, ipaddr->ip, ipaddr->port, (unsigned char*)cur->data, cur->len);
                rtp_packet_free(cur);
            }
            h264_nal = h264_nal->next;
        }
    }
    h264_nal_packet_free(nalu);
    close_h264_file(&file);
    udp_server_deinit(&udp);
    udp_server_deinit(&rtcp);
    printf("rtp exit\n");
}
extern const char* rfc822_datetime_format(time_t time, char* datetime);
void* rtsp_thread(void *args)
{
    ip_t * ipaddr = (ip_t *)args;
    tcp_t tcp;
    int client = 0;
    if(tcp_server_init(&tcp, 8554)){
        printf("tcp server init fail.\n");
        return NULL;
    }

    char msg[2048];
    char sdp[2048]="v=0\n"
              "o=- 16409863082207520751 16409863082207520751 IN IP4 0.0.0.0\n"
              "c=IN IP4 0.0.0.0\n"
              "t=0 0\n"
              "a=range:npt=0-1.4\n"
              "a=recvonly\n"
              "m=video 0 RTP/AVP 97\n"
              "a=rtpmap:97 H264/90000\n"
              ;
    while(1){
        if(client == 0){
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(tcp.sock,&fds);
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            r = select(tcp.sock + 1,&fds,NULL,NULL,&tv);
            if(-1 == r || 0 == r)
            {
                continue;
            }
            client  = tcp_server_wait_client(&tcp);
            sprintf(ipaddr->ip, "%s", inet_ntoa(tcp.addr.sin_addr) ); 
            ipaddr->port = ntohs(tcp.addr.sin_port);
            printf("rtsp client ip:%s port:%d\n", inet_ntoa(tcp.addr.sin_addr), ntohs(tcp.addr.sin_port));
        }
        char recvbuffer[2048];
        tcp_server_receive_msg(&tcp, client, recvbuffer, sizeof(recvbuffer));
        rtsp_msg_t rtsp = rtsp_msg_load(recvbuffer);
        char datetime[30];
	    rfc822_datetime_format(time(NULL), datetime);
        rtsp_rely_t rely = get_rely(rtsp);
        memcpy(rely.datetime, datetime, strlen(datetime));
        switch(rtsp.request.method){
        case SETUP:
            rely.tansport.server_port=45504;
            rtsp_rely_dumps(rely, msg, 2048);
            sprintf(ip.ip, "%s", ipaddr->ip);
            ip.port = rtsp.tansport.client_port;
            g_pause = 1;
            pthread_create(&rtp,NULL,rtp_thread,&ip);
            printf("rtp client ip:%s port:%d\n",ip.ip, ip.port);
            break;
        case DESCRIBE:
            rely.sdp_len = strlen(sdp);
            memcpy(rely.sdp, sdp, rely.sdp_len);
            rtsp_rely_dumps(rely, msg, 2048);
            break;
        case TEARDOWN:
            rtsp_rely_dumps(rely, msg, 2048);
            tcp_server_send_msg(&tcp, client, msg, strlen(msg));
            tcp_server_close_client(&tcp, client);
            client = 0;
            g_pause = 0;
            continue;
        default:
            rtsp_rely_dumps(rely, msg, 2048);
            break;
        }
        tcp_server_send_msg(&tcp, client, msg, strlen(msg));
        usleep(1000);
    }
    tcp_server_deinit(&tcp);
}

void main()
{
    pthread_t rtsp_id;
    pthread_create(&rtsp_id,NULL,rtsp_thread,&ip);
    while(1){
        sleep(1);
    }

}
