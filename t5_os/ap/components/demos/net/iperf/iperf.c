/**
* iperf-liked network performance tool
*
*/
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include "bk_cli.h"
#include <lwip/sockets.h>
#include <stdlib.h>
#include <components/system.h>
#ifndef CONFIG_IPV6
#include <components/netif.h>
#include "net.h"
#endif
#include "bk_misc.h"
#define TAG "IPERF"

#define THREAD_SIZE             (4 * 1024)
#define THREAD_PROIRITY         4
#define IPERF_REPORT_TASK_PRIORITY      3
#define IPERF_PORT              5001
#define IPERF_BUFSZ             (8 * 1024)
#define IPERF_TX_TIMEOUT_SEC    (3)
#define IPERF_RX_TIMEOUT_SEC    (3)
#define IPERF_MAX_TX_CONNECT_RETRY    10
#define IPERF_MAX_TX_RETRY      10
#define IPERF_MAX_RX_RETRY      10
#define IPERF_REPORT_INTERVAL   1
#define IPERF_INVALID_INDEX     (-1)
#define IPERF_DEFAULT_SPEED_LIMIT   (-1)
#define IPERF_REPORT_TASK_NAME "iperf_report_task"
#define IPERF_REPORT_TASK_STACK 2048

#define IPERF_UDP_FIN_MAX_RETRY_CNTS 10 /* UDP FIN or FINACK max retries */
#define IPERF_UDP_FIN_TO 250000 /* 250ms: select timeout for UDP FIN */
#define IPERF_UDP_FINACK_TO 1 /* 1s: select timeout for UDP FINACK */

#define IPERF_DEFAULT_TIME     30
#define UDP_HEADER_FLAG        0x80000000

#define IPERF_THOUSAND_UNIT          1000       /* 1000 */
#define IPERF_MILLION_UNIT           1000000    /* 1000 * 1000 */
#define IPERF_KILO_UNIT              0x400      /* 1024 */
#define IPERF_MEGA_UNIT              0x100000   /* 1024 * 1024 */

enum {
	IPERF_STATE_STOPPED = 0,
	IPERF_STATE_STOPPING,
	IPERF_STATE_STARTED,
};

enum {
	IPERF_MODE_NONE = 0,
	IPERF_MODE_TCP_SERVER,
	IPERF_MODE_TCP_CLIENT,
	IPERF_MODE_UDP_SERVER,
	IPERF_MODE_UDP_CLIENT,
	IPERF_MODE_UNKNOWN,
};

typedef struct {
		int32_t id;
		uint32_t sec;
		uint32_t usec;
} iperf_udp_hdr_t;
	
	
typedef struct {
	int32_t flags;
	int32_t totalLenH;
	int32_t totalLenL;
	int32_t endSec;
	int32_t endUsec;
	int32_t errCnt;
	int32_t oooCnt;
	int32_t datagrams;
	int32_t jitterSec;
	int32_t jitterUsec;
} iperf_udp_server_hdr_t;


typedef struct {
	int state;
	int mode;
	char *host;
	int port;
	uint32_t interval;
} iperf_param_t;

static iperf_param_t s_param = {
	IPERF_STATE_STOPPED,
	IPERF_MODE_NONE, NULL,
	IPERF_PORT,
	IPERF_REPORT_INTERVAL
};
static uint32_t s_tick_last = 0;
static uint32_t s_tick_delta = 0;
static uint32_t s_pkt_delta = 0;
static uint32_t s_time = IPERF_DEFAULT_TIME;
static beken_mutex_t iperf_mutex;
//modifiable iperf parameters
//priority of iperf task
static uint32_t iperf_priority = THREAD_PROIRITY;
static uint32_t iperf_report_priority = IPERF_REPORT_TASK_PRIORITY;

//data size of iperf
static uint32_t iperf_size = IPERF_BUFSZ;
static int speed_limit = IPERF_DEFAULT_SPEED_LIMIT;

// TOS
static uint32_t iperf_tos = 0; // TOS_BE: 0, BK: 0x20, VI: 0xA0, VO: 0xD0

#if (CONFIG_TASK_WDT)
extern void bk_task_wdt_feed(void);
#endif

static void iperf_reset(void)
{
	s_param.mode = IPERF_MODE_NONE;
	if (s_param.host)
		os_free(s_param.host);
	s_param.host = NULL;
	s_param.state = IPERF_STATE_STOPPED;
}

