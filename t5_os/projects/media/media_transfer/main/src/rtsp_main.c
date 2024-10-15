#include <common/bk_include.h>
#include <os/str.h>

#include "lwip/tcp.h"
#include "bk_uart.h"
#include <os/mem.h>
#include <os/os.h>
#include <common/bk_kernel_err.h>

#include <lwip/sockets.h>

#include <components/video_transfer.h>
#include "video_transfer_udp.h"

#include <driver/media_types.h>
#include <driver/flash.h>
#include <driver/psram.h>
#include <time.h>
#include "media_app.h"
#include <driver/timer.h>
#include "rtsp_parse.h"
#include "rtsp_handler.h"
#include "h264_parse.h"
#include "cli.h"

#define DEMO_RTSP_TCP_SERVER_PORT          554
#define RTP_OVER_TCP	1
#define RTP_OVER_UDP	0
#define DEMO_RTSP_TCP_LISTEN_MAX           1

#define TAG "rtsp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
    char ip[16];
    int port;
} ip_t;

#define DEMO_RTSP_TCP_RCV_BUF_LEN               (1460)//1460

#define TCP_HEAD_ID (TCP_HEAD_ID_HB << 8 | TCP_HEAD_ID_LB)

beken_thread_t demo_rtsp_tcp_task = NULL;

static int demo_rtsp_watch_fd_list[DEMO_RTSP_TCP_LISTEN_MAX];
static int demo_rtsp_tcp_server_fd;
volatile int demo_rtsp_tcp_running = 0;

static uint16_t tcp_cache_count = 0;

static uint8_t *video_tcp_cmd_send_buffer = NULL;
ip_t ip;
int g_pause = 0;
beken_mutex_t rtsp_send_lock;
static uint8_t rtp_protocol = RTP_OVER_UDP;

extern void app_demo_rtsp_start(void);
extern void media_trans_set_device_type(camera_type_t type, yuv_mode_t mode, pixel_format_t fmt);
extern void media_trans_set_device_stream(uint16_t width, uint16_t height, frame_fps_t fps);
extern void set_udp_port(int port);
extern void set_udp_addr(uint32_t addr);

static void demo_rtsp_tcp_set_keepalive(int fd)
{
	int opt = 1, ret;
	// open tcp keepalive
	ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(int));

	opt = 30;  // 5 second
	ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &opt, sizeof(int));

	opt = 1;  // 1s second for intval
	ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &opt, sizeof(int));

	opt = 3;  // 3 times
	ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &opt, sizeof(int));
	ret = ret;
}

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

int demo_rtsp_tcp_video_send_packet(uint8_t *data, uint32_t len)
{
	int i = 0, snd_len = 0;

	if ((!demo_rtsp_tcp_task) || (demo_rtsp_tcp_server_fd == -1))
	{
		return 0;
	}

	for (i = 0; i < DEMO_RTSP_TCP_LISTEN_MAX; i++)
	{
		if (demo_rtsp_watch_fd_list[i] == -1)
		{
			continue;
		}

		rtos_lock_mutex(&rtsp_send_lock);
		snd_len = send(demo_rtsp_watch_fd_list[i], data, len, MSG_DONTWAIT | MSG_MORE);
		rtos_unlock_mutex(&rtsp_send_lock);
		if (snd_len < 0)
		{
			/* err */
			os_printf("send return fd:%d\r\n", snd_len);
			snd_len = 0;
		}
	}

	return snd_len;
}

