/*
 * @Author: cijliu
 * @Date: 2021-02-04 16:29:39
 * @LastEditTime: 2021-02-26 16:38:24
 */
#ifndef __RTP_H__
#define __RTP_H__
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#define __RTP_BIG_ENDIAN        (1)
#define __RTP_LITTLE_ENDIAN     (0)
#define __RTP_ENDIAN_MODE     __RTP_LITTLE_ENDIAN
#define RTP_MAX_PAYLOAD			(1280)
#define RTP_RBSP_MAX_LEN		(RTP_MAX_PAYLOAD-14)
#define RTP_ASSERT(x)   do{assert(x != NULL);}while(0)
typedef enum {
    RTP_H264 = 97,
} rtp_pt_enum_t;
typedef struct {
#if __RTP_ENDIAN_MODE == __RTP_BIG_ENDIAN
	uint16_t v:2;
	uint16_t p:1;
	uint16_t x:1;
	uint16_t cc:4;
	uint16_t m:1;
	uint16_t pt:7;
#else
	uint16_t cc:4;
	uint16_t x:1;
	uint16_t p:1;
	uint16_t v:2;
	uint16_t pt:7;
	uint16_t m:1;
#endif
	uint16_t seq;
	uint32_t ts;
	uint32_t ssrc;
} rtp_header_t;
typedef struct rtp_packet{
	union {
		uint8_t data[RTP_MAX_PAYLOAD];
		struct {
			rtp_header_t hdr;
			uint8_t fu_indicator;
			uint8_t fu_header;
			uint8_t rbsp[RTP_RBSP_MAX_LEN];
		} nalu;
	};
	struct rtp_packet *next;
	uint64_t len;
}rtp_packet_t;

void rtp_header_init(rtp_header_t *header);
rtp_packet_t* rtp_packet_malloc(rtp_header_t *header, uint8_t *data, uint32_t len);
void rtp_packet_free(rtp_packet_t *pkt);
#endif