static void iperf_set_sock_opt(int sock)
{
	struct timeval tv;
	int flag = 1;
	int tosval;

	setsockopt(sock, IPPROTO_TCP,   /* set option at TCP level */
			   TCP_NODELAY, /* name of option */
			   (void *)&flag,       /* the cast is historical cruft */
			   sizeof(int));        /* length of option value */

	tv.tv_sec = IPERF_TX_TIMEOUT_SEC;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	tv.tv_sec = IPERF_RX_TIMEOUT_SEC;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	tosval = iperf_tos;
	setsockopt(sock, IPPROTO_IP, IP_TOS, &tosval, sizeof(tosval));
}
static void printf_transfer_bw(uint64_t amount, float start, float end)
{
	int unit;
	char numeralSuffix = 'K';
	char speedSuffix = 'K';
	float trans;
	float gaps = end - start;

	if (amount > IPERF_MEGA_UNIT) {
		numeralSuffix = 'M';
		trans = amount / (float)IPERF_MEGA_UNIT;
	} else {
		trans = amount / (float)IPERF_KILO_UNIT;
	}

	if (((amount * 8) / gaps) >= (float)IPERF_MILLION_UNIT) {
		unit = IPERF_MILLION_UNIT; /* Mbps */
		speedSuffix = 'M';
	} else {
		unit = IPERF_THOUSAND_UNIT; /* Kbps */
	}

	BK_LOGD(NULL, "%4.1f-%4.1f sec	 %6.2f %cBytes	  %.2f %cbits/sec\r\n", start,end,
				trans, numeralSuffix, (((float)amount * 8) / gaps / (float)unit), speedSuffix);
}

static int32_t iperf_udpServer_process_firstData(int sock,const struct sockaddr *from,socklen_t len)
{
	if (connect(sock, from, len) < 0) {
		BK_LOGD(NULL, "connect failed %d\r\n", errno);
		return -1;
	}

	return 0;
}

static void iperf_send_ack_toFin(int sock, uint32_t *buffer ,uint64_t total_recvlen,uint32_t start, uint32_t end)
{
	int32_t ret;
	int32_t tries = 0;
	fd_set rdSet;
	struct timeval tmo;
	iperf_udp_server_hdr_t *hdr = NULL;
	uint32_t len = iperf_size;
	float duration = end-start;

	while (tries++ < IPERF_UDP_FIN_MAX_RETRY_CNTS) {
		if (len >= (sizeof(iperf_udp_hdr_t) + sizeof(iperf_udp_server_hdr_t))) {
			hdr = (iperf_udp_server_hdr_t *)((uint8_t *)buffer + sizeof(iperf_udp_hdr_t));
			(void)os_memset(hdr, 0x0, sizeof(iperf_udp_server_hdr_t));
			hdr->flags		= htonl(UDP_HEADER_FLAG);
			hdr->totalLenH	= htonl((long)(total_recvlen >> 32)); /* 32: bits of int32_t */
			hdr->totalLenL	= htonl((long)(total_recvlen & 0xFFFFFFFF));
			hdr->endSec 	= htonl((long)duration);
			hdr->endUsec	= htonl((long)((duration - (long)duration) * IPERF_MILLION_UNIT));
		}
	
		if (send(sock, buffer, iperf_size, 0) < 0) {
			break;
		}

		FD_ZERO(&rdSet);
		FD_SET(sock, &rdSet);
		tmo.tv_sec	= IPERF_UDP_FINACK_TO;
		tmo.tv_usec = 0;

		ret = select(sock + 1, &rdSet, NULL, NULL, &tmo);
		if (ret < 0) {
			break;
		} else if (ret == 0) {
			return;
		} else {
			ret = recv(sock, buffer, iperf_size, 0);
			if (ret <= 0) {
				return;
			} else {
				len = (uint32_t)ret;
			}
		}
	}	
}

static void iperf_process_udpServerHdr(uint32_t *buffer, uint32_t len)
{
	if (len > sizeof(iperf_udp_hdr_t) + sizeof(iperf_udp_server_hdr_t)) {
		iperf_udp_server_hdr_t *hdr = (iperf_udp_server_hdr_t *)((uint8_t *)buffer + sizeof(iperf_udp_hdr_t));
		uint64_t totalLen;
		float end;

		end = ntohl(hdr->endSec);
		end += ntohl(hdr->endUsec) / (float)IPERF_MILLION_UNIT;
		totalLen = (((uint64_t)ntohl(hdr->totalLenH)) << 32) + ntohl(hdr->totalLenL);  /* 32: bits of int32_t */
		
		printf_transfer_bw(totalLen,0,end);
	}
}

static void iperf_report_avg_bandwidth(uint64_t pkt_len)
{
	if (pkt_len > 0)
	{
		double total_f;
		total_f = (double)pkt_len * 8;
		total_f /= (double)(IPERF_MEGA_UNIT * s_tick_delta);
		BK_LOGD(NULL, "[%d-%d] sec bandwidth: %.2f  Mbits/sec.\r\n",
				0, s_tick_delta , total_f);
	}
}


