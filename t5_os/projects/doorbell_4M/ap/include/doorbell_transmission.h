#ifndef __DOORBELL_TRANSMISSION_H__
#define __DOORBELL_TRANSMISSION_H__


/*
*   Magic code  2 bytes
*   Flags       2 bytes
*   Timestamp   4 bytes
*   Squence     2 bytes
*   Length      2 bytes
*   CRC         1 byte
*   RESERVED    3 byte
*/
typedef struct
{
	uint16_t magic;
	uint16_t flags;
	uint32_t timestamp;
	uint16_t sequence;
	uint16_t length;
	uint8_t crc;
	uint8_t reserved[3];
	uint8_t  payload[];
} __attribute__((__packed__)) db_trans_head_t;

#define HEAD_SIZE_TOTAL             (sizeof(db_trans_head_t))

#define HEAD_MAGIC_CODE             (0xF0D5)
#define HEAD_FLAGS_CRC              (1 << 0)


typedef int (*doorbell_transmission_send_t)(uint8_t *data, uint16_t length);


typedef struct
{
	doorbell_transmission_send_t tsend;
} db_channel_cb_t;


typedef struct
{
	uint8_t *cbuf;
	uint16_t csize;
	uint16_t ccount;
	uint16_t sequence;
	db_trans_head_t *tbuf;
	const db_channel_cb_t *cb;
	uint16_t tsize;
} db_channel_t;


typedef void (*doorbell_transmission_recive_cb_t)(db_channel_t *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length);

void doorbell_transmission_unpack(db_channel_t *channel, uint8_t *data, uint32_t length, doorbell_transmission_recive_cb_t cb);
int doorbell_transmission_pack_send(db_channel_t *channel, uint8_t *data, uint32_t length, doorbell_transmission_send_t cb);
void doorbell_transmission_pack(db_channel_t *channel, uint8_t *data, uint32_t length);

db_channel_t *doorbell_transmission_malloc(uint16_t max_rx_size, uint16_t max_tx_size);


#endif