int demo_rtsp_tcp_video_send_cmd(uint8_t *data, uint32_t len)
{
	int i = 0, snd_len = 0;

	if ((!demo_rtsp_tcp_task) || (demo_rtsp_tcp_server_fd == -1))
	{
		return 0;
	}

	os_memcpy(video_tcp_cmd_send_buffer, data, len);

	//LOGI("sequence: %u, length: %u\n", tcp_sequence, len);

	for (i = 0; i < DEMO_RTSP_TCP_LISTEN_MAX; i++)
	{
		if (demo_rtsp_watch_fd_list[i] == -1)
		{
			continue;
		}

		rtos_lock_mutex(&rtsp_send_lock);
		//addAON_GPIO_Reg0x2 = 2;
		snd_len = write(demo_rtsp_watch_fd_list[i], video_tcp_cmd_send_buffer, len);
		//addAON_GPIO_Reg0x2 = 0;
		rtos_unlock_mutex(&rtsp_send_lock);
		if (snd_len < 0)
		{
			/* err */
			//APP_DEMO_TCP_PRT("send return fd:%d\r\n", snd_len);
			snd_len = 0;
		}
	}

	return snd_len;
}

static uint8_t isplay = 0;
void demo_rtsp_tcp_data_recv_handle(char *data)
{
	if(isplay) {
		os_printf("\r\n <-<-<-<-<-<-\r\n%s \r\n", data);
	}
	int send_len = 0;
	rtsp_msg_t rtsp = rtsp_msg_load((const char *)data);
	char datetime[30];
	char msg[1024];

	rfc822_datetime_format(time(NULL), datetime);
	rtsp_rely_t rely = get_rely(rtsp);
	// rely.sdp = (char *)os_malloc(300);
	// os_memset(rely.sdp, 0, 300);
	os_memcpy(rely.datetime, datetime, strlen(datetime));
	ip_t *ipaddr = (ip_t *)os_malloc(sizeof(ip_t));
	os_memset(ipaddr, 0, sizeof(ipaddr));
	switch(rtsp.request.method){
		case SETUP:
			rely.tansport.server_port=8080;
			send_len = rtsp_rely_dumps(rely, msg, 1024);
			rtp_protocol = (rely.tansport.is_tcp == 0) ? RTP_OVER_UDP : RTP_OVER_TCP;
			sprintf(ip.ip, "%s", ipaddr->ip);
			ip.port = rtsp.tansport.client_port;
			g_pause = 1;
			//to do :udp or tco send thread create
			set_udp_port(ip.port);
			printf("rtp client ip:%s port:%d\n",ip.ip, ip.port);
			break;
		case DESCRIBE:
			// rely.sdp_len = strlen(sdp);
			// memcpy(rely.sdp, sdp, rely.sdp_len);
			send_len = rtsp_rely_dumps(rely, msg, 1024);
			break;
		case TEARDOWN:
			send_len = rtsp_rely_dumps(rely, msg, 1024);
			// tcp_server_send_msg(&tcp, client, msg, strlen(msg));
			// tcp_server_close_client(&tcp, client);
			// to do close conn
			g_pause = 0;
			app_demo_udp_deinit();
		default:
			send_len = rtsp_rely_dumps(rely, msg, 1024);
			break;
	}

	if (isplay) {
		//os_printf("\r\n ->->->->-> \r\n%s \r\n", msg);
	}
	int tcp_len = demo_rtsp_tcp_video_send_cmd((uint8_t *)msg, send_len);
	if (tcp_len > 0 && rtsp.request.method == PLAY)
	{
		os_printf("rtsp start %d !!!!!!!! \r\n", rtp_protocol);
		isplay=1;
		if (rtp_protocol == RTP_OVER_TCP) {
			app_demo_rtsp_start();
		} else {
			app_demo_udp_init();
		}
	}
}