#ifndef CONFIG_IPV6
extern err_t
etharp_request(struct netif *netif, const ip4_addr_t *ipaddr);
extern void *net_get_sta_handle(void);
extern void *net_get_uap_handle(void);

struct netif * get_netif(ip4_addr_t *ipaddr)
{
	struct wlan_ip_config sta_addr, ap_addr;

	net_get_if_addr(&sta_addr, net_get_sta_handle());
	net_get_if_addr(&ap_addr, net_get_uap_handle());

	if ((sta_addr.ipv4.gw & 0xFFFFFFl) == (ipaddr->addr & 0xFFFFFFl)) {
		return (struct netif *)net_get_sta_handle();
	} else if ((ap_addr.ipv4.gw & 0xFFFFFFl) == (ipaddr->addr & 0xFFFFFFl)) {
		return (struct netif *)net_get_uap_handle();
	} else
		return NULL;
}
#endif

static int iperf_bw_delay(int send_size)
{
	int period_us = 0;
	float pkts_per_tick = 0;

    if (speed_limit > 0) {
		pkts_per_tick = speed_limit * 1.0 / (send_size * 8) / 500;
		period_us = 2000 / pkts_per_tick;
		BK_LOGD(NULL, "iperf_size:%d, speed_limit:%d, period_us:%d pkts_per_tick:%d\n",
			send_size, speed_limit, period_us, pkts_per_tick);
	}
	return period_us;
}
static void iperf_report_task_handler(void *arg)
{
	uint32_t delay_interval = (s_param.interval * 1000);
	beken_time_get_time(&s_tick_last);
	s_tick_delta = 0;
	s_pkt_delta = 0;
	uint32_t tick_now = 0;
	uint32_t count = 0;
	uint64_t total_len = 0;

	while (s_param.state == IPERF_STATE_STARTED)
	{
		beken_time_get_time(&tick_now);
		rtos_delay_milliseconds(delay_interval / 5);

		rtos_lock_mutex(&iperf_mutex);
		int f;
		f = s_pkt_delta / IPERF_KILO_UNIT * 8;
		if (++count >= IPERF_REPORT_INTERVAL * 5)  /* 5: schedule every 200ms */
		{
			count = 0;

			if (s_pkt_delta >= 0)
			{
				BK_LOGD(NULL, "[%d-%d] sec bandwidth: %d Kbits/sec.\r\n",
						  s_tick_delta, s_tick_delta + IPERF_REPORT_INTERVAL, f);
			}
			s_tick_delta = s_tick_delta + IPERF_REPORT_INTERVAL;

			total_len += s_pkt_delta;
			s_tick_last = tick_now;
			s_pkt_delta = 0;
		}
		rtos_unlock_mutex(&iperf_mutex);

		if (s_param.mode == IPERF_MODE_TCP_CLIENT || s_param.mode == IPERF_MODE_UDP_CLIENT){
			if (s_tick_delta >= s_time)
			{
				break;
			}
		}
	}

	iperf_report_avg_bandwidth(total_len);
	total_len = 0 ;

	if (s_param.state == IPERF_STATE_STARTED)
	{
		s_param.state = IPERF_STATE_STOPPING;
	}
	rtos_delete_thread(NULL);
	rtos_deinit_mutex(&iperf_mutex);
}

static err_t iperf_report_task_start(void)
{
	int ret;
	ret = rtos_create_thread(NULL, iperf_report_priority, IPERF_REPORT_TASK_NAME,
						iperf_report_task_handler, IPERF_REPORT_TASK_STACK,
						(beken_thread_arg_t) 0);
	
	if (ret != kNoErr) {
		BK_LOGE(TAG, "create task %s failed", IPERF_REPORT_TASK_NAME);
		return BK_FAIL;
	}

	return BK_OK;
}

