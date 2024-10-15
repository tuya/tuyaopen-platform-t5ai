#ifndef __ABC_INCLUDE_H__
#define __ABC_INCLUDE_H__

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned int   BOOL;

u32 enc_data_my(u32 addr, u32 raw_data);

extern u32 coef0;
extern u32 coef1;
extern u32 coef2;
extern u32 coef3;
#endif // __ABC_INCLUDE_H__
// eof

