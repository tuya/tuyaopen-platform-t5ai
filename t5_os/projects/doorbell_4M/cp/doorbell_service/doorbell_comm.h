#ifndef __DOORBELL_COMM_H__
#define __DOORBELL_COMM_H__


#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32, p) {u32 = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) + ((((uint32_t)(*((p) + 3)))) << 24)); (p) += 4;}
#define STREAM_TO_UINT8(u8, p) {u8 = (uint8_t)(*(p)); (p) += 1;}

#define UINT8_TO_STREAM(p, u8)   do{*(p)++ = (uint8_t)(u8);} while(0)
#define UINT16_TO_STREAM(p, u16) do{*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);} while(0)
#define UINT32_TO_STREAM(p, u32) do{*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);} while(0)
#define ARRAY_TO_STREAM(p, s, size) do{os_memcpy(p, s, size); p+= size; } while(0)


typedef enum
{
    DBEVT_DATA_TO_USER = 0x01,
    DBEVT_DATA_TO_APK = 0x02,
    DBEVT_CLOSE_BLUETOOTH = 0x03,

    DBEVT_EXIT = 0xE1,
} dbevt_t;


typedef struct
{
    uint32_t event;
    uint8_t *data;
    uint16_t len;
} doorbell_msg_t;

bk_err_t doorbell_send_msg(doorbell_msg_t *msg);
void doorbell_core_init(void);


#endif