static void iperf_client(void *thread_param)
{
	int i, ret;
	int sock = 0;
	uint8_t *send_buf;
	struct sockaddr_in addr;
	uint32_t retry_cnt = 0;
	uint32_t connect_retry_cnt = 0;
	int period_us = 0;
	int fdelay_us = 0;
	int64_t prev_time = 0;
	int64_t send_time = 0;
	uint32_t now_time = 0;
	send_buf = (uint8_t *) os_malloc(iperf_size);
	if (!send_buf)
		goto _exit;

	for (i = 0; i < iperf_size; i++)
		send_buf[i] = i & 0xff;

	period_us = iperf_bw_delay(iperf_size);

	while (s_param.state == IPERF_STATE_STARTED) {
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0) {
			BK_LOGD(NULL, "iperf: create socket failed, err=%d!\n", errno);
			rtos_delay_milliseconds(1000);
			continue;
		}

		addr.sin_family = PF_INET;
		addr.sin_port = htons(s_param.port);
		addr.sin_addr.s_addr = inet_addr((char *)s_param.host);
#ifndef CONFIG_IPV6
		{
			struct netif *netif;
			netif = get_netif((ip4_addr_t *)&addr.sin_addr.s_addr);
			if (netif) {
				etharp_request(netif, (ip4_addr_t *)&addr.sin_addr.s_addr);
				rtos_delay_milliseconds(1000);
			}
		}
#endif

		ret = connect(sock, (const struct sockaddr *)&addr, sizeof(addr));
		if (ret == -1) {
			BK_LOGD(NULL, "iperf: connect failed, err=%d!\n", errno);
			connect_retry_cnt ++;
			if (connect_retry_cnt >= IPERF_MAX_TX_CONNECT_RETRY) {
				BK_LOGD(NULL, "iperf: tx connect max retry(%u)\n", connect_retry_cnt);
				goto _exit;
			}
		}else{
			break;
		}
	}

	BK_LOGD(NULL, "iperf: connect to iperf server successful!\n");
	iperf_set_sock_opt(sock);

	iperf_report_task_start();

	prev_time = rtos_get_time();
	while (s_param.state == IPERF_STATE_STARTED) {
		if (speed_limit > 0) {
			send_time = rtos_get_time();
			fdelay_us = period_us + (int32_t)(prev_time - send_time);
			prev_time = send_time;
		}
		else if (speed_limit == 0) {
			now_time = rtos_get_time();
			if ((now_time - prev_time) / 1000 > 0) {
				prev_time = now_time;
				rtos_delay_milliseconds(4);
			}
		}

		retry_cnt = 0;
_tx_retry:
		ret = send(sock, send_buf, iperf_size, 0);
		if (ret > 0) {
			s_pkt_delta +=ret;
			if (fdelay_us > 0) {
				bk_delay_us(fdelay_us);
			}
		}
		else {
			if (s_param.state != IPERF_STATE_STARTED)
				break;

			if (errno == EWOULDBLOCK) {
				retry_cnt ++;
				if (retry_cnt >= IPERF_MAX_TX_RETRY) {
					BK_LOGD(NULL, "iperf: tx reaches max retry(%u)\n", retry_cnt);
					break;
				} else
					goto _tx_retry;
			}

			break;
		}

		#if (CONFIG_TASK_WDT)
		bk_task_wdt_feed();
		#endif
	}

	closesocket(sock);
	rtos_delay_milliseconds(1000 * 2);
	
_exit:
	if (send_buf)
		os_free(send_buf);
	iperf_reset();
	BK_LOGD(NULL, "iperf: is stopped\n");
	rtos_delete_thread(NULL);
}

