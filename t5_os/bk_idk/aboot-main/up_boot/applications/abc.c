#include "abc.h"

/*
	pn15_gen u_pn15(.pn_out(pn15),.addr(pn15_addr),.rstn(rstn),.clk(clk),.bps(pn15_bps));
*/
u16 pn15_gen_u_pn15(u16 addr, u8 bps)
{
    u16 pn_out;
    if(bps)
        pn_out = 0;
    else
    {
        /*
        	wire [15:0]   pn_tmp = {addr[6:0], addr[15:7]} ^ {16'h6371 & {4{addr[8:5]}}};
        */
        u16 a, b;
        a = ((addr & 0x7f) << 9) + ((addr >> 7) & 0x1ff);
        b = 0x6371 & ( (((addr >> 5) & 0xf) << 12) + (((addr >> 5) & 0xf) << 8)
                       + (((addr >> 5) & 0xf) << 4) + (((addr >> 5) & 0xf) << 0) );
        pn_out = a ^ b;
    }
    return pn_out;
}

/*
	pn16_gen u_pn16(.pn_out(pn16),.addr(pn16_addr),.rstn(rstn),.clk(clk),.bps(pn16_bps));
*/
u32 pn16_gen_u_pn16(u32 addr, u8 bps)
{
    u32 pn_out;
    if(bps)
        pn_out = 0;
    else
    {
        /*
        	wire [16:0]   pn_tmp = {addr[9:0], addr[16:10]} ^ {17'h13659 & {addr[4],{4{addr[1],addr[5],addr[9],addr[13]}}}};
        */
        u32 a, b, c;
        a = ((addr & 0x3ff) << 7) + ((addr >> 10) & 0x7f);
        b = (((addr >> 1) & 0x1) << 3) + (((addr >> 5) & 0x1) << 2) + (((addr >> 9) & 0x1) << 1) + (((addr >> 13) & 0x1) << 0);
        c = 0x13659 & ( (((addr >> 4) & 0x1) << 16) + (b << 12) + (b << 8) + (b << 4) + b );
        pn_out = a ^ c;
    }
    return pn_out;
}

/*
	pn32_gen u_pn32(.pn_out(pn32),.addr(pn32_addr),.rstn(rstn),.clk(clk),.bps(pn32_bps));
*/
u32 pn32_gen_u_pn32(u32 addr, u8 bps)
{
    u32 pn_out;
    if(bps)
        pn_out = 0;
    else
    {
        /*
        	wire [31:0]   pn_tmp = {addr[14:0], addr[31:15]} ^ {32'hE519A4F1 & {8{addr[5:2]}}};
        */
        u32 a, b, c;
        a = ((addr & 0x7fff) << 17) + ((addr >> 15) & 0x1ffff);
        b = ((addr >> 2) & 0xf);
        c = 0xE519A4F1 & ( (b << 28) + (b << 24) + (b << 20) + (b << 16) + (b << 12) + (b << 8) + (b << 4) + b );
        pn_out = a ^ c;
    }
    return pn_out;
}

/* 控制变量是上电初始化时从内部OTP读出 */

