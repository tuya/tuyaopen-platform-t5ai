#ifndef __AV_CLIENT_COMM_H__
#define __AV_CLIENT_COMM_H__

#include "lwip/sockets.h"
#include "net.h"

#define AV_CLIENT_UDP_IMG_PORT           (7180)
#define AV_CLIENT_UDP_AUD_PORT           (7170)

#define AV_CLIENT_TCP_IMG_PORT           (7150)
#define AV_CLIENT_TCP_AUD_PORT           (7140)

#define AV_CLIENT_NETWORK_MAX_SIZE       (1472)

#define TRANSMISSION_BIG_ENDIAN (BK_FALSE)


#if TRANSMISSION_BIG_ENDIAN == BK_TRUE
#define CHECK_ENDIAN_UINT32(var)    htonl(var)
#define CHECK_ENDIAN_UINT16(var)    htons(var)

#define STREAM_TO_UINT16(u16, p) {u16 = (((uint16_t)(*((p) + 1))) + (((uint16_t)(*((p)))) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32, p) {u32 = ((((uint32_t)(*((p) + 3)))) + ((((uint32_t)(*((p) + 2)))) << 8) + ((((uint32_t)(*((p) + 1)))) << 16) + ((((uint32_t)(*((p))))) << 24)); (p) += 4;}


#else
#define CHECK_ENDIAN_UINT32
#define CHECK_ENDIAN_UINT16

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32, p) {u32 = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) + ((((uint32_t)(*((p) + 3)))) << 24)); (p) += 4;}


#endif

#define STREAM_TO_UINT8(u8, p) {u8 = (uint8_t)(*(p)); (p) += 1;}


#define AV_CLIENT_SEND_MAX_RETRY (2000)
#define AV_CLIENT_SEND_MAX_DELAY (20)


int av_client_socket_sendto(int *fd, const struct sockaddr *dst, uint8_t *data, uint32_t length, int offset, uint16_t *cnt);

#endif