void iperf_server(void *thread_param)
{
	uint8_t *recv_data;
	uint32_t sin_size;
	int sock = -1, connected, bytes_received;
	struct sockaddr_in server_addr, client_addr;
	uint32_t retry_cnt = 0;

	recv_data = (uint8_t *) os_malloc(iperf_size);
	if (recv_data == NULL) {
		BK_LOGD(NULL, "iperf: no memory\n");
		goto __exit;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		BK_LOGD(NULL, "iperf: socket error\n");
		goto __exit;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(s_param.port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	os_memset(&(server_addr.sin_zero), 0x0, sizeof(server_addr.sin_zero));

	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
		BK_LOGD(NULL, "iperf: unable to bind, err=%d\n", errno);
		goto __exit;
	}

	if (listen(sock, 5) == -1) {
		BK_LOGD(NULL, "iperf: listen error, err=%d\n", errno);
		goto __exit;
	}
	iperf_set_sock_opt(sock);

	while (s_param.state == IPERF_STATE_STARTED) {
		sin_size = sizeof(struct sockaddr_in);
_accept_retry:
		connected = accept(sock, (struct sockaddr *)&client_addr, &sin_size);
		if (connected == -1) {
			if (s_param.state != IPERF_STATE_STARTED)
				break;

			if (errno == EWOULDBLOCK)
				goto _accept_retry;
		}
		BK_LOGD(NULL, "iperf: new client connected from (%s, %d)\n",
				  inet_ntoa(client_addr.sin_addr),
				  ntohs(client_addr.sin_port));

		iperf_set_sock_opt(connected);
		iperf_report_task_start();

		while (s_param.state == IPERF_STATE_STARTED) {
			retry_cnt = 0;
_rx_retry:
			bytes_received = recv(connected, recv_data, iperf_size, 0);
			if (bytes_received < 0) {
				if (s_param.state != IPERF_STATE_STARTED)
					break;

				if (errno == EWOULDBLOCK) {
					retry_cnt ++;
					if (retry_cnt >= IPERF_MAX_RX_RETRY) {
						BK_LOGD(NULL, "iperf: rx reaches max retry(%d)\n", retry_cnt);
						break;
					} else
						goto _rx_retry;
				}
				break;
			}else if (bytes_received == 0){
				s_param.state = IPERF_STATE_STOPPING;
				break;
			}else{
				rtos_lock_mutex(&iperf_mutex);
				s_pkt_delta += bytes_received;
				rtos_unlock_mutex(&iperf_mutex);
			}

		}


		if (connected >= 0)
			closesocket(connected);
		connected = -1;
	}


__exit:
	if (sock >= 0)
		closesocket(sock);

	if (recv_data) {
		os_free(recv_data);
		recv_data = NULL;
	}

	iperf_reset();
	BK_LOGD(NULL, "iperf: iperf is stopped\n");
	rtos_delete_thread(NULL);
}

static void iperf_udp_client(void *thread_param)
{
	int sock, ret;
	uint32_t *buffer;
	struct sockaddr_in server;
	uint32_t tick, packet_count = 0;
	uint32_t retry_cnt;
	int send_size;
	int period_us = 0;
	int fdelay_us = 0;
	int64_t prev_time = 0;
	int64_t send_time = 0;
	uint32_t now_time = 0;
	int32_t tries = 0;
	fd_set rdSet;
	struct timeval tmo;

	send_size = iperf_size > 1470 ? 1470 : iperf_size;
	buffer = os_malloc(iperf_size);
	if (buffer == NULL)
		goto udp_exit;
	os_memset(buffer, 0x00, iperf_size);

	period_us = iperf_bw_delay(send_size);

	while (IPERF_STATE_STARTED == s_param.state) {
		sock = socket(PF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			BK_LOGD(NULL, "iperf: create socket failed, err=%d!\n", errno);
			rtos_delay_milliseconds(1000);
			continue;
		}

		server.sin_family = PF_INET;
		server.sin_port = htons(s_param.port);
		server.sin_addr.s_addr = inet_addr(s_param.host);
		BK_LOGD(NULL, "iperf udp mode run...\n");
#ifndef CONFIG_IPV6
		{
			struct netif *netif;
			netif = get_netif((ip4_addr_t *)&server.sin_addr.s_addr);
			if (netif) {
				etharp_request(netif, (ip4_addr_t *)&server.sin_addr.s_addr);
				rtos_delay_milliseconds(1000);
			}
		}
#endif
		prev_time = rtos_get_time();

		iperf_report_task_start();

		while (IPERF_STATE_STARTED == s_param.state) {
			if (speed_limit > 0) {
				send_time = rtos_get_time();
				fdelay_us = period_us + (int32_t)(prev_time - send_time);
				prev_time = send_time;
			}
			else if (speed_limit == 0) {
				now_time = rtos_get_time();
				if ((now_time - prev_time) / 1000 > 0) {
					prev_time = now_time;
					rtos_delay_milliseconds(4);
				}
			}
			packet_count ++;
			retry_cnt = 0;

			tick = bk_get_tick();
			buffer[0] = htonl(packet_count);
			buffer[1] = htonl(tick / bk_get_ticks_per_second());
			buffer[2] = htonl((tick % bk_get_ticks_per_second()) * 1000);
tx_retry:
			ret = sendto(sock, buffer, send_size, 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
			if (ret) {
				if(ret > 0){
					s_pkt_delta +=ret;
				}
				if (fdelay_us > 0) {
					bk_delay_us(fdelay_us);
				}
			}
			else {
				retry_cnt ++;

				if (IPERF_STATE_STARTED != s_param.state)
					break;

				if (retry_cnt > IPERF_MAX_TX_RETRY){
					break;
				}

				goto tx_retry;
			}

			#if (CONFIG_TASK_WDT)
			bk_task_wdt_feed();
			#endif
		}

		if(s_param.state == IPERF_STATE_STOPPING){
			tick = bk_get_tick();
			buffer[0] = htonl(-(packet_count));
			buffer[1] = htonl(tick / bk_get_ticks_per_second());
			buffer[2] = htonl((tick % bk_get_ticks_per_second()) * 1000);

			while (tries++ < IPERF_UDP_FIN_MAX_RETRY_CNTS){
				ret = sendto(sock, buffer, send_size, 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));

				if (ret < 0) {
					break;
				}

				FD_ZERO(&rdSet);
				FD_SET(sock, &rdSet);
				tmo.tv_sec	= 0;
				tmo.tv_usec = IPERF_UDP_FIN_TO;

				ret = select(sock + 1, &rdSet, NULL, NULL, &tmo);
				if (ret < 0) {
					break;
				} else if (ret == 0) {
					continue;
				} else {
					ret = recv(sock, buffer, iperf_size, 0);
					if (ret < 0) {
						break;
					}

					iperf_process_udpServerHdr(buffer, (uint32_t)ret);
					break;
				}
			}
		}

		closesocket(sock);
		if (IPERF_STATE_STARTED != s_param.state)
			break;

		rtos_delay_milliseconds(1000 * 2);
	}

udp_exit:
	if (buffer) {
		os_free(buffer);
		buffer = NULL;
	}
	iperf_reset();
	BK_LOGD(NULL, "iperf_udp: is stopped\n");
	rtos_delete_thread(NULL);
}

static void iperf_udp_server(void *thread_param)
{
	int sock;
	uint32_t *buffer;
	struct sockaddr_in server;
	struct sockaddr_in sender;
	int sender_len, r_size;
	uint32_t pcount = 0, last_pcount = 0;
	uint64_t s_total_recv_len = 0;
	uint32_t lost, total;
	uint64_t tick1, tick2;
	struct timeval timeout;
	uint32_t start;
	uint32_t end;
	struct sockaddr *from = (struct sockaddr *)&sender;
	socklen_t slen = sizeof(struct sockaddr);
	socklen_t *fromLen = &slen;
	bool udp_start = false;

	buffer = os_malloc(iperf_size);
	if (buffer == NULL)
		return;
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		BK_LOGD(NULL, "can't create socket!! exit\n");
		goto userver_exit;
	}
	server.sin_family = PF_INET;
	server.sin_port = htons(s_param.port);
	server.sin_addr.s_addr = inet_addr("0.0.0.0");

	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		BK_LOGD(NULL, "setsockopt failed!!");
		goto userver_exit;
	}

	if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
		BK_LOGD(NULL, "iperf server bind failed!! exit\n");
		goto userver_exit;
	}

	while (s_param.state == IPERF_STATE_STARTED) {
		tick1 = bk_get_tick();
		tick2 = tick1;

		lost = 0;
		total = 0;
		while (s_param.state == IPERF_STATE_STARTED){
		r_size = recvfrom(sock, buffer, iperf_size, 0, (struct sockaddr *)&sender, (socklen_t *)&sender_len);

		if (r_size > 12 ){
			break;
			}
		}
		if(!udp_start)
		{
			iperf_udpServer_process_firstData(sock, from, *fromLen);
			start = ntohl(buffer[1]);
			udp_start = true;
		}

		iperf_report_task_start();

		while ((s_param.state == IPERF_STATE_STARTED) && ((tick2 - tick1) < (bk_get_ticks_per_second() * 999))) {
			r_size = recvfrom(sock, buffer, iperf_size, 0, (struct sockaddr *)&sender, (socklen_t *)&sender_len);
			if (r_size > 12) {
				pcount = ntohl(buffer[0]);
				if (last_pcount < pcount) {
					lost += pcount - last_pcount - 1;
					total += pcount - last_pcount;
				} else
					last_pcount = pcount;
				last_pcount = pcount;

				rtos_lock_mutex(&iperf_mutex);
				s_pkt_delta +=r_size;
				rtos_unlock_mutex(&iperf_mutex);
				s_total_recv_len +=r_size;


				if ((int32_t)(pcount) < 0) {
					s_param.state = IPERF_STATE_STOPPING;
					break;
				}

			}
			tick2 = bk_get_tick();

			#if (CONFIG_TASK_WDT)
			bk_task_wdt_feed();
			#endif
		}
		end = ntohl(buffer[1]) ;

		if (s_param.state == IPERF_STATE_STOPPING){
			iperf_send_ack_toFin(sock, buffer,s_total_recv_len,start,end);
		}

	}