static void demo_rtsp_tcp_main(beken_thread_arg_t data)
{
	GLOBAL_INT_DECLARATION();
	int maxfd = -1;
	int ret = 0, i = 0;
	int rcv_len = 0;
	struct sockaddr_in server_addr;
	socklen_t srvaddr_len = 0;
	fd_set watchfd;
	char *rcv_buf = NULL;

	(void)(data);

	rtos_delay_milliseconds(1000);
	LOGI("%s entry\n", __func__);

	rcv_buf = (char *) os_malloc((DEMO_RTSP_TCP_RCV_BUF_LEN + 1) * sizeof(char));
	if (!rcv_buf)
	{
		LOGE("tcp os_malloc failed\n");
		goto out;
	}

	demo_rtsp_tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (demo_rtsp_tcp_server_fd == -1)
	{
		LOGE("socket failed\n");
		goto out;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(DEMO_RTSP_TCP_SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	srvaddr_len = (socklen_t)sizeof(server_addr);
	if (bind(demo_rtsp_tcp_server_fd, (struct sockaddr *)&server_addr, srvaddr_len) == -1)
	{
		LOGE("bind failed\n");
		goto out;
	}

	if (listen(demo_rtsp_tcp_server_fd, DEMO_RTSP_TCP_LISTEN_MAX) == -1)
	{
		LOGE("listen failed\n");
		goto out;
	}

	maxfd = demo_rtsp_tcp_server_fd;

	for (i = 0; i < DEMO_RTSP_TCP_LISTEN_MAX; i++)
	{
		demo_rtsp_watch_fd_list[i] = -1;
	}

	LOGI("create rtsp instance \r\n");

	GLOBAL_INT_DISABLE();
	demo_rtsp_tcp_running = 1;
	GLOBAL_INT_RESTORE();

	LOGI("%s start\n", __func__);

	while (demo_rtsp_tcp_running)
	{

		LOGI("set fd\n");

		FD_ZERO(&watchfd);
		FD_SET(demo_rtsp_tcp_server_fd, &watchfd);

		for (i = 0; i < DEMO_RTSP_TCP_LISTEN_MAX; i++)
		{
			if (demo_rtsp_watch_fd_list[i] != -1)
			{
				FD_SET(demo_rtsp_watch_fd_list[i], &watchfd);
				if (maxfd < demo_rtsp_watch_fd_list[i])
				{
					maxfd = demo_rtsp_watch_fd_list[i];
				}
			}
		}

		LOGI("select fd\n");
		ret = select(maxfd + 1, &watchfd, NULL, NULL, NULL);
		if (ret <= 0)
		{
			LOGE("select ret:%d\n", ret);
			break;
		}
		else
		{
			// is new connection
			if (FD_ISSET(demo_rtsp_tcp_server_fd, &watchfd))
			{
				int new_cli_sockfd = -1;
				struct sockaddr_in client_addr;
				socklen_t cliaddr_len = 0;

				cliaddr_len = sizeof(client_addr);
				new_cli_sockfd = accept(demo_rtsp_tcp_server_fd, (struct sockaddr *)&client_addr, &cliaddr_len);
				if (new_cli_sockfd < 0)
				{
					LOGE("accept return fd:%d\n", new_cli_sockfd);
					break;
				}
#if CONFIG_ETH
				set_udp_addr(htonl(client_addr.sin_addr.s_addr));
#endif
				LOGI("new accept fd:%d\n", new_cli_sockfd);

				tcp_cache_count = 0;


				for (i = 0; i < DEMO_RTSP_TCP_LISTEN_MAX; i++)
				{
					if (demo_rtsp_watch_fd_list[i] == -1)
					{
						demo_rtsp_watch_fd_list[i] = new_cli_sockfd;

						demo_rtsp_tcp_set_keepalive(new_cli_sockfd);

						//TODO

						break;
					}
				}

				if (i == DEMO_RTSP_TCP_LISTEN_MAX)
				{
					LOGW("only accept %d clients\n", DEMO_RTSP_TCP_LISTEN_MAX);
					close(new_cli_sockfd);
				}
			}

			// search those added fd
			for (i = 0; i < DEMO_RTSP_TCP_LISTEN_MAX; i++)
			{
				if (demo_rtsp_watch_fd_list[i] == -1)
				{
					continue;
				}
				if (!FD_ISSET(demo_rtsp_watch_fd_list[i], &watchfd))
				{
					continue;
				}
				rcv_len = recv(demo_rtsp_watch_fd_list[i], rcv_buf, DEMO_RTSP_TCP_RCV_BUF_LEN, 0);

				FD_CLR(demo_rtsp_watch_fd_list[i], &watchfd);


				if (rcv_len <= 0)
				{
					// close this socket
					LOGI("recv close fd:%d, rcv_len:%d, %d\n", demo_rtsp_watch_fd_list[i], rcv_len, errno);
					close(demo_rtsp_watch_fd_list[i]);
					demo_rtsp_watch_fd_list[i] = -1;
				}
				else
				{
					LOGI("read count: %d\n", rcv_len);
					demo_rtsp_tcp_data_recv_handle(rcv_buf);
				}

			}
		}// ret = select
	}

out:

	LOGE("app_demo_tcp_main exit\n");

	//TODO CLEANUP

	if (rcv_buf)
	{
		os_free(rcv_buf);
		rcv_buf = NULL;
	}

	for (i = 0; i < DEMO_RTSP_TCP_LISTEN_MAX; i++)
	{
		if (demo_rtsp_watch_fd_list[i] != -1)
		{
			close(demo_rtsp_watch_fd_list[i]);
			demo_rtsp_watch_fd_list[i] = -1;
		}
	}

	if (demo_rtsp_tcp_server_fd != -1)
	{
		close(demo_rtsp_tcp_server_fd);
		demo_rtsp_tcp_server_fd = -1;
	}

	if (video_tcp_cmd_send_buffer)
	{
		os_free(video_tcp_cmd_send_buffer);
		video_tcp_cmd_send_buffer = NULL;
	}

	rtos_deinit_mutex(&rtsp_send_lock);

	GLOBAL_INT_DISABLE();
	demo_rtsp_tcp_running = 0;
	GLOBAL_INT_RESTORE();

	demo_rtsp_tcp_task = NULL;
	rtos_delete_thread(NULL);

}

void demo_rtsp_tcp_init()
{
	int ret = 0;

	if (demo_rtsp_tcp_task)
	{
		LOGE("%s already init\n", __func__);
	}

	if (video_tcp_cmd_send_buffer == NULL)
	{
		video_tcp_cmd_send_buffer = (uint8_t *)os_malloc(1460);
	}

	rtos_init_mutex(&rtsp_send_lock);

	if (!demo_rtsp_tcp_task)
	{
		ret = rtos_create_thread(&demo_rtsp_tcp_task,
		                         4,
		                         "demo_rtsp_tcp",
		                         (beken_thread_function_t)demo_rtsp_tcp_main,
		                         1024 * 6,
		                         (beken_thread_arg_t)NULL);
		if (ret != kNoErr)
		{
			LOGE("Error: Failed to create spidma_intfer: %d\n", ret);
		}
	}
}

void demo_rtsp_tcp_deinit(void)
{
	GLOBAL_INT_DECLARATION();

	LOGI("app_demo_tcp_deinit\n");

	if (demo_rtsp_tcp_running == 0)
	{
		return;
	}

	GLOBAL_INT_DISABLE();
	demo_rtsp_tcp_running = 0;
	GLOBAL_INT_RESTORE();

	while (demo_rtsp_tcp_task)
	{
		rtos_delay_milliseconds(10);
	}
}

// #define RTSP_CMD_CNT (sizeof(rtsp_cmd) / sizeof(struct cli_command))
// static const struct cli_command rtsp_cmd[] = {
// 	{"rtsp_send", "rtsp_send tcp|udp", demo_rtsp_tcp_init},
// 	{"rtsp_test", "rtsp_send tcp|udp", test_1},
// };

// int cli_rtsp_init(void)
// {
// 	return cli_register_commands(rtsp_cmd, RTSP_CMD_CNT);
// }