u32 enc_data_my(u32 addr, u32 raw_data)
{
    u8 g_bypass;
    u8 pn15_bps;
    u8 pn16_bps;
    u8 pn32_bps;
    u8 rand_bps;

    u8 pn16_bit;
    u8 pn15_sel;
    u8 pn16_sel;
    u8 pn32_sel;

    u16 pn15_A;
    u32 pn16_A;
    u32 pn32_A;

    u16 pn15_addr;
    u32 pn16_addr;
    u32 pn32_addr;

    u16 pn15;
    u16 pn16;
    u32 pn32;
    u32 random;
    u32 pnout;
    u32 enc_dat;
    
    //printf("raw_data: 0x%x ", raw_data);
    
    /*
    	wire      g_bypass = (coef3[31:24]==8'hFF) |
                         	(coef3[31:24]==8'h00) ;
    */
    enc_dat = coef3 & 0xff000000;
    if( (enc_dat == 0xff000000) || (enc_dat == 0x0) )
        g_bypass = 1;
    else
        g_bypass = 0;
    /*
    	wire      pn15_bps = g_bypass | coef3[0] ;
    	wire      pn16_bps = g_bypass | coef3[1] ;
    	wire      pn32_bps = g_bypass | coef3[2] ;
    	wire      rand_bps = g_bypass | coef3[3] ;
    */
    if( g_bypass || (coef3 & 0x1))
        pn15_bps = 1;
    else
        pn15_bps = 0;

    if( g_bypass || (coef3 & 0x2))
        pn16_bps = 1;
    else
        pn16_bps = 0;

    if( g_bypass || (coef3 & 0x4))
        pn32_bps = 1;
    else
        pn32_bps = 0;

    if( g_bypass || (coef3 & 0x8))
        rand_bps = 1;
    else
        rand_bps = 0;
    /*
    	wire      pn16_bit = coef3[    4] ;
    	wire[1:0] pn15_sel = coef3[ 6: 5] ;
    	wire[1:0] pn16_sel = coef3[ 9: 8] ;
    	wire[1:0] pn32_sel = coef3[12:11] ;
    */
    pn16_bit = (coef3 >> 4) & 0x1;
    pn15_sel = (coef3 >> 5) & 0x3;
    pn16_sel = (coef3 >> 8) & 0x3;
    pn32_sel = (coef3 >> 11) & 0x3;

    /*
    	wire[15:0] pn15_A  = (pn15_sel==0) ? ({addr[31:24],addr[23:16]} ^ {addr[15:8],addr[7:0]}) :
    	                     (pn15_sel==1) ? ({addr[31:24],addr[23:16]} ^ {addr[7:0],addr[15:8]}) :
    	                     (pn15_sel==2) ? ({addr[23:16],addr[31:24]} ^ {addr[15:8],addr[7:0]}) :
    	                                     ({addr[23:16],addr[31:24]} ^ {addr[7:0],addr[15:8]}) ;
    */
    if(pn15_sel == 0)
    {
        u16 a, b;
        a = (((addr >> 24) & 0xff) << 8) + ((addr >> 16) & 0xff);
        b = (((addr >> 8 ) & 0xff) << 8) + ((addr >> 0 ) & 0xff);
        pn15_A = a ^ b;
    }
    else if(pn15_sel == 1)
    {
        u16 a, b;
        a = (((addr >> 24) & 0xff) << 8) + ((addr >> 16) & 0xff);
        b = (((addr >> 0 ) & 0xff) << 8) + ((addr >> 8 ) & 0xff);
        pn15_A = a ^ b;
    }
    else if(pn15_sel == 2)
    {
        u16 a, b;
        a = (((addr >> 16) & 0xff) << 8) + ((addr >> 24) & 0xff);
        b = (((addr >> 8 ) & 0xff) << 8) + ((addr >> 0 ) & 0xff);
        pn15_A = a ^ b;
    }
    else
    {
        u16 a, b;
        a = (((addr >> 16) & 0xff) << 8) + ((addr >> 24) & 0xff);
        b = (((addr >> 0 ) & 0xff) << 8) + ((addr >> 8 ) & 0xff);
        pn15_A = a ^ b;
    }

    /*
    	wire[16:0] pn16_A  = (pn16_sel==0) ?   addr[16:0] :
    	                     (pn16_sel==1) ?   addr[17:1] :
    	                     (pn16_sel==2) ?   addr[18:2] :
    	                                       addr[19:3] ;
    */
    if(pn16_sel == 0)
        pn16_A = (addr >> 0) & 0x1ffff;
    else if(pn16_sel == 1)
        pn16_A = (addr >> 1) & 0x1ffff;
    else if(pn16_sel == 2)
        pn16_A = (addr >> 2) & 0x1ffff;
    else
        pn16_A = (addr >> 3) & 0x1ffff;

    /*
    	wire[31:0] pn32_A  = (pn32_sel==0) ?   addr[31:0] :
    	                     (pn32_sel==1) ?  {addr[ 7:0],addr[31: 8]} :
    	                     (pn32_sel==2) ?  {addr[15:0],addr[31:16]} :
    	                                      {addr[23:0],addr[31:24]} ;
    */
    if(pn32_sel == 0)
        pn32_A = addr;
    else if(pn32_sel == 1)
        pn32_A = (((addr >> 0) & 0xff    ) << 24) + ((addr >> 8 ) & 0xffffff);
    else if(pn32_sel == 2)
        pn32_A = (((addr >> 0) & 0xffff  ) << 16) + ((addr >> 16) & 0xffff);
    else
        pn32_A = (((addr >> 0) & 0xffffff) << 8 ) + ((addr >> 24) & 0xff);


    /*
    	wire[15:0] pn15_addr = pn15_A ^ coef1[31:16] ;
    	wire[16:0] pn16_addr = pn16_A ^ {coef1[15:8], pn16_bit, coef1[7:0]} ;
    	wire[31:0] pn32_addr = pn32_A ^ coef0[31:0] ;
    */
    pn15_addr = pn15_A ^ ((coef1 >> 16) & 0xffff);
    pn16_addr = pn16_A ^ (((coef1 >> 8) & 0xff) << 9) + (pn16_bit << 8) + (coef1 & 0xff);
    pn32_addr = pn32_A ^ coef0;

    /*
    	pn15_gen u_pn15(.pn_out(pn15),.addr(pn15_addr),.rstn(rstn),.clk(clk),.bps(pn15_bps));
    	pn16_gen u_pn16(.pn_out(pn16),.addr(pn16_addr),.rstn(rstn),.clk(clk),.bps(pn16_bps));
    	pn32_gen u_pn32(.pn_out(pn32),.addr(pn32_addr),.rstn(rstn),.clk(clk),.bps(pn32_bps));
    */
    pn15 = pn15_gen_u_pn15(pn15_addr, pn15_bps);
    pn16 = pn16_gen_u_pn16(pn16_addr, pn16_bps);
    pn32 = pn32_gen_u_pn32(pn32_addr, pn32_bps);

    /*
    	wire[31:0] random = rand_bps ? 32'h00000000 : coef2[31:0] ;
    */
    if(rand_bps == 1)
        random = 0;
    else
        random = coef2;
    /*
    	assign     pnout  = pn32[31:0] ^ {pn15[15:0],pn16[15:0]} ^ random[31:0] ;
    */

    pnout = pn32 ^ ((pn15 << 16) + (pn16 & 0xffff)) ^ random;

    /* 最后，明文与加密字异或 */
    enc_dat  = raw_data ^ pnout;

    //printf("enc_dat: 0x%x\r\n", enc_dat);
    
    return enc_dat;
}
// eof