userver_exit:
	if (sock >= 0)
		closesocket(sock);

	if (buffer) {
		os_free(buffer);
		buffer = NULL;
	}

	iperf_reset();
	BK_LOGD(NULL, "iperf_udp: iperf is stopped\n");
	rtos_delete_thread(NULL);
}

int iperf_param_find_id(int argc, char **argv, char *param)
{
	int i;
	int index;

	index = IPERF_INVALID_INDEX;
	if (NULL == param)
		goto find_over;

	for (i = 1; i < argc; i ++) {
		if (os_strcmp(argv[i], param) == 0) {
			index = i;
			break;
		}
	}

find_over:
	return index;
}

int iperf_param_find(int argc, char **argv, char *param)
{
	int id;
	int find_flag = 0;

	id = iperf_param_find_id(argc, argv, param);
	if (IPERF_INVALID_INDEX != id)
		find_flag = 1;

	return find_flag;
}

void iperf_usage(void)
{
	BK_LOGD(NULL, "Usage: iperf [-s|-c host] [options]\n");
	BK_LOGD(NULL, "       iperf [-h|--stop]\n");
	BK_LOGD(NULL, "\n");
	BK_LOGD(NULL, "Client/Server:\n");
	BK_LOGD(NULL, "  -p #         server port to listen on/connect to\n");
	BK_LOGD(NULL, "\n");
	BK_LOGD(NULL, "Server specific:\n");
	BK_LOGD(NULL, "  -s           run in server mode\n");
	BK_LOGD(NULL, "\n");
	BK_LOGD(NULL, "Client specific:\n");
	BK_LOGD(NULL, "  -c <host>    run in client mode, connecting to <host>\n");
	BK_LOGD(NULL, "\n");
	BK_LOGD(NULL, "Miscellaneous:\n");
	BK_LOGD(NULL, "  -u           udp support, and the default mode is tcp\n");
	BK_LOGD(NULL, "  -t #[time]      time in seconds to transmit for (default 30 secs)\n");
	BK_LOGD(NULL, "  -h           print this message and quit\n");
	BK_LOGD(NULL, "  --stop       stop iperf program\n");

	return;
}

