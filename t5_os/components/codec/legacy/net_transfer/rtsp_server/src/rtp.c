/*
 * @Author: cijliu
 * @Date: 2021-02-04 16:35:26
 * @LastEditTime: 2021-02-26 16:39:31
 */
#include "include/rtp.h"
#include "include/h264.h"
#define RTP_SSRC_DEFAULT    (0xd160b58f)
#define RTP_PAYLOAD_DEFAULT (RTP_H264)
#define RTP_MALLOC_REGISTER(hdr)    switch(hdr->pt){
#define RTP_REGISTER(pt, func)              \
{                                           \
    case pt:                                \
    return func(header, data, len);         \
}

#define RTP_MALLOC_REGISTER_END()           }
// void rtp_set_ssrc(rtp_header_t *header, uint32_t ssrc)
// {
//     RTP_ASSERT(header);
//     header->ssrc = htonl(ssrc);
// }

// void rtp_set_pt(rtp_header_t *header, rtp_pt_enum_t e)
// {
//     RTP_ASSERT(header);
//     header->pt = e;
// }

// void rtp_set_ts(rtp_header_t *header, uint32_t ts)
// {
//     RTP_ASSERT(header);
//     header->ts = htonl(ts);
// }

// void rtp_set_seq(rtp_header_t *header, uint16_t seq)
// {
//     RTP_ASSERT(header);
//     header->seq = htons(seq);
// }

void rtp_header_init(rtp_header_t *header)
{
    RTP_ASSERT(header);
    memset(header, 0, sizeof(rtp_header_t));
    header->v = 2;
    header->pt = RTP_PAYLOAD_DEFAULT;
    header->ssrc = htonl(RTP_SSRC_DEFAULT);
}

rtp_packet_t* rtp_packet_malloc(rtp_header_t *header, uint8_t *data, uint32_t len)
{
    RTP_ASSERT(header);
    RTP_ASSERT(data);
    
    RTP_MALLOC_REGISTER(header)
    RTP_REGISTER(RTP_H264, rtp_h264_packet_malloc)
    RTP_MALLOC_REGISTER_END()
    
}

void rtp_packet_free(rtp_packet_t *pkt)
{
    if(pkt == NULL) return;
    rtp_packet_t *p = pkt;
    while(p){
        rtp_packet_t *next = p->next;
        free(p);
        p = next;
    }
    pkt == NULL;
}