static void iperf_stop(void)
{
	if (s_param.state == IPERF_STATE_STARTED) {
		s_param.state = IPERF_STATE_STOPPING;
		BK_LOGD(NULL, "iperf: iperf is stopping...\n");
	}
}

static void iperf_start(int mode, char *host, int port)
{
	if (s_param.state == IPERF_STATE_STOPPED) {
		s_param.state = IPERF_STATE_STARTED;
		s_param.mode = mode;
		s_param.port = port;
		if (s_param.host) {
			os_free(s_param.host);
			s_param.host = NULL;
		}

		if (host)
			s_param.host = os_strdup(host);

		if (mode == IPERF_MODE_TCP_CLIENT) {
#ifdef CONFIG_FREERTOS_SMP
			rtos_smp_create_thread(NULL, iperf_priority, "iperf_tcp_c",
							   iperf_client, THREAD_SIZE,
							   (beken_thread_arg_t) 0);
#else
			rtos_create_thread(NULL, iperf_priority, "iperf_tcp_c",
							   iperf_client, THREAD_SIZE,
							   (beken_thread_arg_t) 0);
#endif
		} else if (mode == IPERF_MODE_TCP_SERVER) {
#ifdef CONFIG_FREERTOS_SMP
			rtos_smp_create_thread(NULL, iperf_priority, "iperf_tcp_s",
							   iperf_server, THREAD_SIZE,
							   (beken_thread_arg_t) 0);
#else
			rtos_create_thread(NULL, iperf_priority, "iperf_tcp_s",
							   iperf_server, THREAD_SIZE,
							   (beken_thread_arg_t) 0);
#endif
		} else if (mode == IPERF_MODE_UDP_CLIENT) {
#ifdef CONFIG_FREERTOS_SMP
			rtos_smp_create_thread(NULL, iperf_priority, "iperf_udp_c",
							   iperf_udp_client, THREAD_SIZE,
							   (beken_thread_arg_t) 0);
#else
			rtos_create_thread(NULL, iperf_priority, "iperf_udp_c",
							   iperf_udp_client, THREAD_SIZE,
							   (beken_thread_arg_t) 0);
#endif
		} else if (mode == IPERF_MODE_UDP_SERVER) {
#ifdef CONFIG_FREERTOS_SMP
			rtos_smp_create_thread(NULL, iperf_priority, "iperf_udp_s",
							   iperf_udp_server, THREAD_SIZE,
							   (beken_thread_arg_t) 0);
#else
			rtos_create_thread(NULL, iperf_priority, "iperf_udp_s",
							   iperf_udp_server, THREAD_SIZE,
							   (beken_thread_arg_t) 0);
#endif
		} else
			BK_LOGD(NULL, "iperf: invalid iperf mode=%d\n", mode);
	} else if (s_param.state == IPERF_STATE_STOPPING)
		BK_LOGD(NULL, "iperf: iperf is stopping, try again later!\n");
	else
		BK_LOGD(NULL, "iperf: iperf is running, stop first!\n");
}
void iperf_config(int argc, char **argv)
{
    if (os_strcmp(argv[1], "config"))
    {
        return;
    }


    if(os_strcmp(argv[2], "-pri") == 0)
    {
        iperf_priority = os_strtoul(argv[3], NULL, 10);
        BK_LOGD(NULL, "iperf config iperf_priority to %d !\n", iperf_priority);
    }
    else if(os_strcmp(argv[2], "-ips") == 0)
    {
        iperf_size = os_strtoul(argv[3], NULL, 10);
        BK_LOGD(NULL, "iperf config iperf_size to %d !\n", iperf_size);
    }
    else if(os_strcmp(argv[2], "-tos") == 0)
    {
        iperf_tos = os_strtoul(argv[3], NULL, 10);
        BK_LOGD(NULL, "iperf config iperf_tos to %d !\n", iperf_tos);
    }
    else
    {
        BK_LOGD(NULL, "iperf config INVALID PRAMATER !\n");
    }
}

static void iperf_set_defaults(void) {
	iperf_size = IPERF_BUFSZ;
	speed_limit = IPERF_DEFAULT_SPEED_LIMIT;
}

void iperf(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int id, mode;
	char *host = NULL;
	int is_udp_flag = 0;
	int port = IPERF_PORT;
	int is_server_mode, is_client_mode;
	uint32_t value;

	iperf_set_defaults();
	/* check parameters of command line*/
	if (iperf_param_find(argc, argv, "-h") || (argc == 1))
		goto __usage;
	else if (iperf_param_find(argc, argv, "--stop")
			 || iperf_param_find(argc, argv, "-stop")) {
		iperf_stop();
		return;
	}
	else if(iperf_param_find(argc, argv, "config"))
	{
		iperf_config(argc, argv);
		return;
	}

	is_server_mode = iperf_param_find(argc, argv, "-s");
	is_client_mode = iperf_param_find(argc, argv, "-c");
	if ((is_client_mode && is_server_mode)
		|| ((0 == is_server_mode) && (0 == is_client_mode)))
		goto __usage;

	if (iperf_param_find(argc, argv, "-u"))
		is_udp_flag = 1;

	/* config iperf operation mode*/
	if (is_udp_flag) {
		if (is_server_mode)
			mode = IPERF_MODE_UDP_SERVER;
		else
			mode = IPERF_MODE_UDP_CLIENT;
	} else {
		if (is_server_mode)
			mode = IPERF_MODE_TCP_SERVER;
		else
			mode = IPERF_MODE_TCP_CLIENT;
	}

	/* config protocol port*/
	id = iperf_param_find_id(argc, argv, "-p");
	if (IPERF_INVALID_INDEX != id) {
		port = atoi(argv[id + 1]);

		if (argc - 1 < id + 1)
			goto __usage;
	}

	if (is_client_mode) {
		id = iperf_param_find_id(argc, argv, "-c");
		if (IPERF_INVALID_INDEX != id) {
			host = argv[id + 1];

			if (argc - 1 < id + 1)
				goto __usage;
		}
	}

	id = iperf_param_find_id(argc, argv, "-l");
	if (IPERF_INVALID_INDEX != id) {
		iperf_size = atoi(argv[id + 1]);

		if ((iperf_size == 0) || argc - 1 < id + 1)
			goto __usage;
	}

	if (is_client_mode) {
		id = iperf_param_find_id(argc, argv, "-t");
		if (IPERF_INVALID_INDEX != id) {
			s_time = atoi(argv[id + 1]);
			if(s_time == 0){
				s_time = IPERF_DEFAULT_TIME;
				}

			if (argc - 1 < id + 1)
				goto __usage;
		}
	}


	id = iperf_param_find_id(argc, argv, "-b");
	if (IPERF_INVALID_INDEX != id) {
		if (argv[id + 1] == NULL) {
			speed_limit = 0;
		}
		else {
			speed_limit = atoi(argv[id + 1]);

			if ((speed_limit == 0) || argc - 1 < id + 1)
				goto __usage;

			value = strlen(argv[id + 1]);
			if (value > 1) {
				if (argv[id + 1][value - 1] == 'k') {
					speed_limit *= 1000;
				} else if (argv[id + 1][value - 1] == 'K') {
					speed_limit *= 1024;
				} else if (argv[id + 1][value - 1] == 'm') {
					speed_limit *= 1000 * 1000;
				} else if (argv[id + 1][value - 1] == 'M') {
					speed_limit *= 1024 * 1024;
				} else {
					goto __usage;
				}
			}
		}
	}
	rtos_init_mutex(&iperf_mutex);
	iperf_start(mode, host, port);

	return;

__usage:
	iperf_usage();

	return;
}
// eof

