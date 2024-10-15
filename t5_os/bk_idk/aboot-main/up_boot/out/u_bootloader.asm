
./out/u_bootloader.elf:     file format elf32-littleriscv


Disassembly of section .nds_vector:

00001f00 <reset_vector>:
    1f00:	342022f3          	csrr	t0,mcause
    1f04:	00028463          	beqz	t0,1f0c <_start>
    1f08:	05a0006f          	j	1f62 <nmi_handler>

00001f0c <_start>:
    1f0c:	7ca027f3          	csrr	a5,mcache_ctl
    1f10:	0017e793          	ori	a5,a5,1
    1f14:	7ca79073          	csrw	mcache_ctl,a5
    1f18:	7ca027f3          	csrr	a5,mcache_ctl
    1f1c:	0027e793          	ori	a5,a5,2
    1f20:	7ca79073          	csrw	mcache_ctl,a5
    1f24:	2ffff197          	auipc	gp,0x2ffff
    1f28:	9d418193          	addi	gp,gp,-1580 # 300008f8 <__global_pointer$>
    1f2c:	3005e297          	auipc	t0,0x3005e
    1f30:	0d428293          	addi	t0,t0,212 # 30060000 <_stack>
    1f34:	8116                	c.mv	sp,t0
    1f36:	00005297          	auipc	t0,0x5
    1f3a:	d8a28293          	addi	t0,t0,-630 # 6cc0 <_ITB_BASE_>
    1f3e:	80029073          	csrw	uitb,t0
    1f42:	00001297          	auipc	t0,0x1
    1f46:	ae228293          	addi	t0,t0,-1310 # 2a24 <trap_entry>
    1f4a:	30529073          	csrw	mtvec,t0
    1f4e:	00001097          	auipc	ra,0x1
    1f52:	9da080e7          	jalr	-1574(ra) # 2928 <__platform_init>
    1f56:	00001097          	auipc	ra,0x1
    1f5a:	9ba080e7          	jalr	-1606(ra) # 2910 <reset_handler>
    1f5e:	a001                	c.j	1f5e <_start+0x52>
    1f60:	8082                	c.jr	ra

00001f62 <nmi_handler>:
    1f62:	09e0e06f          	j	10000 <_data_lmastart+0x66ba>
    1f66:	a001                	c.j	1f66 <nmi_handler+0x4>
	...
    2000:	4b42                	c.lwsp	s6,16(sp)
    2002:	36353237          	lui	tp,0x36353
	...
    200e:	0000                	unimp
    2010:	6568                	c.flw	fa0,76(a0)
    2012:	6461                	c.lui	s0,0x18
	...

Disassembly of section .text:

00002030 <GPIO_UART_function_enable>:
    2030:	ed0d                	c.bnez	a0,206a <GPIO_UART_function_enable+0x3a>
    2032:	440107b7          	lui	a5,0x44010
    2036:	44000eb7          	lui	t4,0x44000
    203a:	07c00f13          	li	t5,124
    203e:	43eea423          	sw	t5,1064(t4) # 44000428 <_stack+0x13fa0428>
    2042:	08078293          	addi	t0,a5,128 # 44010080 <_stack+0x13fb0080>
    2046:	07800f93          	li	t6,120
    204a:	43fea623          	sw	t6,1068(t4)
    204e:	7365                	c.lui	t1,0xffff9
    2050:	0442a703          	lw	a4,68(t0)
    2054:	8ff30393          	addi	t2,t1,-1793 # ffff88ff <_stack+0xcff988ff>
    2058:	00777533          	and	a0,a4,t2
    205c:	04a2a223          	sw	a0,68(t0)
    2060:	0442a583          	lw	a1,68(t0)
    2064:	04b2a223          	sw	a1,68(t0)
    2068:	8082                	c.jr	ra
    206a:	02156b5b          	bnec	a0,1,20a0 <GPIO_UART_function_enable+0x70>
    206e:	440003b7          	lui	t2,0x44000
    2072:	04800513          	li	a0,72
    2076:	44010637          	lui	a2,0x44010
    207a:	40a3a023          	sw	a0,1024(t2) # 44000400 <_stack+0x13fa0400>
    207e:	08060693          	addi	a3,a2,128 # 44010080 <_stack+0x13fb0080>
    2082:	07800593          	li	a1,120
    2086:	40b3a223          	sw	a1,1028(t2)
    208a:	0406a803          	lw	a6,64(a3)
    208e:	f8887893          	andi	a7,a6,-120
    2092:	0516a023          	sw	a7,64(a3)
    2096:	0406ae03          	lw	t3,64(a3)
    209a:	05c6a023          	sw	t3,64(a3)
    209e:	8082                	c.jr	ra
    20a0:	00256e5b          	bnec	a0,2,20bc <GPIO_UART_function_enable+0x8c>
    20a4:	440107b7          	lui	a5,0x44010
    20a8:	0d47a703          	lw	a4,212(a5) # 440100d4 <_stack+0x13fb00d4>
    20ac:	08078293          	addi	t0,a5,128
    20b0:	04e2aa23          	sw	a4,84(t0)
    20b4:	0542a303          	lw	t1,84(t0)
    20b8:	0462aa23          	sw	t1,84(t0)
    20bc:	8082                	c.jr	ra

000020be <uart0_init>:
    20be:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    20c0:	8074                	exec.it	#57     !lui	a5,0x44820
    20c2:	8470                	exec.it	#51     !sw	zero,0(a5) # 44820000 <_stack+0x147c0000>
    20c4:	8464                	exec.it	#58     !li	s0,225
    20c6:	cd19                	c.beqz	a0,20e4 <uart0_init+0x26>
    20c8:	8624                	exec.it	#90     !lui	ra,0x18cc
    20ca:	8c70                	exec.it	#55     !srli	s0,a0,0x1
    20cc:	8220                	exec.it	#80     !addi	t0,ra,-1409 # 18cba7f <SRAM2_SIZE+0x18b1357>
    20ce:	8a44                	exec.it	#108     !add	t1,s0,t0
    20d0:	8a34                	exec.it	#93     !divu	t2,t1,a0
    20d2:	4491                	c.li	s1,4
    20d4:	8e20                	exec.it	#86     !addi	s0,t2,-1
    20d6:	00947363          	bgeu	s0,s1,20dc <uart0_init+0x1e>
    20da:	4411                	c.li	s0,4
    20dc:	6509                	c.lui	a0,0x2
    20de:	00a46363          	bltu	s0,a0,20e4 <uart0_init+0x26>
    20e2:	8a50                	exec.it	#101     !addi	s0,a0,-1 # 1fff <nmi_handler+0x9d>
    20e4:	8620                	exec.it	#82     !lui	s1,0x44010
    20e6:	588c                	c.lw	a1,48(s1)
    20e8:	4501                	c.li	a0,0
    20ea:	0045e613          	ori	a2,a1,4
    20ee:	d890                	c.sw	a2,48(s1)
    20f0:	3781                	c.jal	2030 <GPIO_UART_function_enable>
    20f2:	670d                	c.lui	a4,0x3
    20f4:	448206b7          	lui	a3,0x44820
    20f8:	04070813          	addi	a6,a4,64 # 3040 <bsdiff_flash_init+0xa>
    20fc:	0106a223          	sw	a6,4(a3) # 44820004 <_stack+0x147c0004>
    2100:	04200893          	li	a7,66
    2104:	0116a823          	sw	a7,16(a3)
    2108:	0006ac23          	sw	zero,24(a3)
    210c:	00841e13          	slli	t3,s0,0x8
    2110:	0006ae23          	sw	zero,28(a3)
    2114:	01be6413          	ori	s0,t3,27
    2118:	c280                	c.sw	s0,0(a3)
    211a:	8201a223          	sw	zero,-2012(gp) # 3000011c <uart0_rx_done>
    211e:	8201a023          	sw	zero,-2016(gp) # 30000118 <uart0_rx_index>
    2122:	8874                	exec.it	#61     !addi	s1,s1,128 # 44010080 <_stack+0x13fb0080>
    2124:	8230                	exec.it	#81     !lw	t4,0(s1)
    2126:	010eef13          	ori	t5,t4,16
    212a:	01e4a023          	sw	t5,0(s1)
    212e:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>

00002130 <uart0_disable>:
    2130:	8074                	exec.it	#57     !lui	a5,0x44820
    2132:	4398                	c.lw	a4,0(a5)
    2134:	76f5                	c.lui	a3,0xffffd
    2136:	8a24                	exec.it	#92     !andi	t0,a4,-28
    2138:	8830                	exec.it	#21     !sw	t0,0(a5) # 44820000 <_stack+0x147c0000>
    213a:	8250                	exec.it	#97     !lw	t1,4(a5)
    213c:	8064                	exec.it	#56     !addi	t2,a3,-65 # ffffcfbf <_stack+0xcff9cfbf>
    213e:	8204                	exec.it	#72     !and	a0,t1,t2
    2140:	c3c8                	c.sw	a0,4(a5)
    2142:	4b8c                	c.lw	a1,16(a5)
    2144:	8c64                	exec.it	#62     !lui	a6,0x44010
    2146:	8270                	exec.it	#113     !addi	a7,a6,128 # 44010080 <_stack+0x13fb0080>
    2148:	8a10                	exec.it	#69     !andi	a2,a1,-67
    214a:	cb90                	c.sw	a2,16(a5)
    214c:	8424                	exec.it	#26     !lw	t3,0(a7)
    214e:	fefe7e93          	andi	t4,t3,-17
    2152:	01d8a023          	sw	t4,0(a7)
    2156:	03082f03          	lw	t5,48(a6)
    215a:	ffbf7f93          	andi	t6,t5,-5
    215e:	03f82823          	sw	t6,48(a6)
    2162:	8082                	c.jr	ra

00002164 <UART0_InterruptHandler>:
    2164:	8060                	exec.it	#48     !jal	t0,5d3a <__riscv_save_4>
    2166:	8074                	exec.it	#57     !lui	a5,0x44820
    2168:	4bc0                	c.lw	s0,20(a5)
    216a:	8e04                	exec.it	#78     !andi	t0,s0,66
    216c:	02028063          	beqz	t0,218c <UART0_InterruptHandler+0x28>
    2170:	8e00                	exec.it	#70     !lw	t1,-2036(gp) # 30000104 <boot_downloading>
    2172:	0613695b          	bnec	t1,1,21e4 <UART0_InterruptHandler+0x80>
    2176:	8e44                	exec.it	#110     !addi	s1,gp,-2040 # 30000100 <uart_dl_port>
    2178:	4098                	c.lw	a4,0(s1)
    217a:	06e36563          	bltu	t1,a4,21e4 <UART0_InterruptHandler+0x80>
    217e:	44820937          	lui	s2,0x44820
    2182:	4985                	c.li	s3,1
    2184:	00892783          	lw	a5,8(s2) # 44820008 <_stack+0x147c0008>
    2188:	4157f75b          	bbs	a5,21,2196 <UART0_InterruptHandler+0x32>
    218c:	448202b7          	lui	t0,0x44820
    2190:	0082aa23          	sw	s0,20(t0) # 44820014 <_stack+0x147c0014>
    2194:	8440                	exec.it	#34     !j	5d6e <__riscv_restore_4>
    2196:	8c44                	exec.it	#46     !lw	ra,0(s1)
    2198:	00009363          	bnez	ra,219e <UART0_InterruptHandler+0x3a>
    219c:	8a60                	exec.it	#116     !sw	s3,0(s1)
    219e:	00c92303          	lw	t1,12(s2)
    21a2:	3c83255b          	bfoz	a0,t1,15,8
    21a6:	26a5                	c.jal	250e <uart_download_rx>
    21a8:	bff1                	c.j	2184 <UART0_InterruptHandler+0x20>
    21aa:	00c62883          	lw	a7,12(a2)
    21ae:	0003ae03          	lw	t3,0(t2)
    21b2:	0088df13          	srli	t5,a7,0x8
    21b6:	001e0813          	addi	a6,t3,1
    21ba:	01c58eb3          	add	t4,a1,t3
    21be:	0103a023          	sw	a6,0(t2)
    21c2:	0003af83          	lw	t6,0(t2)
    21c6:	01ee8023          	sb	t5,0(t4)
    21ca:	00af9463          	bne	t6,a0,21d2 <UART0_InterruptHandler+0x6e>
    21ce:	8201a023          	sw	zero,-2016(gp) # 30000118 <uart0_rx_index>
    21d2:	4614                	c.lw	a3,8(a2)
    21d4:	fd56fb5b          	bbs	a3,21,21aa <UART0_InterruptHandler+0x46>
    21d8:	ba647a5b          	bbc	s0,6,218c <UART0_InterruptHandler+0x28>
    21dc:	4485                	c.li	s1,1
    21de:	8291a223          	sw	s1,-2012(gp) # 3000011c <uart0_rx_done>
    21e2:	b76d                	c.j	218c <UART0_InterruptHandler+0x28>
    21e4:	44820637          	lui	a2,0x44820
    21e8:	82018393          	addi	t2,gp,-2016 # 30000118 <uart0_rx_index>
    21ec:	8b018593          	addi	a1,gp,-1872 # 300001a8 <uart0_rx_buf>
    21f0:	08000513          	li	a0,128
    21f4:	bff9                	c.j	21d2 <UART0_InterruptHandler+0x6e>

000021f6 <uart1_send_byte>:
    21f6:	8050                	exec.it	#33     !lui	a5,0x45830
    21f8:	4798                	c.lw	a4,8(a5)
    21fa:	bf477f5b          	bbc	a4,20,21f8 <uart1_send_byte+0x2>
    21fe:	c7c8                	c.sw	a0,12(a5)
    2200:	8082                	c.jr	ra

00002202 <uart1_init>:
    2202:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    2204:	8050                	exec.it	#33     !lui	a5,0x45830
    2206:	8470                	exec.it	#51     !sw	zero,0(a5) # 45830000 <_stack+0x157d0000>
    2208:	8464                	exec.it	#58     !li	s0,225
    220a:	cd19                	c.beqz	a0,2228 <uart1_init+0x26>
    220c:	8624                	exec.it	#90     !lui	ra,0x18cc
    220e:	8c70                	exec.it	#55     !srli	s0,a0,0x1
    2210:	8220                	exec.it	#80     !addi	t0,ra,-1409 # 18cba7f <SRAM2_SIZE+0x18b1357>
    2212:	8a44                	exec.it	#108     !add	t1,s0,t0
    2214:	8a34                	exec.it	#93     !divu	t2,t1,a0
    2216:	4491                	c.li	s1,4
    2218:	8e20                	exec.it	#86     !addi	s0,t2,-1
    221a:	00947363          	bgeu	s0,s1,2220 <uart1_init+0x1e>
    221e:	4411                	c.li	s0,4
    2220:	6509                	c.lui	a0,0x2
    2222:	00a46363          	bltu	s0,a0,2228 <uart1_init+0x26>
    2226:	8a50                	exec.it	#101     !addi	s0,a0,-1 # 1fff <nmi_handler+0x9d>
    2228:	8620                	exec.it	#82     !lui	s1,0x44010
    222a:	588c                	c.lw	a1,48(s1)
    222c:	4505                	c.li	a0,1
    222e:	4005e613          	ori	a2,a1,1024
    2232:	d890                	c.sw	a2,48(s1)
    2234:	3bf5                	c.jal	2030 <GPIO_UART_function_enable>
    2236:	670d                	c.lui	a4,0x3
    2238:	458306b7          	lui	a3,0x45830
    223c:	04070813          	addi	a6,a4,64 # 3040 <bsdiff_flash_init+0xa>
    2240:	0106a223          	sw	a6,4(a3) # 45830004 <_stack+0x157d0004>
    2244:	04200893          	li	a7,66
    2248:	0116a823          	sw	a7,16(a3)
    224c:	0006ac23          	sw	zero,24(a3)
    2250:	00841e13          	slli	t3,s0,0x8
    2254:	0006ae23          	sw	zero,28(a3)
    2258:	01be6413          	ori	s0,t3,27
    225c:	c280                	c.sw	s0,0(a3)
    225e:	8201a623          	sw	zero,-2004(gp) # 30000124 <uart1_rx_done>
    2262:	8201a423          	sw	zero,-2008(gp) # 30000120 <uart1_rx_index>
    2266:	8874                	exec.it	#61     !addi	s1,s1,128 # 44010080 <_stack+0x13fb0080>
    2268:	8230                	exec.it	#81     !lw	t4,0(s1)
    226a:	6f21                	c.lui	t5,0x8
    226c:	01eeefb3          	or	t6,t4,t5
    2270:	01f4a023          	sw	t6,0(s1)
    2274:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>

00002276 <uart1_disable>:
    2276:	8050                	exec.it	#33     !lui	a5,0x45830
    2278:	4398                	c.lw	a4,0(a5)
    227a:	76f5                	c.lui	a3,0xffffd
    227c:	8a24                	exec.it	#92     !andi	t0,a4,-28
    227e:	8830                	exec.it	#21     !sw	t0,0(a5) # 45830000 <_stack+0x157d0000>
    2280:	8250                	exec.it	#97     !lw	t1,4(a5)
    2282:	8064                	exec.it	#56     !addi	t2,a3,-65 # ffffcfbf <_stack+0xcff9cfbf>
    2284:	8204                	exec.it	#72     !and	a0,t1,t2
    2286:	c3c8                	c.sw	a0,4(a5)
    2288:	4b8c                	c.lw	a1,16(a5)
    228a:	8c64                	exec.it	#62     !lui	a6,0x44010
    228c:	8270                	exec.it	#113     !addi	a7,a6,128 # 44010080 <_stack+0x13fb0080>
    228e:	8a10                	exec.it	#69     !andi	a2,a1,-67
    2290:	cb90                	c.sw	a2,16(a5)
    2292:	7ee1                	c.lui	t4,0xffff8
    2294:	8424                	exec.it	#26     !lw	t3,0(a7)
    2296:	fffe8f13          	addi	t5,t4,-1 # ffff7fff <_stack+0xcff97fff>
    229a:	01ee7fb3          	and	t6,t3,t5
    229e:	01f8a023          	sw	t6,0(a7)
    22a2:	03082783          	lw	a5,48(a6)
    22a6:	bff7f713          	andi	a4,a5,-1025
    22aa:	02e82823          	sw	a4,48(a6)
    22ae:	8082                	c.jr	ra

000022b0 <UART1_InterruptHandler>:
    22b0:	8060                	exec.it	#48     !jal	t0,5d3a <__riscv_save_4>
    22b2:	8050                	exec.it	#33     !lui	a5,0x45830
    22b4:	4bc0                	c.lw	s0,20(a5)
    22b6:	8e04                	exec.it	#78     !andi	t0,s0,66
    22b8:	02028163          	beqz	t0,22da <UART1_InterruptHandler+0x2a>
    22bc:	8e00                	exec.it	#70     !lw	t1,-2036(gp) # 30000104 <boot_downloading>
    22be:	0613685b          	bnec	t1,1,232e <UART1_InterruptHandler+0x7e>
    22c2:	8e44                	exec.it	#110     !addi	s1,gp,-2040 # 30000100 <uart_dl_port>
    22c4:	8604                	exec.it	#74     !lw	t2,0(s1)
    22c6:	ffd3f513          	andi	a0,t2,-3
    22ca:	e135                	c.bnez	a0,232e <UART1_InterruptHandler+0x7e>
    22cc:	45830937          	lui	s2,0x45830
    22d0:	4989                	c.li	s3,2
    22d2:	00892783          	lw	a5,8(s2) # 45830008 <_stack+0x157d0008>
    22d6:	4157f75b          	bbs	a5,21,22e4 <UART1_InterruptHandler+0x34>
    22da:	458302b7          	lui	t0,0x45830
    22de:	0082aa23          	sw	s0,20(t0) # 45830014 <_stack+0x157d0014>
    22e2:	8440                	exec.it	#34     !j	5d6e <__riscv_restore_4>
    22e4:	8c44                	exec.it	#46     !lw	ra,0(s1)
    22e6:	00009363          	bnez	ra,22ec <UART1_InterruptHandler+0x3c>
    22ea:	8a60                	exec.it	#116     !sw	s3,0(s1)
    22ec:	00c92303          	lw	t1,12(s2)
    22f0:	0ff37513          	andi	a0,t1,255
    22f4:	2c29                	c.jal	250e <uart_download_rx>
    22f6:	bff1                	c.j	22d2 <UART1_InterruptHandler+0x22>
    22f8:	00c6a883          	lw	a7,12(a3)
    22fc:	8a00                	exec.it	#68     !lw	t3,0(a4)
    22fe:	001e0e93          	addi	t4,t3,1
    2302:	01c60f33          	add	t5,a2,t3
    2306:	01d72023          	sw	t4,0(a4)
    230a:	00072f83          	lw	t6,0(a4)
    230e:	011f0023          	sb	a7,0(t5) # 8000 <irq_handler+0xfa4>
    2312:	00bf9463          	bne	t6,a1,231a <UART1_InterruptHandler+0x6a>
    2316:	8201a423          	sw	zero,-2008(gp) # 30000120 <uart1_rx_index>
    231a:	0086a803          	lw	a6,8(a3)
    231e:	fd587d5b          	bbs	a6,21,22f8 <UART1_InterruptHandler+0x48>
    2322:	ba647c5b          	bbc	s0,6,22da <UART1_InterruptHandler+0x2a>
    2326:	4485                	c.li	s1,1
    2328:	8291a623          	sw	s1,-2004(gp) # 30000124 <uart1_rx_done>
    232c:	b77d                	c.j	22da <UART1_InterruptHandler+0x2a>
    232e:	458306b7          	lui	a3,0x45830
    2332:	82818713          	addi	a4,gp,-2008 # 30000120 <uart1_rx_index>
    2336:	93018613          	addi	a2,gp,-1744 # 30000228 <uart1_rx_buf>
    233a:	08000593          	li	a1,128
    233e:	bff1                	c.j	231a <UART1_InterruptHandler+0x6a>

00002340 <uart1_send>:
    2340:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    2342:	842a                	c.mv	s0,a0
    2344:	00b504b3          	add	s1,a0,a1
    2348:	00941363          	bne	s0,s1,234e <uart1_send+0xe>
    234c:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>
    234e:	00044503          	lbu	a0,0(s0) # 18000 <_data_lmastart+0xe6ba>
    2352:	0405                	c.addi	s0,1
    2354:	354d                	c.jal	21f6 <uart1_send_byte>
    2356:	bfcd                	c.j	2348 <uart1_send+0x8>

00002358 <uart1_send_string>:
    2358:	1fd032ef          	jal	t0,5d54 <__riscv_save_0>
    235c:	8081a783          	lw	a5,-2040(gp) # 30000100 <uart_dl_port>
    2360:	842a                	c.mv	s0,a0
    2362:	0027ea5b          	bnec	a5,2,2376 <uart1_send_string+0x1e>
    2366:	80c1a283          	lw	t0,-2036(gp) # 30000104 <boot_downloading>
    236a:	0012e65b          	bnec	t0,1,2376 <uart1_send_string+0x1e>
    236e:	20b0306f          	j	5d78 <__riscv_restore_0>
    2372:	0405                	c.addi	s0,1
    2374:	3549                	c.jal	21f6 <uart1_send_byte>
    2376:	00044503          	lbu	a0,0(s0)
    237a:	fd65                	c.bnez	a0,2372 <uart1_send_string+0x1a>
    237c:	bfcd                	c.j	236e <uart1_send_string+0x16>

0000237e <uart1_wait_tx_finish>:
    237e:	45830737          	lui	a4,0x45830
    2382:	471c                	c.lw	a5,8(a4)
    2384:	bf17ff5b          	bbc	a5,17,2382 <uart1_wait_tx_finish+0x4>
    2388:	8082                	c.jr	ra

0000238a <wdt_ctrl>:
    238a:	1cb032ef          	jal	t0,5d54 <__riscv_save_0>
    238e:	0e3307b7          	lui	a5,0xe330
    2392:	00378713          	addi	a4,a5,3 # e330003 <SRAM2_SIZE+0xe3158db>
    2396:	04e50763          	beq	a0,a4,23e4 <wdt_ctrl+0x5a>
    239a:	00a76d63          	bltu	a4,a0,23b4 <wdt_ctrl+0x2a>
    239e:	00178f13          	addi	t5,a5,1
    23a2:	03e50a63          	beq	a0,t5,23d6 <wdt_ctrl+0x4c>
    23a6:	00278293          	addi	t0,a5,2
    23aa:	06550c63          	beq	a0,t0,2422 <wdt_ctrl+0x98>
    23ae:	4501                	c.li	a0,0
    23b0:	1c90306f          	j	5d78 <__riscv_restore_0>
    23b4:	00478893          	addi	a7,a5,4
    23b8:	ff151be3          	bne	a0,a7,23ae <wdt_ctrl+0x24>
    23bc:	8401a423          	sw	zero,-1976(gp) # 30000140 <g_wdt_period>
    23c0:	44010fb7          	lui	t6,0x44010
    23c4:	030fae03          	lw	t3,48(t6) # 44010030 <_stack+0x13fb0030>
    23c8:	80000eb7          	lui	t4,0x80000
    23cc:	01de6733          	or	a4,t3,t4
    23d0:	02efa823          	sw	a4,48(t6)
    23d4:	bfe9                	c.j	23ae <wdt_ctrl+0x24>
    23d6:	44010fb7          	lui	t6,0x44010
    23da:	030fa783          	lw	a5,48(t6) # 44010030 <_stack+0x13fb0030>
    23de:	7807a75b          	bfoz	a4,a5,30,0
    23e2:	b7fd                	c.j	23d0 <wdt_ctrl+0x46>
    23e4:	84818913          	addi	s2,gp,-1976 # 30000140 <g_wdt_period>
    23e8:	00092083          	lw	ra,0(s2)
    23ec:	62c1                	c.lui	t0,0x10
    23ee:	fff28413          	addi	s0,t0,-1 # ffff <_data_lmastart+0x66b9>
    23f2:	0080f333          	and	t1,ra,s0
    23f6:	005a03b7          	lui	t2,0x5a0
    23fa:	00736533          	or	a0,t1,t2
    23fe:	440004b7          	lui	s1,0x44000
    2402:	60a4a023          	sw	a0,1536(s1) # 44000600 <_stack+0x13fa0600>
    2406:	8a30                	exec.it	#85     !li	a0,300
    2408:	2871                	c.jal	24a4 <DelayUS>
    240a:	8054                	exec.it	#41     !lw	a1,0(s2)
    240c:	00a506b7          	lui	a3,0xa50
    2410:	0085f633          	and	a2,a1,s0
    2414:	00d66833          	or	a6,a2,a3
    2418:	6104a023          	sw	a6,1536(s1)
    241c:	8a30                	exec.it	#85     !li	a0,300
    241e:	2059                	c.jal	24a4 <DelayUS>
    2420:	b779                	c.j	23ae <wdt_ctrl+0x24>
    2422:	0005a903          	lw	s2,0(a1)
    2426:	6441                	c.lui	s0,0x10
    2428:	147d                	c.addi	s0,-1
    242a:	008970b3          	and	ra,s2,s0
    242e:	005a0337          	lui	t1,0x5a0
    2432:	8521a423          	sw	s2,-1976(gp) # 30000140 <g_wdt_period>
    2436:	0060e3b3          	or	t2,ra,t1
    243a:	44000937          	lui	s2,0x44000
    243e:	8a30                	exec.it	#85     !li	a0,300
    2440:	84ae                	c.mv	s1,a1
    2442:	60792023          	sw	t2,1536(s2) # 44000600 <_stack+0x13fa0600>
    2446:	28b9                	c.jal	24a4 <DelayUS>
    2448:	4088                	c.lw	a0,0(s1)
    244a:	00a50637          	lui	a2,0xa50
    244e:	008575b3          	and	a1,a0,s0
    2452:	00c5e6b3          	or	a3,a1,a2
    2456:	60d92023          	sw	a3,1536(s2)
    245a:	b7c9                	c.j	241c <wdt_ctrl+0x92>

0000245c <wdt_reboot>:
    245c:	0f9032ef          	jal	t0,5d54 <__riscv_save_0>
    2460:	1141                	c.addi	sp,-16
    2462:	0e330537          	lui	a0,0xe330
    2466:	4799                	c.li	a5,6
    2468:	006c                	c.addi4spn	a1,sp,12
    246a:	0509                	c.addi	a0,2
    246c:	c63e                	c.swsp	a5,12(sp)
    246e:	3f31                	c.jal	238a <wdt_ctrl>
    2470:	44010737          	lui	a4,0x44010
    2474:	03072283          	lw	t0,48(a4) # 44010030 <_stack+0x13fb0030>
    2478:	7802a35b          	bfoz	t1,t0,30,0
    247c:	02672823          	sw	t1,48(a4)
    2480:	a001                	c.j	2480 <wdt_reboot+0x24>

00002482 <wdt_close>:
    2482:	440007b7          	lui	a5,0x44000
    2486:	005a06b7          	lui	a3,0x5a0
    248a:	60d7a023          	sw	a3,1536(a5) # 44000600 <_stack+0x13fa0600>
    248e:	00a50737          	lui	a4,0xa50
    2492:	448002b7          	lui	t0,0x44800
    2496:	60e7a023          	sw	a4,1536(a5)
    249a:	00d2a023          	sw	a3,0(t0) # 44800000 <_stack+0x147a0000>
    249e:	00e2a023          	sw	a4,0(t0)
    24a2:	8082                	c.jr	ra

000024a4 <DelayUS>:
    24a4:	1101                	c.addi	sp,-32
    24a6:	46ed                	c.li	a3,27
    24a8:	c62a                	c.swsp	a0,12(sp)
    24aa:	47b2                	c.lwsp	a5,12(sp)
    24ac:	fff78713          	addi	a4,a5,-1
    24b0:	c63a                	c.swsp	a4,12(sp)
    24b2:	e399                	c.bnez	a5,24b8 <DelayUS+0x14>
    24b4:	6105                	c.addi16sp	sp,32
    24b6:	8082                	c.jr	ra
    24b8:	ce02                	c.swsp	zero,28(sp)
    24ba:	42f2                	c.lwsp	t0,28(sp)
    24bc:	fe56e7e3          	bltu	a3,t0,24aa <DelayUS+0x6>
    24c0:	4372                	c.lwsp	t1,28(sp)
    24c2:	8c50                	exec.it	#39     !addi	t2,t1,1 # 5a0001 <SRAM2_SIZE+0x5858d9>
    24c4:	ce1e                	c.swsp	t2,28(sp)
    24c6:	bfd5                	c.j	24ba <DelayUS+0x16>

000024c8 <jump_to_app>:
    24c8:	08d032ef          	jal	t0,5d54 <__riscv_save_0>
    24cc:	00005517          	auipc	a0,0x5
    24d0:	9c850513          	addi	a0,a0,-1592 # 6e94 <_ITB_BASE_+0x1d4>
    24d4:	0fffe097          	auipc	ra,0xfffe
    24d8:	0a4080e7          	jalr	164(ra) # 10000578 <bk_printf>
    24dc:	00090537          	lui	a0,0x90
    24e0:	0fffe097          	auipc	ra,0xfffe
    24e4:	10c080e7          	jalr	268(ra) # 100005ec <bk_print_hex>
    24e8:	00005517          	auipc	a0,0x5
    24ec:	9c050513          	addi	a0,a0,-1600 # 6ea8 <_ITB_BASE_+0x1e8>
    24f0:	0fffe097          	auipc	ra,0xfffe
    24f4:	088080e7          	jalr	136(ra) # 10000578 <bk_printf>
    24f8:	3559                	c.jal	237e <uart1_wait_tx_finish>
    24fa:	0ffff097          	auipc	ra,0xffff
    24fe:	b76080e7          	jalr	-1162(ra) # 10001070 <bl_hw_board_deinit>
    2502:	000907b7          	lui	a5,0x90
    2506:	9782                	c.jalr	a5
    2508:	4501                	c.li	a0,0
    250a:	06f0306f          	j	5d78 <__riscv_restore_0>

0000250e <uart_download_rx>:
    250e:	84c18793          	addi	a5,gp,-1972 # 30000144 <uart_buff_write>
    2512:	4394                	c.lw	a3,0(a5)
    2514:	2fffe297          	auipc	t0,0x2fffe
    2518:	e1428293          	addi	t0,t0,-492 # 30000328 <bim_uart_rx_buf>
    251c:	00168713          	addi	a4,a3,1 # 5a0001 <SRAM2_SIZE+0x5858d9>
    2520:	00d28333          	add	t1,t0,a3
    2524:	6385                	c.lui	t2,0x1
    2526:	c398                	c.sw	a4,0(a5)
    2528:	00a30023          	sb	a0,0(t1)
    252c:	00771463          	bne	a4,t2,2534 <uart_download_rx+0x26>
    2530:	8401a623          	sw	zero,-1972(gp) # 30000144 <uart_buff_write>
    2534:	8082                	c.jr	ra

00002536 <init>:
    2536:	4501                	c.li	a0,0
    2538:	8082                	c.jr	ra

0000253a <erase>:
    253a:	001032ef          	jal	t0,5d3a <__riscv_save_4>
    253e:	84aa                	c.mv	s1,a0
    2540:	842e                	c.mv	s0,a1
    2542:	0fffe097          	auipc	ra,0xfffe
    2546:	ac0080e7          	jalr	-1344(ra) # 10000002 <bk_flash_unlock>
    254a:	00fff537          	lui	a0,0xfff
    254e:	67c1                	c.lui	a5,0x10
    2550:	8ce9                	c.and	s1,a0
    2552:	fff78293          	addi	t0,a5,-1 # ffff <_data_lmastart+0x66b9>
    2556:	0054f733          	and	a4,s1,t0
    255a:	8926                	c.mv	s2,s1
    255c:	c705                	c.beqz	a4,2584 <erase+0x4a>
    255e:	005480b3          	add	ra,s1,t0
    2562:	00ff0337          	lui	t1,0xff0
    2566:	0060f3b3          	and	t2,ra,t1
    256a:	008489b3          	add	s3,s1,s0
    256e:	0133f363          	bgeu	t2,s3,2574 <erase+0x3a>
    2572:	899e                	c.mv	s3,t2
    2574:	8926                	c.mv	s2,s1
    2576:	00005a97          	auipc	s5,0x5
    257a:	a52a8a93          	addi	s5,s5,-1454 # 6fc8 <_ITB_BASE_+0x308>
    257e:	6a05                	c.lui	s4,0x1
    2580:	03396a63          	bltu	s2,s3,25b4 <erase+0x7a>
    2584:	9922                	c.add	s2,s0
    2586:	6a41                	c.lui	s4,0x10
    2588:	00005a97          	auipc	s5,0x5
    258c:	a44a8a93          	addi	s5,s5,-1468 # 6fcc <_ITB_BASE_+0x30c>
    2590:	408909b3          	sub	s3,s2,s0
    2594:	028a6e63          	bltu	s4,s0,25d0 <erase+0x96>
    2598:	00005a17          	auipc	s4,0x5
    259c:	a30a0a13          	addi	s4,s4,-1488 # 6fc8 <_ITB_BASE_+0x308>
    25a0:	6905                	c.lui	s2,0x1
    25a2:	e021                	c.bnez	s0,25e2 <erase+0xa8>
    25a4:	0fffe097          	auipc	ra,0xfffe
    25a8:	a5c080e7          	jalr	-1444(ra) # 10000000 <_itcm_ema_start>
    25ac:	40998533          	sub	a0,s3,s1
    25b0:	7be0306f          	j	5d6e <__riscv_restore_4>
    25b4:	854a                	c.mv	a0,s2
    25b6:	0fffe097          	auipc	ra,0xfffe
    25ba:	cfa080e7          	jalr	-774(ra) # 100002b0 <flash_erase_sector>
    25be:	8556                	c.mv	a0,s5
    25c0:	9952                	c.add	s2,s4
    25c2:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    25c4:	01446463          	bltu	s0,s4,25cc <erase+0x92>
    25c8:	8824                	exec.it	#28     !sub	s0,s0,s4
    25ca:	bf5d                	c.j	2580 <erase+0x46>
    25cc:	4401                	c.li	s0,0
    25ce:	bf4d                	c.j	2580 <erase+0x46>
    25d0:	854e                	c.mv	a0,s3
    25d2:	0fffe097          	auipc	ra,0xfffe
    25d6:	d14080e7          	jalr	-748(ra) # 100002e6 <flash_erase_block>
    25da:	8556                	c.mv	a0,s5
    25dc:	8824                	exec.it	#28     !sub	s0,s0,s4
    25de:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    25e0:	bf45                	c.j	2590 <erase+0x56>
    25e2:	854e                	c.mv	a0,s3
    25e4:	0fffe097          	auipc	ra,0xfffe
    25e8:	ccc080e7          	jalr	-820(ra) # 100002b0 <flash_erase_sector>
    25ec:	8552                	c.mv	a0,s4
    25ee:	99ca                	c.add	s3,s2
    25f0:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    25f2:	fb2469e3          	bltu	s0,s2,25a4 <erase+0x6a>
    25f6:	41240433          	sub	s0,s0,s2
    25fa:	b765                	c.j	25a2 <erase+0x68>

000025fc <write>:
    25fc:	758032ef          	jal	t0,5d54 <__riscv_save_0>
    2600:	8432                	c.mv	s0,a2
    2602:	892a                	c.mv	s2,a0
    2604:	84ae                	c.mv	s1,a1
    2606:	0fffe097          	auipc	ra,0xfffe
    260a:	9fc080e7          	jalr	-1540(ra) # 10000002 <bk_flash_unlock>
    260e:	8526                	c.mv	a0,s1
    2610:	8622                	c.mv	a2,s0
    2612:	85ca                	c.mv	a1,s2
    2614:	0fffe097          	auipc	ra,0xfffe
    2618:	e16080e7          	jalr	-490(ra) # 1000042a <flash_write_data>
    261c:	0fffe097          	auipc	ra,0xfffe
    2620:	9e4080e7          	jalr	-1564(ra) # 10000000 <_itcm_ema_start>
    2624:	8522                	c.mv	a0,s0
    2626:	7520306f          	j	5d78 <__riscv_restore_0>

0000262a <read>:
    262a:	72a032ef          	jal	t0,5d54 <__riscv_save_0>
    262e:	87ae                	c.mv	a5,a1
    2630:	85aa                	c.mv	a1,a0
    2632:	853e                	c.mv	a0,a5
    2634:	8432                	c.mv	s0,a2
    2636:	0fffe097          	auipc	ra,0xfffe
    263a:	d66080e7          	jalr	-666(ra) # 1000039c <flash_read_data>
    263e:	8522                	c.mv	a0,s0
    2640:	7380306f          	j	5d78 <__riscv_restore_0>

00002644 <init>:
    2644:	4501                	c.li	a0,0
    2646:	8082                	c.jr	ra

00002648 <erase>:
    2648:	6f2032ef          	jal	t0,5d3a <__riscv_save_4>
    264c:	02000493          	li	s1,32
    2650:	029542b3          	div	t0,a0,s1
    2654:	01f58413          	addi	s0,a1,31
    2658:	02200793          	li	a5,34
    265c:	00545093          	srli	ra,s0,0x5
    2660:	02f08433          	mul	s0,ra,a5
    2664:	02f28933          	mul	s2,t0,a5
    2668:	0fffe097          	auipc	ra,0xfffe
    266c:	99a080e7          	jalr	-1638(ra) # 10000002 <bk_flash_unlock>
    2670:	00fff537          	lui	a0,0xfff
    2674:	6341                	c.lui	t1,0x10
    2676:	00a974b3          	and	s1,s2,a0
    267a:	fff30393          	addi	t2,t1,-1 # ffff <_data_lmastart+0x66b9>
    267e:	0074f733          	and	a4,s1,t2
    2682:	8926                	c.mv	s2,s1
    2684:	c705                	c.beqz	a4,26ac <erase+0x64>
    2686:	007485b3          	add	a1,s1,t2
    268a:	00ff0637          	lui	a2,0xff0
    268e:	00c5f6b3          	and	a3,a1,a2
    2692:	008489b3          	add	s3,s1,s0
    2696:	0136f363          	bgeu	a3,s3,269c <erase+0x54>
    269a:	89b6                	c.mv	s3,a3
    269c:	8926                	c.mv	s2,s1
    269e:	00005a97          	auipc	s5,0x5
    26a2:	92aa8a93          	addi	s5,s5,-1750 # 6fc8 <_ITB_BASE_+0x308>
    26a6:	6a05                	c.lui	s4,0x1
    26a8:	03396a63          	bltu	s2,s3,26dc <erase+0x94>
    26ac:	9922                	c.add	s2,s0
    26ae:	6a41                	c.lui	s4,0x10
    26b0:	00005a97          	auipc	s5,0x5
    26b4:	91ca8a93          	addi	s5,s5,-1764 # 6fcc <_ITB_BASE_+0x30c>
    26b8:	408909b3          	sub	s3,s2,s0
    26bc:	028a6e63          	bltu	s4,s0,26f8 <erase+0xb0>
    26c0:	00005a17          	auipc	s4,0x5
    26c4:	908a0a13          	addi	s4,s4,-1784 # 6fc8 <_ITB_BASE_+0x308>
    26c8:	6905                	c.lui	s2,0x1
    26ca:	e021                	c.bnez	s0,270a <erase+0xc2>
    26cc:	0fffe097          	auipc	ra,0xfffe
    26d0:	934080e7          	jalr	-1740(ra) # 10000000 <_itcm_ema_start>
    26d4:	40998533          	sub	a0,s3,s1
    26d8:	6960306f          	j	5d6e <__riscv_restore_4>
    26dc:	854a                	c.mv	a0,s2
    26de:	0fffe097          	auipc	ra,0xfffe
    26e2:	bd2080e7          	jalr	-1070(ra) # 100002b0 <flash_erase_sector>
    26e6:	8556                	c.mv	a0,s5
    26e8:	9952                	c.add	s2,s4
    26ea:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    26ec:	01446463          	bltu	s0,s4,26f4 <erase+0xac>
    26f0:	8824                	exec.it	#28     !sub	s0,s0,s4
    26f2:	bf5d                	c.j	26a8 <erase+0x60>
    26f4:	4401                	c.li	s0,0
    26f6:	bf4d                	c.j	26a8 <erase+0x60>
    26f8:	854e                	c.mv	a0,s3
    26fa:	0fffe097          	auipc	ra,0xfffe
    26fe:	bec080e7          	jalr	-1044(ra) # 100002e6 <flash_erase_block>
    2702:	8556                	c.mv	a0,s5
    2704:	8824                	exec.it	#28     !sub	s0,s0,s4
    2706:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    2708:	bf45                	c.j	26b8 <erase+0x70>
    270a:	854e                	c.mv	a0,s3
    270c:	0fffe097          	auipc	ra,0xfffe
    2710:	ba4080e7          	jalr	-1116(ra) # 100002b0 <flash_erase_sector>
    2714:	8552                	c.mv	a0,s4
    2716:	99ca                	c.add	s3,s2
    2718:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    271a:	fb2469e3          	bltu	s0,s2,26cc <erase+0x84>
    271e:	41240433          	sub	s0,s0,s2
    2722:	b765                	c.j	26ca <erase+0x82>

00002724 <read>:
    2724:	616032ef          	jal	t0,5d3a <__riscv_save_4>
    2728:	02000793          	li	a5,32
    272c:	02f54433          	div	s0,a0,a5
    2730:	8aae                	c.mv	s5,a1
    2732:	8932                	c.mv	s2,a2
    2734:	89b2                	c.mv	s3,a2
    2736:	02f564b3          	rem	s1,a0,a5
    273a:	02200513          	li	a0,34
    273e:	02a40433          	mul	s0,s0,a0
    2742:	c48d                	c.beqz	s1,276c <read+0x48>
    2744:	008485b3          	add	a1,s1,s0
    2748:	409784b3          	sub	s1,a5,s1
    274c:	00c7e363          	bltu	a5,a2,2752 <read+0x2e>
    2750:	84b2                	c.mv	s1,a2
    2752:	00258293          	addi	t0,a1,2
    2756:	00928433          	add	s0,t0,s1
    275a:	409909b3          	sub	s3,s2,s1
    275e:	c499                	c.beqz	s1,276c <read+0x48>
    2760:	8626                	c.mv	a2,s1
    2762:	8556                	c.mv	a0,s5
    2764:	0fffe097          	auipc	ra,0xfffe
    2768:	c38080e7          	jalr	-968(ra) # 1000039c <flash_read_data>
    276c:	02098863          	beqz	s3,279c <read+0x78>
    2770:	02000b13          	li	s6,32
    2774:	8a4e                	c.mv	s4,s3
    2776:	013b7463          	bgeu	s6,s3,277e <read+0x5a>
    277a:	02000a13          	li	s4,32
    277e:	85a2                	c.mv	a1,s0
    2780:	009a8533          	add	a0,s5,s1
    2784:	8652                	c.mv	a2,s4
    2786:	414989b3          	sub	s3,s3,s4
    278a:	02240413          	addi	s0,s0,34 # 10022 <_data_lmastart+0x66dc>
    278e:	94d2                	c.add	s1,s4
    2790:	0fffe097          	auipc	ra,0xfffe
    2794:	c0c080e7          	jalr	-1012(ra) # 1000039c <flash_read_data>
    2798:	fc099ee3          	bnez	s3,2774 <read+0x50>
    279c:	854a                	c.mv	a0,s2
    279e:	5d00306f          	j	5d6e <__riscv_restore_4>

000027a2 <write>:
    27a2:	58a032ef          	jal	t0,5d2c <__riscv_save_10>
    27a6:	7139                	c.addi16sp	sp,-64
    27a8:	0ff00793          	li	a5,255
    27ac:	c62a                	c.swsp	a0,12(sp)
    27ae:	8aae                	c.mv	s5,a1
    27b0:	8cb2                	c.mv	s9,a2
    27b2:	4581                	c.li	a1,0
    27b4:	4679                	c.li	a2,30
    27b6:	1008                	c.addi4spn	a0,sp,32
    27b8:	ce3e                	c.swsp	a5,28(sp)
    27ba:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    27bc:	46b2                	c.lwsp	a3,12(sp)
    27be:	01f6f493          	andi	s1,a3,31
    27c2:	c48d                	c.beqz	s1,27ec <write+0x4a>
    27c4:	8766                	c.mv	a4,s9
    27c6:	07500613          	li	a2,117
    27ca:	2fffe597          	auipc	a1,0x2fffe
    27ce:	92e58593          	addi	a1,a1,-1746 # 300000f8 <__fini_array_end>
    27d2:	00005517          	auipc	a0,0x5
    27d6:	83250513          	addi	a0,a0,-1998 # 7004 <beken_onchip_flash+0x34>
    27da:	4481                	c.li	s1,0
    27dc:	0fffe097          	auipc	ra,0xfffe
    27e0:	d9c080e7          	jalr	-612(ra) # 10000578 <bk_printf>
    27e4:	8526                	c.mv	a0,s1
    27e6:	6121                	c.addi16sp	sp,64
    27e8:	57c0306f          	j	5d64 <__riscv_restore_10>
    27ec:	02000413          	li	s0,32
    27f0:	0286c6b3          	div	a3,a3,s0
    27f4:	6985                	c.lui	s3,0x1
    27f6:	02200093          	li	ra,34
    27fa:	10098513          	addi	a0,s3,256 # 1100 <__rtos_signature_freertos_v10_3+0x1100>
    27fe:	02168433          	mul	s0,a3,ra
    2802:	8c54                	exec.it	#47     !jal	ra,5d84 <malloc>
    2804:	892a                	c.mv	s2,a0
    2806:	e519                	c.bnez	a0,2814 <write+0x72>
    2808:	00005517          	auipc	a0,0x5
    280c:	81850513          	addi	a0,a0,-2024 # 7020 <beken_onchip_flash+0x50>
    2810:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    2812:	bfc9                	c.j	27e4 <write+0x42>
    2814:	10098613          	addi	a2,s3,256
    2818:	8040                	exec.it	#32     !li	a1,255
    281a:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    281c:	02200613          	li	a2,34
    2820:	8040                	exec.it	#32     !li	a1,255
    2822:	0868                	c.addi4spn	a0,sp,28
    2824:	6b41                	c.lui	s6,0x10
    2826:	7be1                	c.lui	s7,0xffff8
    2828:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    282a:	4a01                	c.li	s4,0
    282c:	4d01                	c.li	s10,0
    282e:	4481                	c.li	s1,0
    2830:	1b7d                	c.addi	s6,-1
    2832:	0b95                	c.addi	s7,5
    2834:	10098993          	addi	s3,s3,256
    2838:	0fffd097          	auipc	ra,0xfffd
    283c:	7ca080e7          	jalr	1994(ra) # 10000002 <bk_flash_unlock>
    2840:	000c9963          	bnez	s9,2852 <write+0xb0>
    2844:	0fffd097          	auipc	ra,0xfffd
    2848:	7bc080e7          	jalr	1980(ra) # 10000000 <_itcm_ema_start>
    284c:	854a                	c.mv	a0,s2
    284e:	8834                	exec.it	#29     !jal	ra,5d8a <free>
    2850:	bf51                	c.j	27e4 <write+0x42>
    2852:	42fd                	c.li	t0,31
    2854:	4c01                	c.li	s8,0
    2856:	0192f663          	bgeu	t0,s9,2862 <write+0xc0>
    285a:	fe0c8c13          	addi	s8,s9,-32
    285e:	02000c93          	li	s9,32
    2862:	009a85b3          	add	a1,s5,s1
    2866:	8666                	c.mv	a2,s9
    2868:	0868                	c.addi4spn	a0,sp,28
    286a:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    286c:	94e6                	c.add	s1,s9
    286e:	4701                	c.li	a4,0
    2870:	8e5a                	c.mv	t3,s6
    2872:	0870                	c.addi4spn	a2,sp,28
    2874:	00e60333          	add	t1,a2,a4
    2878:	00034383          	lbu	t2,0(t1)
    287c:	4ea1                	c.li	t4,8
    287e:	00839513          	slli	a0,t2,0x8
    2882:	01c545b3          	xor	a1,a0,t3
    2886:	3c05ae5b          	bfoz	t3,a1,15,0
    288a:	3c0e385b          	bfos	a6,t3,15,0
    288e:	001e1893          	slli	a7,t3,0x1
    2892:	3c08ae5b          	bfoz	t3,a7,15,0
    2896:	00085663          	bgez	a6,28a2 <write+0x100>
    289a:	0178ccb3          	xor	s9,a7,s7
    289e:	3c0cae5b          	bfoz	t3,s9,15,0
    28a2:	1efd                	c.addi	t4,-1
    28a4:	fe0e93e3          	bnez	t4,288a <write+0xe8>
    28a8:	0705                	c.addi	a4,1
    28aa:	bc0764db          	bnec	a4,32,2872 <write+0xd0>
    28ae:	008e1f13          	slli	t5,t3,0x8
    28b2:	008e5f93          	srli	t6,t3,0x8
    28b6:	01ff67b3          	or	a5,t5,t6
    28ba:	01a90533          	add	a0,s2,s10
    28be:	02200613          	li	a2,34
    28c2:	086c                	c.addi4spn	a1,sp,28
    28c4:	02f11e23          	sh	a5,60(sp)
    28c8:	022d0d13          	addi	s10,s10,34
    28cc:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    28ce:	000c1e63          	bnez	s8,28ea <write+0x148>
    28d2:	033a00b3          	mul	ra,s4,s3
    28d6:	866a                	c.mv	a2,s10
    28d8:	854a                	c.mv	a0,s2
    28da:	008085b3          	add	a1,ra,s0
    28de:	0fffe097          	auipc	ra,0xfffe
    28e2:	b4c080e7          	jalr	-1204(ra) # 1000042a <flash_write_data>
    28e6:	8ce2                	c.mv	s9,s8
    28e8:	bfa1                	c.j	2840 <write+0x9e>
    28ea:	ff3d1ee3          	bne	s10,s3,28e6 <write+0x144>
    28ee:	03aa06b3          	mul	a3,s4,s10
    28f2:	866a                	c.mv	a2,s10
    28f4:	854a                	c.mv	a0,s2
    28f6:	008685b3          	add	a1,a3,s0
    28fa:	0fffe097          	auipc	ra,0xfffe
    28fe:	b30080e7          	jalr	-1232(ra) # 1000042a <flash_write_data>
    2902:	866a                	c.mv	a2,s10
    2904:	8040                	exec.it	#32     !li	a1,255
    2906:	854a                	c.mv	a0,s2
    2908:	0a05                	c.addi	s4,1
    290a:	4d01                	c.li	s10,0
    290c:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    290e:	bfe1                	c.j	28e6 <write+0x144>

00002910 <reset_handler>:
    2910:	444032ef          	jal	t0,5d54 <__riscv_save_0>
    2914:	36bd                	c.jal	2482 <wdt_close>
    2916:	2811                	c.jal	292a <c_startup>
    2918:	20ad                	c.jal	2982 <system_init>
    291a:	2869                	c.jal	29b4 <__libc_init_array>
    291c:	0fffe097          	auipc	ra,0xfffe
    2920:	7bc080e7          	jalr	1980(ra) # 100010d8 <boot_main>
    2924:	4540306f          	j	5d78 <__riscv_restore_0>

00002928 <__platform_init>:
    2928:	8082                	c.jr	ra

0000292a <c_startup>:
    292a:	42a032ef          	jal	t0,5d54 <__riscv_save_0>
    292e:	00007797          	auipc	a5,0x7
    2932:	01878793          	addi	a5,a5,24 # 9946 <_data_lmastart>
    2936:	00006597          	auipc	a1,0x6
    293a:	d7a58593          	addi	a1,a1,-646 # 86b0 <_itcm_lma_start>
    293e:	40b78633          	sub	a2,a5,a1
    2942:	00b78763          	beq	a5,a1,2950 <c_startup+0x26>
    2946:	0fffd517          	auipc	a0,0xfffd
    294a:	6ba50513          	addi	a0,a0,1722 # 10000000 <_itcm_ema_start>
    294e:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    2950:	2fffd417          	auipc	s0,0x2fffd
    2954:	7c840413          	addi	s0,s0,1992 # 30000118 <uart0_rx_index>
    2958:	2fffd517          	auipc	a0,0x2fffd
    295c:	6a850513          	addi	a0,a0,1704 # 30000000 <SRAM2_BEGIN>
    2960:	40a40633          	sub	a2,s0,a0
    2964:	00007597          	auipc	a1,0x7
    2968:	fe258593          	addi	a1,a1,-30 # 9946 <_data_lmastart>
    296c:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    296e:	30018617          	auipc	a2,0x30018
    2972:	dba60613          	addi	a2,a2,-582 # 3001a728 <_end>
    2976:	8e01                	c.sub	a2,s0
    2978:	4581                	c.li	a1,0
    297a:	8522                	c.mv	a0,s0
    297c:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    297e:	3fa0306f          	j	5d78 <__riscv_restore_0>

00002982 <system_init>:
    2982:	fffff717          	auipc	a4,0xfffff
    2986:	57e70713          	addi	a4,a4,1406 # 1f00 <NDS_SAG_LMA_bootloader>
    298a:	f01007b7          	lui	a5,0xf0100
    298e:	cbb8                	c.sw	a4,80(a5)
    2990:	7d0022f3          	csrr	t0,mmisc_ctl
    2994:	0012fc5b          	bbc	t0,1,29ac <system_init+0x2a>
    2998:	e4000337          	lui	t1,0xe4000
    299c:	438d                	c.li	t2,3
    299e:	14000513          	li	a0,320
    29a2:	00732023          	sw	t2,0(t1) # e4000000 <_stack+0xb3fa0000>
    29a6:	7d0527f3          	csrrs	a5,mmisc_ctl,a0
    29aa:	8082                	c.jr	ra
    29ac:	e4000337          	lui	t1,0xe4000
    29b0:	4385                	c.li	t2,1
    29b2:	b7f5                	c.j	299e <system_init+0x1c>

000029b4 <__libc_init_array>:
    29b4:	3a0032ef          	jal	t0,5d54 <__riscv_save_0>
    29b8:	2fffd497          	auipc	s1,0x2fffd
    29bc:	74048493          	addi	s1,s1,1856 # 300000f8 <__fini_array_end>
    29c0:	2fffd417          	auipc	s0,0x2fffd
    29c4:	73840413          	addi	s0,s0,1848 # 300000f8 <__fini_array_end>
    29c8:	408482b3          	sub	t0,s1,s0
    29cc:	4022d493          	srai	s1,t0,0x2
    29d0:	4901                	c.li	s2,0
    29d2:	02991363          	bne	s2,s1,29f8 <__libc_init_array+0x44>
    29d6:	2fffd417          	auipc	s0,0x2fffd
    29da:	72240413          	addi	s0,s0,1826 # 300000f8 <__fini_array_end>
    29de:	2fffd317          	auipc	t1,0x2fffd
    29e2:	71a30313          	addi	t1,t1,1818 # 300000f8 <__fini_array_end>
    29e6:	408303b3          	sub	t2,t1,s0
    29ea:	4023d493          	srai	s1,t2,0x2
    29ee:	4901                	c.li	s2,0
    29f0:	00991963          	bne	s2,s1,2a02 <__libc_init_array+0x4e>
    29f4:	3840306f          	j	5d78 <__riscv_restore_0>
    29f8:	4008                	c.lw	a0,0(s0)
    29fa:	0905                	c.addi	s2,1
    29fc:	0411                	c.addi	s0,4
    29fe:	9502                	c.jalr	a0
    2a00:	bfc9                	c.j	29d2 <__libc_init_array+0x1e>
    2a02:	401c                	c.lw	a5,0(s0)
    2a04:	0905                	c.addi	s2,1
    2a06:	0411                	c.addi	s0,4
    2a08:	9782                	c.jalr	a5
    2a0a:	b7dd                	c.j	29f0 <__libc_init_array+0x3c>

00002a0c <mtime_handler>:
    2a0c:	08000793          	li	a5,128
    2a10:	3047b7f3          	csrrc	a5,mie,a5
    2a14:	8082                	c.jr	ra

00002a16 <mswi_handler>:
    2a16:	304477f3          	csrrci	a5,mie,8
    2a1a:	8082                	c.jr	ra

00002a1c <syscall_handler>:
    2a1c:	8082                	c.jr	ra

00002a1e <except_handler>:
    2a1e:	852e                	c.mv	a0,a1
    2a20:	8082                	c.jr	ra
    2a22:	0001                	c.nop

00002a24 <trap_entry>:
    2a24:	715d                	c.addi16sp	sp,-80
    2a26:	c686                	c.swsp	ra,76(sp)
    2a28:	c496                	c.swsp	t0,72(sp)
    2a2a:	c29a                	c.swsp	t1,68(sp)
    2a2c:	c09e                	c.swsp	t2,64(sp)
    2a2e:	de22                	c.swsp	s0,60(sp)
    2a30:	dc26                	c.swsp	s1,56(sp)
    2a32:	da2a                	c.swsp	a0,52(sp)
    2a34:	d82e                	c.swsp	a1,48(sp)
    2a36:	d632                	c.swsp	a2,44(sp)
    2a38:	d436                	c.swsp	a3,40(sp)
    2a3a:	d23a                	c.swsp	a4,36(sp)
    2a3c:	d03e                	c.swsp	a5,32(sp)
    2a3e:	ce42                	c.swsp	a6,28(sp)
    2a40:	cc46                	c.swsp	a7,24(sp)
    2a42:	ca72                	c.swsp	t3,20(sp)
    2a44:	c876                	c.swsp	t4,16(sp)
    2a46:	c67a                	c.swsp	t5,12(sp)
    2a48:	c47e                	c.swsp	t6,8(sp)
    2a4a:	342027f3          	csrr	a5,mcause
    2a4e:	34102473          	csrr	s0,mepc
    2a52:	300024f3          	csrr	s1,mstatus
    2a56:	0807d363          	bgez	a5,2adc <trap_entry+0xb8>
    2a5a:	7807a75b          	bfoz	a4,a5,30,0
    2a5e:	04b7685b          	bnec	a4,11,2aae <trap_entry+0x8a>
    2a62:	f14020f3          	csrr	ra,mhartid
    2a66:	e42008b7          	lui	a7,0xe4200
    2a6a:	00c09813          	slli	a6,ra,0xc
    2a6e:	00488e13          	addi	t3,a7,4 # e4200004 <_stack+0xb41a0004>
    2a72:	01c80eb3          	add	t4,a6,t3
    2a76:	000ea503          	lw	a0,0(t4) # 80000000 <_stack+0x4ffa0000>
    2a7a:	2a91                	c.jal	2bce <mext_interrupt>
    2a7c:	30049073          	csrw	mstatus,s1
    2a80:	34141073          	csrw	mepc,s0
    2a84:	5472                	c.lwsp	s0,60(sp)
    2a86:	40b6                	c.lwsp	ra,76(sp)
    2a88:	42a6                	c.lwsp	t0,72(sp)
    2a8a:	4316                	c.lwsp	t1,68(sp)
    2a8c:	4386                	c.lwsp	t2,64(sp)
    2a8e:	54e2                	c.lwsp	s1,56(sp)
    2a90:	5552                	c.lwsp	a0,52(sp)
    2a92:	55c2                	c.lwsp	a1,48(sp)
    2a94:	5632                	c.lwsp	a2,44(sp)
    2a96:	56a2                	c.lwsp	a3,40(sp)
    2a98:	5712                	c.lwsp	a4,36(sp)
    2a9a:	5782                	c.lwsp	a5,32(sp)
    2a9c:	4872                	c.lwsp	a6,28(sp)
    2a9e:	48e2                	c.lwsp	a7,24(sp)
    2aa0:	4e52                	c.lwsp	t3,20(sp)
    2aa2:	4ec2                	c.lwsp	t4,16(sp)
    2aa4:	4f32                	c.lwsp	t5,12(sp)
    2aa6:	4fa2                	c.lwsp	t6,8(sp)
    2aa8:	6161                	c.addi16sp	sp,80
    2aaa:	30200073          	mret
    2aae:	0077645b          	bnec	a4,7,2ab6 <trap_entry+0x92>
    2ab2:	3fa9                	c.jal	2a0c <mtime_handler>
    2ab4:	b7e1                	c.j	2a7c <trap_entry+0x58>
    2ab6:	02376d5b          	bnec	a4,3,2af0 <trap_entry+0xcc>
    2aba:	3fb1                	c.jal	2a16 <mswi_handler>
    2abc:	f14022f3          	csrr	t0,mhartid
    2ac0:	8254                	exec.it	#105     !addi	t1,t0,1
    2ac2:	f14023f3          	csrr	t2,mhartid
    2ac6:	e66006b7          	lui	a3,0xe6600
    2aca:	00c39513          	slli	a0,t2,0xc
    2ace:	00468593          	addi	a1,a3,4 # e6600004 <_stack+0xb65a0004>
    2ad2:	00b50633          	add	a2,a0,a1
    2ad6:	00662023          	sw	t1,0(a2)
    2ada:	b74d                	c.j	2a7c <trap_entry+0x58>
    2adc:	00b7ea5b          	bnec	a5,11,2af0 <trap_entry+0xcc>
    2ae0:	8736                	c.mv	a4,a3
    2ae2:	86b2                	c.mv	a3,a2
    2ae4:	862e                	c.mv	a2,a1
    2ae6:	85aa                	c.mv	a1,a0
    2ae8:	8546                	c.mv	a0,a7
    2aea:	3f0d                	c.jal	2a1c <syscall_handler>
    2aec:	0411                	c.addi	s0,4
    2aee:	b779                	c.j	2a7c <trap_entry+0x58>
    2af0:	85a2                	c.mv	a1,s0
    2af2:	853e                	c.mv	a0,a5
    2af4:	372d                	c.jal	2a1e <except_handler>
    2af6:	842a                	c.mv	s0,a0
    2af8:	b751                	c.j	2a7c <trap_entry+0x58>

00002afa <default_irq_handler>:
    2afa:	8082                	c.jr	ra

00002afc <uart_irq_handler>:
    2afc:	e68ff06f          	j	2164 <UART0_InterruptHandler>

00002b00 <uart1_irq_handler>:
    2b00:	fb0ff06f          	j	22b0 <UART1_InterruptHandler>

00002b04 <uart2_irq_handler>:
    2b04:	0fffe317          	auipc	t1,0xfffe
    2b08:	bf630067          	jr	-1034(t1) # 100006fa <UART2_InterruptHandler>

00002b0c <arch_interrupt_ctrl>:
    2b0c:	1141                	c.addi	sp,-16
    2b0e:	c602                	c.swsp	zero,12(sp)
    2b10:	0615625b          	bnec	a0,1,2b74 <arch_interrupt_ctrl+0x68>
    2b14:	03f00e13          	li	t3,63
    2b18:	e40008b7          	lui	a7,0xe4000
    2b1c:	4605                	c.li	a2,1
    2b1e:	e4002837          	lui	a6,0xe4002
    2b22:	c62a                	c.swsp	a0,12(sp)
    2b24:	46b2                	c.lwsp	a3,12(sp)
    2b26:	00de7b63          	bgeu	t3,a3,2b3c <arch_interrupt_ctrl+0x30>
    2b2a:	6505                	c.lui	a0,0x1
    2b2c:	80050593          	addi	a1,a0,-2048 # 800 <__rtos_signature_freertos_v10_3+0x800>
    2b30:	3045a7f3          	csrrs	a5,mie,a1
    2b34:	300467f3          	csrrsi	a5,mstatus,8
    2b38:	0141                	c.addi	sp,16
    2b3a:	8082                	c.jr	ra
    2b3c:	4732                	c.lwsp	a4,12(sp)
    2b3e:	0ce88edb          	lea.w	t4,a7,a4
    2b42:	00cea023          	sw	a2,0(t4)
    2b46:	4f32                	c.lwsp	t5,12(sp)
    2b48:	f1402ff3          	csrr	t6,mhartid
    2b4c:	005f5793          	srli	a5,t5,0x5
    2b50:	0cf802db          	lea.w	t0,a6,a5
    2b54:	007f9313          	slli	t1,t6,0x7
    2b58:	006283b3          	add	t2,t0,t1
    2b5c:	8654                	exec.it	#107     !lw	a3,0(t2) # 1000 <__rtos_signature_freertos_v10_3+0x1000>
    2b5e:	01e61533          	sll	a0,a2,t5
    2b62:	00d565b3          	or	a1,a0,a3
    2b66:	00b3a023          	sw	a1,0(t2)
    2b6a:	4732                	c.lwsp	a4,12(sp)
    2b6c:	00170e93          	addi	t4,a4,1
    2b70:	c676                	c.swsp	t4,12(sp)
    2b72:	bf4d                	c.j	2b24 <arch_interrupt_ctrl+0x18>
    2b74:	6785                	c.lui	a5,0x1
    2b76:	88878293          	addi	t0,a5,-1912 # 888 <__rtos_signature_freertos_v10_3+0x888>
    2b7a:	3042b7f3          	csrrc	a5,mie,t0
    2b7e:	300477f3          	csrrci	a5,mstatus,8
    2b82:	4305                	c.li	t1,1
    2b84:	03f00613          	li	a2,63
    2b88:	e40025b7          	lui	a1,0xe4002
    2b8c:	4505                	c.li	a0,1
    2b8e:	c61a                	c.swsp	t1,12(sp)
    2b90:	43b2                	c.lwsp	t2,12(sp)
    2b92:	00767563          	bgeu	a2,t2,2b9c <arch_interrupt_ctrl+0x90>
    2b96:	34205073          	csrwi	mcause,0
    2b9a:	bf79                	c.j	2b38 <arch_interrupt_ctrl+0x2c>
    2b9c:	4832                	c.lwsp	a6,12(sp)
    2b9e:	f14026f3          	csrr	a3,mhartid
    2ba2:	00585713          	srli	a4,a6,0x5
    2ba6:	0ce588db          	lea.w	a7,a1,a4
    2baa:	00769e13          	slli	t3,a3,0x7
    2bae:	01c88eb3          	add	t4,a7,t3
    2bb2:	000eaf03          	lw	t5,0(t4)
    2bb6:	01051fb3          	sll	t6,a0,a6
    2bba:	ffffc793          	not	a5,t6
    2bbe:	01e7f2b3          	and	t0,a5,t5
    2bc2:	005ea023          	sw	t0,0(t4)
    2bc6:	4332                	c.lwsp	t1,12(sp)
    2bc8:	8c50                	exec.it	#39     !addi	t2,t1,1
    2bca:	c61e                	c.swsp	t2,12(sp)
    2bcc:	b7d1                	c.j	2b90 <arch_interrupt_ctrl+0x84>

00002bce <mext_interrupt>:
    2bce:	186032ef          	jal	t0,5d54 <__riscv_save_0>
    2bd2:	00004797          	auipc	a5,0x4
    2bd6:	48a78793          	addi	a5,a5,1162 # 705c <irq_handler>
    2bda:	0ca780db          	lea.w	ra,a5,a0
    2bde:	0000a303          	lw	t1,0(ra)
    2be2:	842a                	c.mv	s0,a0
    2be4:	84a1aa23          	sw	a0,-1964(gp) # 3000014c <g_irq_source>
    2be8:	9302                	c.jalr	t1
    2bea:	f14022f3          	csrr	t0,mhartid
    2bee:	e4200737          	lui	a4,0xe4200
    2bf2:	00c29393          	slli	t2,t0,0xc
    2bf6:	00470513          	addi	a0,a4,4 # e4200004 <_stack+0xb41a0004>
    2bfa:	00a385b3          	add	a1,t2,a0
    2bfe:	c180                	c.sw	s0,0(a1)
    2c00:	1780306f          	j	5d78 <__riscv_restore_0>

00002c04 <__read_manage_block.constprop.0>:
    2c04:	136032ef          	jal	t0,5d3a <__riscv_save_4>
    2c08:	54f9                	c.li	s1,-2
    2c0a:	c12d                	c.beqz	a0,2c6c <__read_manage_block.constprop.0+0x68>
    2c0c:	8432                	c.mv	s0,a2
    2c0e:	54f9                	c.li	s1,-2
    2c10:	ce31                	c.beqz	a2,2c6c <__read_manage_block.constprop.0+0x68>
    2c12:	8a36                	c.mv	s4,a3
    2c14:	4114                	c.lw	a3,0(a0)
    2c16:	03f00793          	li	a5,63
    2c1a:	892a                	c.mv	s2,a0
    2c1c:	04d7f863          	bgeu	a5,a3,2c6c <__read_manage_block.constprop.0+0x68>
    2c20:	862e                	c.mv	a2,a1
    2c22:	89ae                	c.mv	s3,a1
    2c24:	494c                	c.lw	a1,20(a0)
    2c26:	00004517          	auipc	a0,0x4
    2c2a:	55a50513          	addi	a0,a0,1370 # 7180 <irq_handler+0x124>
    2c2e:	0fffe097          	auipc	ra,0xfffe
    2c32:	94a080e7          	jalr	-1718(ra) # 10000578 <bk_printf>
    2c36:	00092283          	lw	t0,0(s2) # 1000 <__rtos_signature_freertos_v10_3+0x1000>
    2c3a:	01492903          	lw	s2,20(s2)
    2c3e:	02598333          	mul	t1,s3,t0
    2c42:	86d2                	c.mv	a3,s4
    2c44:	8410                	exec.it	#3     !li	a2,64
    2c46:	991a                	c.add	s2,t1
    2c48:	85a2                	c.mv	a1,s0
    2c4a:	854a                	c.mv	a0,s2
    2c4c:	8c14                	exec.it	#15     !jal	ra,41d4 <ty_adapt_flash_read>
    2c4e:	84aa                	c.mv	s1,a0
    2c50:	c10d                	c.beqz	a0,2c72 <__read_manage_block.constprop.0+0x6e>
    2c52:	85aa                	c.mv	a1,a0
    2c54:	8722                	c.mv	a4,s0
    2c56:	04000693          	li	a3,64
    2c5a:	864a                	c.mv	a2,s2
    2c5c:	00004517          	auipc	a0,0x4
    2c60:	55050513          	addi	a0,a0,1360 # 71ac <irq_handler+0x150>
    2c64:	0fffe097          	auipc	ra,0xfffe
    2c68:	914080e7          	jalr	-1772(ra) # 10000578 <bk_printf>
    2c6c:	8526                	c.mv	a0,s1
    2c6e:	1000306f          	j	5d6e <__riscv_restore_4>
    2c72:	abcde0b7          	lui	ra,0xabcde
    2c76:	4018                	c.lw	a4,0(s0)
    2c78:	cba08393          	addi	t2,ra,-838 # abcddcba <_stack+0x7bc7dcba>
    2c7c:	06771763          	bne	a4,t2,2cea <__read_manage_block.constprop.0+0xe6>
    2c80:	00840593          	addi	a1,s0,8
    2c84:	04040513          	addi	a0,s0,64
    2c88:	4901                	c.li	s2,0
    2c8a:	0005c683          	lbu	a3,0(a1) # e4002000 <_stack+0xb3fa2000>
    2c8e:	0585                	c.addi	a1,1
    2c90:	9936                	c.add	s2,a3
    2c92:	feb51ce3          	bne	a0,a1,2c8a <__read_manage_block.constprop.0+0x86>
    2c96:	00442883          	lw	a7,4(s0)
    2c9a:	01844603          	lbu	a2,24(s0)
    2c9e:	01944803          	lbu	a6,25(s0)
    2ca2:	01289b63          	bne	a7,s2,2cb8 <__read_manage_block.constprop.0+0xb4>
    2ca6:	00c99963          	bne	s3,a2,2cb8 <__read_manage_block.constprop.0+0xb4>
    2caa:	fff80e13          	addi	t3,a6,-1 # e4001fff <_stack+0xb3fa1fff>
    2cae:	0ffe7e93          	andi	t4,t3,255
    2cb2:	4f09                	c.li	t5,2
    2cb4:	fbdf7ce3          	bgeu	t5,t4,2c6c <__read_manage_block.constprop.0+0x68>
    2cb8:	01944683          	lbu	a3,25(s0)
    2cbc:	8614                	exec.it	#75     !lbu	a1,24(s0)
    2cbe:	864e                	c.mv	a2,s3
    2cc0:	00004517          	auipc	a0,0x4
    2cc4:	52850513          	addi	a0,a0,1320 # 71e8 <irq_handler+0x18c>
    2cc8:	0fffe097          	auipc	ra,0xfffe
    2ccc:	8b0080e7          	jalr	-1872(ra) # 10000578 <bk_printf>
    2cd0:	4054                	c.lw	a3,4(s0)
    2cd2:	400c                	c.lw	a1,0(s0)
    2cd4:	864a                	c.mv	a2,s2
    2cd6:	00004517          	auipc	a0,0x4
    2cda:	55250513          	addi	a0,a0,1362 # 7228 <irq_handler+0x1cc>
    2cde:	54f1                	c.li	s1,-4
    2ce0:	0fffe097          	auipc	ra,0xfffe
    2ce4:	898080e7          	jalr	-1896(ra) # 10000578 <bk_printf>
    2ce8:	b751                	c.j	2c6c <__read_manage_block.constprop.0+0x68>
    2cea:	4901                	c.li	s2,0
    2cec:	b7f1                	c.j	2cb8 <__read_manage_block.constprop.0+0xb4>

00002cee <update_manage_info>:
    2cee:	c92d                	c.beqz	a0,2d60 <update_manage_info+0x72>
    2cf0:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    2cf2:	842a                	c.mv	s0,a0
    2cf4:	1141                	c.addi	sp,-16
    2cf6:	87ae                	c.mv	a5,a1
    2cf8:	5579                	c.li	a0,-2
    2cfa:	c1ad                	c.beqz	a1,2d5c <update_manage_info+0x6e>
    2cfc:	86b2                	c.mv	a3,a2
    2cfe:	abcde737          	lui	a4,0xabcde
    2d02:	4010                	c.lw	a2,0(s0)
    2d04:	cba70293          	addi	t0,a4,-838 # abcddcba <_stack+0x7bc7dcba>
    2d08:	5571                	c.li	a0,-4
    2d0a:	04561963          	bne	a2,t0,2d5c <update_manage_info+0x6e>
    2d0e:	01844483          	lbu	s1,24(s0)
    2d12:	8860                	exec.it	#52     !lw	t1,12(s0)
    2d14:	0014b093          	seqz	ra,s1
    2d18:	8c50                	exec.it	#39     !addi	t2,t1,1
    2d1a:	00840893          	addi	a7,s0,8
    2d1e:	04040593          	addi	a1,s0,64
    2d22:	4801                	c.li	a6,0
    2d24:	00140c23          	sb	ra,24(s0)
    2d28:	00742623          	sw	t2,12(s0)
    2d2c:	8640                	exec.it	#98     !lbu	a0,0(a7) # e4000000 <_stack+0xb3fa0000>
    2d2e:	0885                	c.addi	a7,1
    2d30:	982a                	c.add	a6,a0
    2d32:	ff159de3          	bne	a1,a7,2d2c <update_manage_info+0x3e>
    2d36:	01042223          	sw	a6,4(s0)
    2d3a:	438c                	c.lw	a1,0(a5)
    2d3c:	0147ae83          	lw	t4,20(a5)
    2d40:	02b08e33          	mul	t3,ra,a1
    2d44:	8636                	c.mv	a2,a3
    2d46:	c636                	c.swsp	a3,12(sp)
    2d48:	01de04b3          	add	s1,t3,t4
    2d4c:	8526                	c.mv	a0,s1
    2d4e:	8414                	exec.it	#11     !jal	ra,418e <ty_adapt_flash_erase>
    2d50:	e511                	c.bnez	a0,2d5c <update_manage_info+0x6e>
    2d52:	46b2                	c.lwsp	a3,12(sp)
    2d54:	8410                	exec.it	#3     !li	a2,64
    2d56:	85a2                	c.mv	a1,s0
    2d58:	8526                	c.mv	a0,s1
    2d5a:	8034                	exec.it	#25     !jal	ra,41bc <ty_adapt_flash_write>
    2d5c:	0141                	c.addi	sp,16
    2d5e:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>
    2d60:	5579                	c.li	a0,-2
    2d62:	8082                	c.jr	ra

00002d64 <bsdiff_mag_clear>:
    2d64:	8060                	exec.it	#48     !jal	t0,5d3a <__riscv_save_4>
    2d66:	5479                	c.li	s0,-2
    2d68:	c125                	c.beqz	a0,2dc8 <bsdiff_mag_clear+0x64>
    2d6a:	84ae                	c.mv	s1,a1
    2d6c:	5479                	c.li	s0,-2
    2d6e:	cda9                	c.beqz	a1,2dc8 <bsdiff_mag_clear+0x64>
    2d70:	01854903          	lbu	s2,24(a0)
    2d74:	419c                	c.lw	a5,0(a1)
    2d76:	00193413          	seqz	s0,s2
    2d7a:	02f400b3          	mul	ra,s0,a5
    2d7e:	0145a283          	lw	t0,20(a1)
    2d82:	00004517          	auipc	a0,0x4
    2d86:	4f250513          	addi	a0,a0,1266 # 7274 <irq_handler+0x218>
    2d8a:	00508433          	add	s0,ra,t0
    2d8e:	89b2                	c.mv	s3,a2
    2d90:	0fffd097          	auipc	ra,0xfffd
    2d94:	7e8080e7          	jalr	2024(ra) # 10000578 <bk_printf>
    2d98:	408c                	c.lw	a1,0(s1)
    2d9a:	8522                	c.mv	a0,s0
    2d9c:	864e                	c.mv	a2,s3
    2d9e:	8414                	exec.it	#11     !jal	ra,418e <ty_adapt_flash_erase>
    2da0:	842a                	c.mv	s0,a0
    2da2:	00004517          	auipc	a0,0x4
    2da6:	4de50513          	addi	a0,a0,1246 # 7280 <irq_handler+0x224>
    2daa:	0fffd097          	auipc	ra,0xfffd
    2dae:	7ce080e7          	jalr	1998(ra) # 10000578 <bk_printf>
    2db2:	e819                	c.bnez	s0,2dc8 <bsdiff_mag_clear+0x64>
    2db4:	408c                	c.lw	a1,0(s1)
    2db6:	01203933          	snez	s2,s2
    2dba:	02b90933          	mul	s2,s2,a1
    2dbe:	48c8                	c.lw	a0,20(s1)
    2dc0:	864e                	c.mv	a2,s3
    2dc2:	954a                	c.add	a0,s2
    2dc4:	8414                	exec.it	#11     !jal	ra,418e <ty_adapt_flash_erase>
    2dc6:	842a                	c.mv	s0,a0
    2dc8:	8522                	c.mv	a0,s0
    2dca:	8440                	exec.it	#34     !j	5d6e <__riscv_restore_4>

00002dcc <_flit_data_step>:
    2dcc:	8024                	exec.it	#24     !jal	t0,5d24 <__riscv_save_12>
    2dce:	1101                	c.addi	sp,-32
    2dd0:	84aa                	c.mv	s1,a0
    2dd2:	8a32                	c.mv	s4,a2
    2dd4:	8c3a                	c.mv	s8,a4
    2dd6:	8b3e                	c.mv	s6,a5
    2dd8:	8bae                	c.mv	s7,a1
    2dda:	8936                	c.mv	s2,a3
    2ddc:	8840                	exec.it	#36     !jal	ra,4188 <ty_adapt_flash_get_cfg>
    2dde:	00052883          	lw	a7,0(a0)
    2de2:	04048713          	addi	a4,s1,64
    2de6:	031a72b3          	remu	t0,s4,a7
    2dea:	478d                	c.li	a5,3
    2dec:	ca3a                	c.swsp	a4,20(sp)
    2dee:	00f48ca3          	sb	a5,25(s1)
    2df2:	8ad2                	c.mv	s5,s4
    2df4:	00028663          	beqz	t0,2e00 <_flit_data_step+0x34>
    2df8:	031a5ab3          	divu	s5,s4,a7
    2dfc:	031a8ab3          	mul	s5,s5,a7
    2e00:	000a9363          	bnez	s5,2e06 <_flit_data_step+0x3a>
    2e04:	8ad2                	c.mv	s5,s4
    2e06:	032ad0b3          	divu	ra,s5,s2
    2e0a:	8456                	c.mv	s0,s5
    2e0c:	00108313          	addi	t1,ra,1
    2e10:	cc1a                	c.swsp	t1,24(sp)
    2e12:	00008363          	beqz	ra,2e18 <_flit_data_step+0x4c>
    2e16:	844a                	c.mv	s0,s2
    2e18:	8522                	c.mv	a0,s0
    2e1a:	ce46                	c.swsp	a7,28(sp)
    2e1c:	8c30                	exec.it	#23     !jal	ra,41ec <ty_adapt_malloc>
    2e1e:	48f2                	c.lwsp	a7,28(sp)
    2e20:	89aa                	c.mv	s3,a0
    2e22:	e11d                	c.bnez	a0,2e48 <_flit_data_step+0x7c>
    2e24:	8856                	c.mv	a6,s5
    2e26:	87de                	c.mv	a5,s7
    2e28:	8762                	c.mv	a4,s8
    2e2a:	86ca                	c.mv	a3,s2
    2e2c:	8652                	c.mv	a2,s4
    2e2e:	85a2                	c.mv	a1,s0
    2e30:	00004517          	auipc	a0,0x4
    2e34:	45c50513          	addi	a0,a0,1116 # 728c <irq_handler+0x230>
    2e38:	5cf5                	c.li	s9,-3
    2e3a:	0fffd097          	auipc	ra,0xfffd
    2e3e:	73e080e7          	jalr	1854(ra) # 10000578 <bk_printf>
    2e42:	8566                	c.mv	a0,s9
    2e44:	6105                	c.addi16sp	sp,32
    2e46:	8c34                	exec.it	#31     !j	5d60 <__riscv_restore_12>
    2e48:	0144a903          	lw	s2,20(s1)
    2e4c:	00004d17          	auipc	s10,0x4
    2e50:	49cd0d13          	addi	s10,s10,1180 # 72e8 <irq_handler+0x28c>
    2e54:	00004d97          	auipc	s11,0x4
    2e58:	5ccd8d93          	addi	s11,s11,1484 # 7420 <irq_handler+0x3c4>
    2e5c:	01796663          	bltu	s2,s7,2e68 <_flit_data_step+0x9c>
    2e60:	854e                	c.mv	a0,s3
    2e62:	4c81                	c.li	s9,0
    2e64:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    2e66:	bff1                	c.j	2e42 <_flit_data_step+0x76>
    2e68:	44b4                	c.lw	a3,72(s1)
    2e6a:	45e2                	c.lwsp	a1,24(sp)
    2e6c:	88d6                	c.mv	a7,s5
    2e6e:	8862                	c.mv	a6,s8
    2e70:	87d2                	c.mv	a5,s4
    2e72:	8722                	c.mv	a4,s0
    2e74:	865e                	c.mv	a2,s7
    2e76:	856a                	c.mv	a0,s10
    2e78:	0124aa23          	sw	s2,20(s1)
    2e7c:	c04a                	c.swsp	s2,0(sp)
    2e7e:	0fffd097          	auipc	ra,0xfffd
    2e82:	6fa080e7          	jalr	1786(ra) # 10000578 <bk_printf>
    2e86:	45d2                	c.lwsp	a1,20(sp)
    2e88:	865a                	c.mv	a2,s6
    2e8a:	8526                	c.mv	a0,s1
    2e8c:	358d                	c.jal	2cee <update_manage_info>
    2e8e:	8caa                	c.mv	s9,a0
    2e90:	cd09                	c.beqz	a0,2eaa <_flit_data_step+0xde>
    2e92:	85aa                	c.mv	a1,a0
    2e94:	00004517          	auipc	a0,0x4
    2e98:	4f450513          	addi	a0,a0,1268 # 7388 <irq_handler+0x32c>
    2e9c:	0fffd097          	auipc	ra,0xfffd
    2ea0:	6dc080e7          	jalr	1756(ra) # 10000578 <bk_printf>
    2ea4:	854e                	c.mv	a0,s3
    2ea6:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    2ea8:	bf69                	c.j	2e42 <_flit_data_step+0x76>
    2eaa:	44a8                	c.lw	a0,72(s1)
    2eac:	86da                	c.mv	a3,s6
    2eae:	00aa03b3          	add	t2,s4,a0
    2eb2:	8622                	c.mv	a2,s0
    2eb4:	85ce                	c.mv	a1,s3
    2eb6:	01238533          	add	a0,t2,s2
    2eba:	8c14                	exec.it	#15     !jal	ra,41d4 <ty_adapt_flash_read>
    2ebc:	8caa                	c.mv	s9,a0
    2ebe:	c519                	c.beqz	a0,2ecc <_flit_data_step+0x100>
    2ec0:	85aa                	c.mv	a1,a0
    2ec2:	00004517          	auipc	a0,0x4
    2ec6:	4ea50513          	addi	a0,a0,1258 # 73ac <irq_handler+0x350>
    2eca:	bfc9                	c.j	2e9c <_flit_data_step+0xd0>
    2ecc:	44b4                	c.lw	a3,72(s1)
    2ece:	865a                	c.mv	a2,s6
    2ed0:	85a2                	c.mv	a1,s0
    2ed2:	00d90533          	add	a0,s2,a3
    2ed6:	8414                	exec.it	#11     !jal	ra,418e <ty_adapt_flash_erase>
    2ed8:	8caa                	c.mv	s9,a0
    2eda:	c519                	c.beqz	a0,2ee8 <_flit_data_step+0x11c>
    2edc:	85aa                	c.mv	a1,a0
    2ede:	00004517          	auipc	a0,0x4
    2ee2:	4f250513          	addi	a0,a0,1266 # 73d0 <irq_handler+0x374>
    2ee6:	bf5d                	c.j	2e9c <_flit_data_step+0xd0>
    2ee8:	0484a803          	lw	a6,72(s1)
    2eec:	86da                	c.mv	a3,s6
    2eee:	8622                	c.mv	a2,s0
    2ef0:	85ce                	c.mv	a1,s3
    2ef2:	01090533          	add	a0,s2,a6
    2ef6:	8034                	exec.it	#25     !jal	ra,41bc <ty_adapt_flash_write>
    2ef8:	8caa                	c.mv	s9,a0
    2efa:	c519                	c.beqz	a0,2f08 <_flit_data_step+0x13c>
    2efc:	85aa                	c.mv	a1,a0
    2efe:	00004517          	auipc	a0,0x4
    2f02:	4fa50513          	addi	a0,a0,1274 # 73f8 <irq_handler+0x39c>
    2f06:	bf59                	c.j	2e9c <_flit_data_step+0xd0>
    2f08:	012405b3          	add	a1,s0,s2
    2f0c:	00bbfa63          	bgeu	s7,a1,2f20 <_flit_data_step+0x154>
    2f10:	412b8433          	sub	s0,s7,s2
    2f14:	85a2                	c.mv	a1,s0
    2f16:	856e                	c.mv	a0,s11
    2f18:	0fffd097          	auipc	ra,0xfffd
    2f1c:	660080e7          	jalr	1632(ra) # 10000578 <bk_printf>
    2f20:	0484ac83          	lw	s9,72(s1)
    2f24:	865a                	c.mv	a2,s6
    2f26:	85a2                	c.mv	a1,s0
    2f28:	01990533          	add	a0,s2,s9
    2f2c:	3d7000ef          	jal	ra,3b02 <flash_crc32_cal>
    2f30:	44b0                	c.lw	a2,72(s1)
    2f32:	8caa                	c.mv	s9,a0
    2f34:	00ca0e33          	add	t3,s4,a2
    2f38:	85a2                	c.mv	a1,s0
    2f3a:	865a                	c.mv	a2,s6
    2f3c:	012e0533          	add	a0,t3,s2
    2f40:	3c3000ef          	jal	ra,3b02 <flash_crc32_cal>
    2f44:	862a                	c.mv	a2,a0
    2f46:	02ac8063          	beq	s9,a0,2f66 <_flit_data_step+0x19a>
    2f4a:	85e6                	c.mv	a1,s9
    2f4c:	86a2                	c.mv	a3,s0
    2f4e:	00004517          	auipc	a0,0x4
    2f52:	4e250513          	addi	a0,a0,1250 # 7430 <irq_handler+0x3d4>
    2f56:	0fffd097          	auipc	ra,0xfffd
    2f5a:	622080e7          	jalr	1570(ra) # 10000578 <bk_printf>
    2f5e:	854e                	c.mv	a0,s3
    2f60:	4c99                	c.li	s9,6
    2f62:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    2f64:	bdf9                	c.j	2e42 <_flit_data_step+0x76>
    2f66:	9922                	c.add	s2,s0
    2f68:	bdd5                	c.j	2e5c <_flit_data_step+0x90>

00002f6a <_bkup_2_frmware>:
    2f6a:	5d1022ef          	jal	t0,5d3a <__riscv_save_4>
    2f6e:	84be                	c.mv	s1,a5
    2f70:	01954783          	lbu	a5,25(a0)
    2f74:	0427e45b          	bnec	a5,2,2fbc <_bkup_2_frmware+0x52>
    2f78:	8a32                	c.mv	s4,a2
    2f7a:	8936                	c.mv	s2,a3
    2f7c:	842a                	c.mv	s0,a0
    2f7e:	4928                	c.lw	a0,80(a0)
    2f80:	89ae                	c.mv	s3,a1
    2f82:	86a6                	c.mv	a3,s1
    2f84:	864a                	c.mv	a2,s2
    2f86:	85d2                	c.mv	a1,s4
    2f88:	8aba                	c.mv	s5,a4
    2f8a:	8c14                	exec.it	#15     !jal	ra,41d4 <ty_adapt_flash_read>
    2f8c:	e515                	c.bnez	a0,2fb8 <_bkup_2_frmware+0x4e>
    2f8e:	8626                	c.mv	a2,s1
    2f90:	85ca                	c.mv	a1,s2
    2f92:	854e                	c.mv	a0,s3
    2f94:	8414                	exec.it	#11     !jal	ra,418e <ty_adapt_flash_erase>
    2f96:	e10d                	c.bnez	a0,2fb8 <_bkup_2_frmware+0x4e>
    2f98:	86a6                	c.mv	a3,s1
    2f9a:	864a                	c.mv	a2,s2
    2f9c:	85d2                	c.mv	a1,s4
    2f9e:	854e                	c.mv	a0,s3
    2fa0:	8034                	exec.it	#25     !jal	ra,41bc <ty_adapt_flash_write>
    2fa2:	e919                	c.bnez	a0,2fb8 <_bkup_2_frmware+0x4e>
    2fa4:	000a9a63          	bnez	s5,2fb8 <_bkup_2_frmware+0x4e>
    2fa8:	4085                	c.li	ra,1
    2faa:	8626                	c.mv	a2,s1
    2fac:	04040593          	addi	a1,s0,64
    2fb0:	8522                	c.mv	a0,s0
    2fb2:	00140ca3          	sb	ra,25(s0)
    2fb6:	3b25                	c.jal	2cee <update_manage_info>
    2fb8:	5b70206f          	j	5d6e <__riscv_restore_4>
    2fbc:	557d                	c.li	a0,-1
    2fbe:	bfed                	c.j	2fb8 <_bkup_2_frmware+0x4e>

00002fc0 <_buff_2_bkup_2_frmware>:
    2fc0:	56d022ef          	jal	t0,5d2c <__riscv_save_10>
    2fc4:	01954a83          	lbu	s5,25(a0)
    2fc8:	061ae55b          	bnec	s5,1,3032 <_buff_2_bkup_2_frmware+0x72>
    2fcc:	842a                	c.mv	s0,a0
    2fce:	4928                	c.lw	a0,80(a0)
    2fd0:	8c2e                	c.mv	s8,a1
    2fd2:	89b2                	c.mv	s3,a2
    2fd4:	85ba                	c.mv	a1,a4
    2fd6:	8642                	c.mv	a2,a6
    2fd8:	8b36                	c.mv	s6,a3
    2fda:	893a                	c.mv	s2,a4
    2fdc:	8a3e                	c.mv	s4,a5
    2fde:	84c2                	c.mv	s1,a6
    2fe0:	8414                	exec.it	#11     !jal	ra,418e <ty_adapt_flash_erase>
    2fe2:	e531                	c.bnez	a0,302e <_buff_2_bkup_2_frmware+0x6e>
    2fe4:	4828                	c.lw	a0,80(s0)
    2fe6:	86a6                	c.mv	a3,s1
    2fe8:	864a                	c.mv	a2,s2
    2fea:	85da                	c.mv	a1,s6
    2fec:	8034                	exec.it	#25     !jal	ra,41bc <ty_adapt_flash_write>
    2fee:	e121                	c.bnez	a0,302e <_buff_2_bkup_2_frmware+0x6e>
    2ff0:	04040b93          	addi	s7,s0,64
    2ff4:	4789                	c.li	a5,2
    2ff6:	8626                	c.mv	a2,s1
    2ff8:	85de                	c.mv	a1,s7
    2ffa:	8522                	c.mv	a0,s0
    2ffc:	00f40ca3          	sb	a5,25(s0)
    3000:	01842823          	sw	s8,16(s0)
    3004:	31ed                	c.jal	2cee <update_manage_info>
    3006:	e505                	c.bnez	a0,302e <_buff_2_bkup_2_frmware+0x6e>
    3008:	8626                	c.mv	a2,s1
    300a:	85ca                	c.mv	a1,s2
    300c:	854e                	c.mv	a0,s3
    300e:	8414                	exec.it	#11     !jal	ra,418e <ty_adapt_flash_erase>
    3010:	ed19                	c.bnez	a0,302e <_buff_2_bkup_2_frmware+0x6e>
    3012:	86a6                	c.mv	a3,s1
    3014:	864a                	c.mv	a2,s2
    3016:	85da                	c.mv	a1,s6
    3018:	854e                	c.mv	a0,s3
    301a:	8034                	exec.it	#25     !jal	ra,41bc <ty_adapt_flash_write>
    301c:	e909                	c.bnez	a0,302e <_buff_2_bkup_2_frmware+0x6e>
    301e:	000a1863          	bnez	s4,302e <_buff_2_bkup_2_frmware+0x6e>
    3022:	8626                	c.mv	a2,s1
    3024:	85de                	c.mv	a1,s7
    3026:	8522                	c.mv	a0,s0
    3028:	01540ca3          	sb	s5,25(s0)
    302c:	31c9                	c.jal	2cee <update_manage_info>
    302e:	5370206f          	j	5d64 <__riscv_restore_10>
    3032:	557d                	c.li	a0,-1
    3034:	bfed                	c.j	302e <_buff_2_bkup_2_frmware+0x6e>

00003036 <bsdiff_flash_init>:
    3036:	4f7022ef          	jal	t0,5d2c <__riscv_save_10>
    303a:	842a                	c.mv	s0,a0
    303c:	89ae                	c.mv	s3,a1
    303e:	8932                	c.mv	s2,a2
    3040:	8bb6                	c.mv	s7,a3
    3042:	8840                	exec.it	#36     !jal	ra,4188 <ty_adapt_flash_get_cfg>
    3044:	e509                	c.bnez	a0,304e <bsdiff_flash_init+0x18>
    3046:	54fd                	c.li	s1,-1
    3048:	8526                	c.mv	a0,s1
    304a:	51b0206f          	j	5d64 <__riscv_restore_10>
    304e:	8b2a                	c.mv	s6,a0
    3050:	85aa                	c.mv	a1,a0
    3052:	4621                	c.li	a2,8
    3054:	854e                	c.mv	a0,s3
    3056:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    3058:	004b2783          	lw	a5,4(s6) # 10004 <_data_lmastart+0x66be>
    305c:	54f9                	c.li	s1,-2
    305e:	00f9aa23          	sw	a5,20(s3)
    3062:	d07d                	c.beqz	s0,3048 <bsdiff_flash_init+0x12>
    3064:	fe0982e3          	beqz	s3,3048 <bsdiff_flash_init+0x12>
    3068:	8474                	exec.it	#59     !li	a0,64
    306a:	8c30                	exec.it	#23     !jal	ra,41ec <ty_adapt_malloc>
    306c:	54f5                	c.li	s1,-3
    306e:	8a2a                	c.mv	s4,a0
    3070:	dd61                	c.beqz	a0,3048 <bsdiff_flash_init+0x12>
    3072:	8474                	exec.it	#59     !li	a0,64
    3074:	8c30                	exec.it	#23     !jal	ra,41ec <ty_adapt_malloc>
    3076:	8aaa                	c.mv	s5,a0
    3078:	e501                	c.bnez	a0,3080 <bsdiff_flash_init+0x4a>
    307a:	8552                	c.mv	a0,s4
    307c:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    307e:	b7e9                	c.j	3048 <bsdiff_flash_init+0x12>
    3080:	86de                	c.mv	a3,s7
    3082:	8652                	c.mv	a2,s4
    3084:	4581                	c.li	a1,0
    3086:	854e                	c.mv	a0,s3
    3088:	3eb5                	c.jal	2c04 <__read_manage_block.constprop.0>
    308a:	84aa                	c.mv	s1,a0
    308c:	20050363          	beqz	a0,3292 <bsdiff_flash_init+0x25c>
    3090:	50f1                	c.li	ra,-4
    3092:	02151f63          	bne	a0,ra,30d0 <bsdiff_flash_init+0x9a>
    3096:	4c85                	c.li	s9,1
    3098:	86de                	c.mv	a3,s7
    309a:	8656                	c.mv	a2,s5
    309c:	4585                	c.li	a1,1
    309e:	854e                	c.mv	a0,s3
    30a0:	3695                	c.jal	2c04 <__read_manage_block.constprop.0>
    30a2:	84aa                	c.mv	s1,a0
    30a4:	1e050963          	beqz	a0,3296 <bsdiff_flash_init+0x260>
    30a8:	52f1                	c.li	t0,-4
    30aa:	02551363          	bne	a0,t0,30d0 <bsdiff_flash_init+0x9a>
    30ae:	4c05                	c.li	s8,1
    30b0:	8662                	c.mv	a2,s8
    30b2:	85e6                	c.mv	a1,s9
    30b4:	00004517          	auipc	a0,0x4
    30b8:	3c850513          	addi	a0,a0,968 # 747c <irq_handler+0x420>
    30bc:	0fffd097          	auipc	ra,0xfffd
    30c0:	4bc080e7          	jalr	1212(ra) # 10000578 <bk_printf>
    30c4:	1c0c8b63          	beqz	s9,329a <bsdiff_flash_init+0x264>
    30c8:	54ed                	c.li	s1,-5
    30ca:	8410                	exec.it	#3     !li	a2,64
    30cc:	1e0c0163          	beqz	s8,32ae <bsdiff_flash_init+0x278>
    30d0:	8552                	c.mv	a0,s4
    30d2:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    30d4:	8556                	c.mv	a0,s5
    30d6:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    30d8:	f8a5                	c.bnez	s1,3048 <bsdiff_flash_init+0x12>
    30da:	01c42383          	lw	t2,28(s0)
    30de:	0009a483          	lw	s1,0(s3)
    30e2:	0079a423          	sw	t2,8(s3)
    30e6:	4410                	c.lw	a2,8(s0)
    30e8:	029675b3          	remu	a1,a2,s1
    30ec:	c589                	c.beqz	a1,30f6 <bsdiff_flash_init+0xc0>
    30ee:	00c48533          	add	a0,s1,a2
    30f2:	40b50633          	sub	a2,a0,a1
    30f6:	01a44683          	lbu	a3,26(s0)
    30fa:	1c069d63          	bnez	a3,32d4 <bsdiff_flash_init+0x29e>
    30fe:	0149a803          	lw	a6,20(s3)
    3102:	40c80533          	sub	a0,a6,a2
    3106:	00a9a623          	sw	a0,12(s3)
    310a:	000b2883          	lw	a7,0(s6)
    310e:	01c42e03          	lw	t3,28(s0)
    3112:	031e7eb3          	remu	t4,t3,a7
    3116:	f20e98e3          	bnez	t4,3046 <bsdiff_flash_init+0x10>
    311a:	02042f03          	lw	t5,32(s0)
    311e:	031f7fb3          	remu	t6,t5,a7
    3122:	f20f92e3          	bnez	t6,3046 <bsdiff_flash_init+0x10>
    3126:	86de                	c.mv	a3,s7
    3128:	8410                	exec.it	#3     !li	a2,64
    312a:	85ca                	c.mv	a1,s2
    312c:	8c14                	exec.it	#15     !jal	ra,41d4 <ty_adapt_flash_read>
    312e:	84aa                	c.mv	s1,a0
    3130:	f0051ce3          	bnez	a0,3048 <bsdiff_flash_init+0x12>
    3134:	00004517          	auipc	a0,0x4
    3138:	3b850513          	addi	a0,a0,952 # 74ec <irq_handler+0x490>
    313c:	0fffd097          	auipc	ra,0xfffd
    3140:	43c080e7          	jalr	1084(ra) # 10000578 <bk_printf>
    3144:	8054                	exec.it	#41     !lw	a1,0(s2)
    3146:	00004517          	auipc	a0,0x4
    314a:	3e650513          	addi	a0,a0,998 # 752c <irq_handler+0x4d0>
    314e:	0fffd097          	auipc	ra,0xfffd
    3152:	42a080e7          	jalr	1066(ra) # 10000578 <bk_printf>
    3156:	00492583          	lw	a1,4(s2)
    315a:	00004517          	auipc	a0,0x4
    315e:	3f650513          	addi	a0,a0,1014 # 7550 <irq_handler+0x4f4>
    3162:	0fffd097          	auipc	ra,0xfffd
    3166:	416080e7          	jalr	1046(ra) # 10000578 <bk_printf>
    316a:	00892583          	lw	a1,8(s2)
    316e:	00004517          	auipc	a0,0x4
    3172:	40650513          	addi	a0,a0,1030 # 7574 <irq_handler+0x518>
    3176:	0fffd097          	auipc	ra,0xfffd
    317a:	402080e7          	jalr	1026(ra) # 10000578 <bk_printf>
    317e:	00c92583          	lw	a1,12(s2)
    3182:	00004517          	auipc	a0,0x4
    3186:	41650513          	addi	a0,a0,1046 # 7598 <irq_handler+0x53c>
    318a:	0fffd097          	auipc	ra,0xfffd
    318e:	3ee080e7          	jalr	1006(ra) # 10000578 <bk_printf>
    3192:	01092583          	lw	a1,16(s2)
    3196:	00004517          	auipc	a0,0x4
    319a:	42650513          	addi	a0,a0,1062 # 75bc <irq_handler+0x560>
    319e:	0fffd097          	auipc	ra,0xfffd
    31a2:	3da080e7          	jalr	986(ra) # 10000578 <bk_printf>
    31a6:	01492583          	lw	a1,20(s2)
    31aa:	00004517          	auipc	a0,0x4
    31ae:	43650513          	addi	a0,a0,1078 # 75e0 <irq_handler+0x584>
    31b2:	0fffd097          	auipc	ra,0xfffd
    31b6:	3c6080e7          	jalr	966(ra) # 10000578 <bk_printf>
    31ba:	01892583          	lw	a1,24(s2)
    31be:	00004517          	auipc	a0,0x4
    31c2:	44650513          	addi	a0,a0,1094 # 7604 <irq_handler+0x5a8>
    31c6:	0fffd097          	auipc	ra,0xfffd
    31ca:	3b2080e7          	jalr	946(ra) # 10000578 <bk_printf>
    31ce:	01c92583          	lw	a1,28(s2)
    31d2:	00004517          	auipc	a0,0x4
    31d6:	45650513          	addi	a0,a0,1110 # 7628 <irq_handler+0x5cc>
    31da:	0fffd097          	auipc	ra,0xfffd
    31de:	39e080e7          	jalr	926(ra) # 10000578 <bk_printf>
    31e2:	02092583          	lw	a1,32(s2)
    31e6:	00004517          	auipc	a0,0x4
    31ea:	46a50513          	addi	a0,a0,1130 # 7650 <irq_handler+0x5f4>
    31ee:	0fffd097          	auipc	ra,0xfffd
    31f2:	38a080e7          	jalr	906(ra) # 10000578 <bk_printf>
    31f6:	02492583          	lw	a1,36(s2)
    31fa:	00004517          	auipc	a0,0x4
    31fe:	47a50513          	addi	a0,a0,1146 # 7674 <irq_handler+0x618>
    3202:	0fffd097          	auipc	ra,0xfffd
    3206:	376080e7          	jalr	886(ra) # 10000578 <bk_printf>
    320a:	02892583          	lw	a1,40(s2)
    320e:	00004517          	auipc	a0,0x4
    3212:	48a50513          	addi	a0,a0,1162 # 7698 <irq_handler+0x63c>
    3216:	0fffd097          	auipc	ra,0xfffd
    321a:	362080e7          	jalr	866(ra) # 10000578 <bk_printf>
    321e:	02c92583          	lw	a1,44(s2)
    3222:	00004517          	auipc	a0,0x4
    3226:	49650513          	addi	a0,a0,1174 # 76b8 <irq_handler+0x65c>
    322a:	0fffd097          	auipc	ra,0xfffd
    322e:	34e080e7          	jalr	846(ra) # 10000578 <bk_printf>
    3232:	03094583          	lbu	a1,48(s2)
    3236:	00004517          	auipc	a0,0x4
    323a:	4a250513          	addi	a0,a0,1186 # 76d8 <irq_handler+0x67c>
    323e:	0fffd097          	auipc	ra,0xfffd
    3242:	33a080e7          	jalr	826(ra) # 10000578 <bk_printf>
    3246:	03295583          	lhu	a1,50(s2)
    324a:	00004517          	auipc	a0,0x4
    324e:	4aa50513          	addi	a0,a0,1194 # 76f4 <irq_handler+0x698>
    3252:	0fffd097          	auipc	ra,0xfffd
    3256:	326080e7          	jalr	806(ra) # 10000578 <bk_printf>
    325a:	00004517          	auipc	a0,0x4
    325e:	4c250513          	addi	a0,a0,1218 # 771c <irq_handler+0x6c0>
    3262:	0fffd097          	auipc	ra,0xfffd
    3266:	316080e7          	jalr	790(ra) # 10000578 <bk_printf>
    326a:	8840                	exec.it	#36     !jal	ra,4188 <ty_adapt_flash_get_cfg>
    326c:	534957b7          	lui	a5,0x53495
    3270:	00492583          	lw	a1,4(s2)
    3274:	e4778293          	addi	t0,a5,-441 # 53494e47 <_stack+0x23434e47>
    3278:	8a2a                	c.mv	s4,a0
    327a:	04558f63          	beq	a1,t0,32d8 <bsdiff_flash_init+0x2a2>
    327e:	00004517          	auipc	a0,0x4
    3282:	4de50513          	addi	a0,a0,1246 # 775c <irq_handler+0x700>
    3286:	44a9                	c.li	s1,10
    3288:	0fffd097          	auipc	ra,0xfffd
    328c:	2f0080e7          	jalr	752(ra) # 10000578 <bk_printf>
    3290:	bb65                	c.j	3048 <bsdiff_flash_init+0x12>
    3292:	4c81                	c.li	s9,0
    3294:	b511                	c.j	3098 <bsdiff_flash_init+0x62>
    3296:	4c01                	c.li	s8,0
    3298:	bd21                	c.j	30b0 <bsdiff_flash_init+0x7a>
    329a:	8410                	exec.it	#3     !li	a2,64
    329c:	020c1a63          	bnez	s8,32d0 <bsdiff_flash_init+0x29a>
    32a0:	00ca2703          	lw	a4,12(s4)
    32a4:	00caa303          	lw	t1,12(s5)
    32a8:	8410                	exec.it	#3     !li	a2,64
    32aa:	02677363          	bgeu	a4,t1,32d0 <bsdiff_flash_init+0x29a>
    32ae:	85d6                	c.mv	a1,s5
    32b0:	8522                	c.mv	a0,s0
    32b2:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    32b4:	01944683          	lbu	a3,25(s0)
    32b8:	4810                	c.lw	a2,16(s0)
    32ba:	8614                	exec.it	#75     !lbu	a1,24(s0)
    32bc:	00004517          	auipc	a0,0x4
    32c0:	1ec50513          	addi	a0,a0,492 # 74a8 <irq_handler+0x44c>
    32c4:	4481                	c.li	s1,0
    32c6:	0fffd097          	auipc	ra,0xfffd
    32ca:	2b2080e7          	jalr	690(ra) # 10000578 <bk_printf>
    32ce:	b509                	c.j	30d0 <bsdiff_flash_init+0x9a>
    32d0:	85d2                	c.mv	a1,s4
    32d2:	bff9                	c.j	32b0 <bsdiff_flash_init+0x27a>
    32d4:	5408                	c.lw	a0,40(s0)
    32d6:	bd05                	c.j	3106 <bsdiff_flash_init+0xd0>
    32d8:	03295603          	lhu	a2,50(s2)
    32dc:	4089                	c.li	ra,2
    32de:	4589                	c.li	a1,2
    32e0:	00c0fc63          	bgeu	ra,a2,32f8 <bsdiff_flash_init+0x2c2>
    32e4:	00004517          	auipc	a0,0x4
    32e8:	48c50513          	addi	a0,a0,1164 # 7770 <irq_handler+0x714>
    32ec:	449d                	c.li	s1,7
    32ee:	0fffd097          	auipc	ra,0xfffd
    32f2:	28a080e7          	jalr	650(ra) # 10000578 <bk_printf>
    32f6:	bb89                	c.j	3048 <bsdiff_flash_init+0x12>
    32f8:	00004517          	auipc	a0,0x4
    32fc:	49850513          	addi	a0,a0,1176 # 7790 <irq_handler+0x734>
    3300:	0fffd097          	auipc	ra,0xfffd
    3304:	278080e7          	jalr	632(ra) # 10000578 <bk_printf>
    3308:	02c92583          	lw	a1,44(s2)
    330c:	000a2703          	lw	a4,0(s4)
    3310:	02e5f333          	remu	t1,a1,a4
    3314:	00030c63          	beqz	t1,332c <bsdiff_flash_init+0x2f6>
    3318:	00004517          	auipc	a0,0x4
    331c:	4a450513          	addi	a0,a0,1188 # 77bc <irq_handler+0x760>
    3320:	44a1                	c.li	s1,8
    3322:	0fffd097          	auipc	ra,0xfffd
    3326:	256080e7          	jalr	598(ra) # 10000578 <bk_printf>
    332a:	bb39                	c.j	3048 <bsdiff_flash_init+0x12>
    332c:	01a44383          	lbu	t2,26(s0)
    3330:	1a039b63          	bnez	t2,34e6 <bsdiff_flash_init+0x4b0>
    3334:	00c9a503          	lw	a0,12(s3)
    3338:	40b505b3          	sub	a1,a0,a1
    333c:	00b9a823          	sw	a1,16(s3)
    3340:	400c                	c.lw	a1,0(s0)
    3342:	00004517          	auipc	a0,0x4
    3346:	48e50513          	addi	a0,a0,1166 # 77d0 <irq_handler+0x774>
    334a:	0fffd097          	auipc	ra,0xfffd
    334e:	22e080e7          	jalr	558(ra) # 10000578 <bk_printf>
    3352:	404c                	c.lw	a1,4(s0)
    3354:	00004517          	auipc	a0,0x4
    3358:	49450513          	addi	a0,a0,1172 # 77e8 <irq_handler+0x78c>
    335c:	0fffd097          	auipc	ra,0xfffd
    3360:	21c080e7          	jalr	540(ra) # 10000578 <bk_printf>
    3364:	440c                	c.lw	a1,8(s0)
    3366:	00004517          	auipc	a0,0x4
    336a:	49a50513          	addi	a0,a0,1178 # 7800 <irq_handler+0x7a4>
    336e:	0fffd097          	auipc	ra,0xfffd
    3372:	20a080e7          	jalr	522(ra) # 10000578 <bk_printf>
    3376:	444c                	c.lw	a1,12(s0)
    3378:	00004517          	auipc	a0,0x4
    337c:	4a450513          	addi	a0,a0,1188 # 781c <irq_handler+0x7c0>
    3380:	0fffd097          	auipc	ra,0xfffd
    3384:	1f8080e7          	jalr	504(ra) # 10000578 <bk_printf>
    3388:	480c                	c.lw	a1,16(s0)
    338a:	00004517          	auipc	a0,0x4
    338e:	4aa50513          	addi	a0,a0,1194 # 7834 <irq_handler+0x7d8>
    3392:	0fffd097          	auipc	ra,0xfffd
    3396:	1e6080e7          	jalr	486(ra) # 10000578 <bk_printf>
    339a:	484c                	c.lw	a1,20(s0)
    339c:	00004517          	auipc	a0,0x4
    33a0:	4b050513          	addi	a0,a0,1200 # 784c <irq_handler+0x7f0>
    33a4:	0fffd097          	auipc	ra,0xfffd
    33a8:	1d4080e7          	jalr	468(ra) # 10000578 <bk_printf>
    33ac:	8614                	exec.it	#75     !lbu	a1,24(s0)
    33ae:	00004517          	auipc	a0,0x4
    33b2:	4ba50513          	addi	a0,a0,1210 # 7868 <irq_handler+0x80c>
    33b6:	0fffd097          	auipc	ra,0xfffd
    33ba:	1c2080e7          	jalr	450(ra) # 10000578 <bk_printf>
    33be:	01944583          	lbu	a1,25(s0)
    33c2:	00004517          	auipc	a0,0x4
    33c6:	4be50513          	addi	a0,a0,1214 # 7880 <irq_handler+0x824>
    33ca:	0fffd097          	auipc	ra,0xfffd
    33ce:	1ae080e7          	jalr	430(ra) # 10000578 <bk_printf>
    33d2:	01a44583          	lbu	a1,26(s0)
    33d6:	00004517          	auipc	a0,0x4
    33da:	4c250513          	addi	a0,a0,1218 # 7898 <irq_handler+0x83c>
    33de:	0fffd097          	auipc	ra,0xfffd
    33e2:	19a080e7          	jalr	410(ra) # 10000578 <bk_printf>
    33e6:	4c4c                	c.lw	a1,28(s0)
    33e8:	00004517          	auipc	a0,0x4
    33ec:	4d050513          	addi	a0,a0,1232 # 78b8 <irq_handler+0x85c>
    33f0:	0fffd097          	auipc	ra,0xfffd
    33f4:	188080e7          	jalr	392(ra) # 10000578 <bk_printf>
    33f8:	500c                	c.lw	a1,32(s0)
    33fa:	00004517          	auipc	a0,0x4
    33fe:	4e250513          	addi	a0,a0,1250 # 78dc <irq_handler+0x880>
    3402:	0fffd097          	auipc	ra,0xfffd
    3406:	176080e7          	jalr	374(ra) # 10000578 <bk_printf>
    340a:	504c                	c.lw	a1,36(s0)
    340c:	00004517          	auipc	a0,0x4
    3410:	4ec50513          	addi	a0,a0,1260 # 78f8 <irq_handler+0x89c>
    3414:	0fffd097          	auipc	ra,0xfffd
    3418:	164080e7          	jalr	356(ra) # 10000578 <bk_printf>
    341c:	540c                	c.lw	a1,40(s0)
    341e:	00004517          	auipc	a0,0x4
    3422:	50250513          	addi	a0,a0,1282 # 7920 <irq_handler+0x8c4>
    3426:	0fffd097          	auipc	ra,0xfffd
    342a:	152080e7          	jalr	338(ra) # 10000578 <bk_printf>
    342e:	0089a583          	lw	a1,8(s3)
    3432:	00004517          	auipc	a0,0x4
    3436:	51650513          	addi	a0,a0,1302 # 7948 <irq_handler+0x8ec>
    343a:	0fffd097          	auipc	ra,0xfffd
    343e:	13e080e7          	jalr	318(ra) # 10000578 <bk_printf>
    3442:	00c9a583          	lw	a1,12(s3)
    3446:	00004517          	auipc	a0,0x4
    344a:	51e50513          	addi	a0,a0,1310 # 7964 <irq_handler+0x908>
    344e:	0fffd097          	auipc	ra,0xfffd
    3452:	12a080e7          	jalr	298(ra) # 10000578 <bk_printf>
    3456:	0109a583          	lw	a1,16(s3)
    345a:	00004517          	auipc	a0,0x4
    345e:	52650513          	addi	a0,a0,1318 # 7980 <irq_handler+0x924>
    3462:	0fffd097          	auipc	ra,0xfffd
    3466:	116080e7          	jalr	278(ra) # 10000578 <bk_printf>
    346a:	0149a583          	lw	a1,20(s3)
    346e:	00004517          	auipc	a0,0x4
    3472:	52e50513          	addi	a0,a0,1326 # 799c <irq_handler+0x940>
    3476:	0fffd097          	auipc	ra,0xfffd
    347a:	102080e7          	jalr	258(ra) # 10000578 <bk_printf>
    347e:	01492603          	lw	a2,20(s2)
    3482:	01092583          	lw	a1,16(s2)
    3486:	00c5f363          	bgeu	a1,a2,348c <bsdiff_flash_init+0x456>
    348a:	85b2                	c.mv	a1,a2
    348c:	000b2683          	lw	a3,0(s6)
    3490:	02d5f833          	remu	a6,a1,a3
    3494:	00080663          	beqz	a6,34a0 <bsdiff_flash_init+0x46a>
    3498:	00b688b3          	add	a7,a3,a1
    349c:	410885b3          	sub	a1,a7,a6
    34a0:	02092e03          	lw	t3,32(s2)
    34a4:	02892e83          	lw	t4,40(s2)
    34a8:	02c92f83          	lw	t6,44(s2)
    34ac:	8460                	exec.it	#50     !add	t5,t3,t4
    34ae:	01ff07b3          	add	a5,t5,t6
    34b2:	0ad782db          	lea.h	t0,a5,a3
    34b6:	00b28633          	add	a2,t0,a1
    34ba:	02d670b3          	remu	ra,a2,a3
    34be:	00008663          	beqz	ra,34ca <bsdiff_flash_init+0x494>
    34c2:	00c68733          	add	a4,a3,a2
    34c6:	40170633          	sub	a2,a4,ra
    34ca:	02042303          	lw	t1,32(s0)
    34ce:	00c37e63          	bgeu	t1,a2,34ea <bsdiff_flash_init+0x4b4>
    34d2:	00004517          	auipc	a0,0x4
    34d6:	4e650513          	addi	a0,a0,1254 # 79b8 <irq_handler+0x95c>
    34da:	4495                	c.li	s1,5
    34dc:	0fffd097          	auipc	ra,0xfffd
    34e0:	09c080e7          	jalr	156(ra) # 10000578 <bk_printf>
    34e4:	b695                	c.j	3048 <bsdiff_flash_init+0x12>
    34e6:	504c                	c.lw	a1,36(s0)
    34e8:	bd91                	c.j	333c <bsdiff_flash_init+0x306>
    34ea:	00004517          	auipc	a0,0x4
    34ee:	50250513          	addi	a0,a0,1282 # 79ec <irq_handler+0x990>
    34f2:	0fffd097          	auipc	ra,0xfffd
    34f6:	086080e7          	jalr	134(ra) # 10000578 <bk_printf>
    34fa:	00004517          	auipc	a0,0x4
    34fe:	22250513          	addi	a0,a0,546 # 771c <irq_handler+0x6c0>
    3502:	0fffd097          	auipc	ra,0xfffd
    3506:	076080e7          	jalr	118(ra) # 10000578 <bk_printf>
    350a:	be3d                	c.j	3048 <bsdiff_flash_init+0x12>

0000350c <offtin>:
    350c:	00754683          	lbu	a3,7(a0)
    3510:	00654703          	lbu	a4,6(a0)
    3514:	20e6a7db          	bfoz	a5,a3,8,14
    3518:	00f702b3          	add	t0,a4,a5
    351c:	00554583          	lbu	a1,5(a0)
    3520:	00829393          	slli	t2,t0,0x8
    3524:	00758833          	add	a6,a1,t2
    3528:	00e2b333          	sltu	t1,t0,a4
    352c:	00b838b3          	sltu	a7,a6,a1
    3530:	00454703          	lbu	a4,4(a0)
    3534:	00831613          	slli	a2,t1,0x8
    3538:	00c88e33          	add	t3,a7,a2
    353c:	00881793          	slli	a5,a6,0x8
    3540:	00f702b3          	add	t0,a4,a5
    3544:	01885e93          	srli	t4,a6,0x18
    3548:	008e1f13          	slli	t5,t3,0x8
    354c:	01eeefb3          	or	t6,t4,t5
    3550:	00354e03          	lbu	t3,3(a0)
    3554:	00e2b333          	sltu	t1,t0,a4
    3558:	01f303b3          	add	t2,t1,t6
    355c:	00829893          	slli	a7,t0,0x8
    3560:	011e0eb3          	add	t4,t3,a7
    3564:	0182d613          	srli	a2,t0,0x18
    3568:	00839593          	slli	a1,t2,0x8
    356c:	00b66833          	or	a6,a2,a1
    3570:	00254383          	lbu	t2,2(a0)
    3574:	01cebf33          	sltu	t5,t4,t3
    3578:	010f0fb3          	add	t6,t5,a6
    357c:	008e9313          	slli	t1,t4,0x8
    3580:	00638633          	add	a2,t2,t1
    3584:	018ed793          	srli	a5,t4,0x18
    3588:	008f9713          	slli	a4,t6,0x8
    358c:	00e7e2b3          	or	t0,a5,a4
    3590:	007635b3          	sltu	a1,a2,t2
    3594:	00154f83          	lbu	t6,1(a0)
    3598:	00558833          	add	a6,a1,t0
    359c:	00861f13          	slli	t5,a2,0x8
    35a0:	01865893          	srli	a7,a2,0x18
    35a4:	01ef87b3          	add	a5,t6,t5
    35a8:	00881e13          	slli	t3,a6,0x8
    35ac:	00054583          	lbu	a1,0(a0)
    35b0:	01c8eeb3          	or	t4,a7,t3
    35b4:	01f7b733          	sltu	a4,a5,t6
    35b8:	01d702b3          	add	t0,a4,t4
    35bc:	00879813          	slli	a6,a5,0x8
    35c0:	01058533          	add	a0,a1,a6
    35c4:	0187d313          	srli	t1,a5,0x18
    35c8:	00829393          	slli	t2,t0,0x8
    35cc:	00736633          	or	a2,t1,t2
    35d0:	00b538b3          	sltu	a7,a0,a1
    35d4:	1c06b6db          	bfos	a3,a3,7,0
    35d8:	00c885b3          	add	a1,a7,a2
    35dc:	0006da63          	bgez	a3,35f0 <offtin+0xe4>
    35e0:	00a03e33          	snez	t3,a0
    35e4:	40b00eb3          	neg	t4,a1
    35e8:	41ce85b3          	sub	a1,t4,t3
    35ec:	40a00533          	neg	a0,a0
    35f0:	8082                	c.jr	ra

000035f2 <bspatch_core>:
    35f2:	8024                	exec.it	#24     !jal	t0,5d24 <__riscv_save_12>
    35f4:	714d                	c.addi16sp	sp,-336
    35f6:	84c2                	c.mv	s1,a6
    35f8:	8b36                	c.mv	s6,a3
    35fa:	893e                	c.mv	s2,a5
    35fc:	8a46                	c.mv	s4,a7
    35fe:	ca32                	c.swsp	a2,20(sp)
    3600:	c63a                	c.swsp	a4,12(sp)
    3602:	8dae                	c.mv	s11,a1
    3604:	19012a83          	lw	s5,400(sp)
    3608:	8840                	exec.it	#36     !jal	ra,4188 <ty_adapt_flash_get_cfg>
    360a:	c82a                	c.swsp	a0,16(sp)
    360c:	8210                	exec.it	#65     !jal	ra,4216 <ty_adapter_get_free_heap_size>
    360e:	85aa                	c.mv	a1,a0
    3610:	00004517          	auipc	a0,0x4
    3614:	40c50513          	addi	a0,a0,1036 # 7a1c <irq_handler+0x9c0>
    3618:	0fffd097          	auipc	ra,0xfffd
    361c:	f60080e7          	jalr	-160(ra) # 10000578 <bk_printf>
    3620:	8526                	c.mv	a0,s1
    3622:	59f5                	c.li	s3,-3
    3624:	8c30                	exec.it	#23     !jal	ra,41ec <ty_adapt_malloc>
    3626:	c931                	c.beqz	a0,367a <bspatch_core+0x88>
    3628:	07c00613          	li	a2,124
    362c:	4581                	c.li	a1,0
    362e:	842a                	c.mv	s0,a0
    3630:	0888                	c.addi4spn	a0,sp,80
    3632:	c682                	c.swsp	zero,76(sp)
    3634:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    3636:	02c00613          	li	a2,44
    363a:	4581                	c.li	a1,0
    363c:	1008                	c.addi4spn	a0,sp,32
    363e:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    3640:	004a2783          	lw	a5,4(s4)
    3644:	8004                	exec.it	#8     !lw	a3,408(sp)
    3646:	8c24                	exec.it	#30     !li	a2,32
    3648:	00ec                	c.addi4spn	a1,sp,76
    364a:	8552                	c.mv	a0,s4
    364c:	9782                	c.jalr	a5
    364e:	00055bdb          	beqc	a0,32,3664 <bspatch_core+0x72>
    3652:	00004517          	auipc	a0,0x4
    3656:	3ee50513          	addi	a0,a0,1006 # 7a40 <irq_handler+0x9e4>
    365a:	0fffd097          	auipc	ra,0xfffd
    365e:	f1e080e7          	jalr	-226(ra) # 10000578 <bk_printf>
    3662:	a809                	c.j	3674 <bspatch_core+0x82>
    3664:	4621                	c.li	a2,8
    3666:	00004597          	auipc	a1,0x4
    366a:	3fe58593          	addi	a1,a1,1022 # 7a64 <irq_handler+0xa08>
    366e:	00e8                	c.addi4spn	a0,sp,76
    3670:	8240                	exec.it	#96     !jal	ra,5d90 <memcmp>
    3672:	c519                	c.beqz	a0,3680 <bspatch_core+0x8e>
    3674:	8522                	c.mv	a0,s0
    3676:	59fd                	c.li	s3,-1
    3678:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    367a:	854e                	c.mv	a0,s3
    367c:	6171                	c.addi16sp	sp,336
    367e:	8c34                	exec.it	#31     !j	5d60 <__riscv_restore_12>
    3680:	004aa303          	lw	t1,4(s5)
    3684:	8004                	exec.it	#8     !lw	a3,408(sp)
    3686:	8c24                	exec.it	#30     !li	a2,32
    3688:	00ec                	c.addi4spn	a1,sp,76
    368a:	8556                	c.mv	a0,s5
    368c:	9302                	c.jalr	t1
    368e:	000557db          	beqc	a0,32,369c <bspatch_core+0xaa>
    3692:	00004517          	auipc	a0,0x4
    3696:	3de50513          	addi	a0,a0,990 # 7a70 <irq_handler+0xa14>
    369a:	b7c1                	c.j	365a <bspatch_core+0x68>
    369c:	4621                	c.li	a2,8
    369e:	00004597          	auipc	a1,0x4
    36a2:	3c658593          	addi	a1,a1,966 # 7a64 <irq_handler+0xa08>
    36a6:	00e8                	c.addi4spn	a0,sp,76
    36a8:	8240                	exec.it	#96     !jal	ra,5d90 <memcmp>
    36aa:	89aa                	c.mv	s3,a0
    36ac:	f561                	c.bnez	a0,3674 <bspatch_core+0x82>
    36ae:	08c8                	c.addi4spn	a0,sp,84
    36b0:	3db1                	c.jal	350c <offtin>
    36b2:	8baa                	c.mv	s7,a0
    36b4:	d82a                	c.swsp	a0,48(sp)
    36b6:	08e8                	c.addi4spn	a0,sp,92
    36b8:	3d91                	c.jal	350c <offtin>
    36ba:	da2a                	c.swsp	a0,52(sp)
    36bc:	c02a                	c.swsp	a0,0(sp)
    36be:	10c8                	c.addi4spn	a0,sp,100
    36c0:	35b1                	c.jal	350c <offtin>
    36c2:	4602                	c.lwsp	a2,0(sp)
    36c4:	86aa                	c.mv	a3,a0
    36c6:	de2a                	c.swsp	a0,60(sp)
    36c8:	85de                	c.mv	a1,s7
    36ca:	00004517          	auipc	a0,0x4
    36ce:	3ca50513          	addi	a0,a0,970 # 7a94 <irq_handler+0xa38>
    36d2:	0fffd097          	auipc	ra,0xfffd
    36d6:	ea6080e7          	jalr	-346(ra) # 10000578 <bk_printf>
    36da:	52f2                	c.lwsp	t0,60(sp)
    36dc:	01b28a63          	beq	t0,s11,36f0 <bspatch_core+0xfe>
    36e0:	00004517          	auipc	a0,0x4
    36e4:	3dc50513          	addi	a0,a0,988 # 7abc <irq_handler+0xa60>
    36e8:	0fffd097          	auipc	ra,0xfffd
    36ec:	e90080e7          	jalr	-368(ra) # 10000578 <bk_printf>
    36f0:	55c2                	c.lwsp	a1,48(sp)
    36f2:	40e1                	c.li	ra,24
    36f4:	0215d3b3          	divu	t2,a1,ra
    36f8:	00004517          	auipc	a0,0x4
    36fc:	3dc50513          	addi	a0,a0,988 # 7ad4 <irq_handler+0xa78>
    3700:	4b81                	c.li	s7,0
    3702:	08000d13          	li	s10,128
    3706:	3c03a5db          	bfoz	a1,t2,15,0
    370a:	02711623          	sh	t2,44(sp)
    370e:	0fffd097          	auipc	ra,0xfffd
    3712:	e6a080e7          	jalr	-406(ra) # 10000578 <bk_printf>
    3716:	8210                	exec.it	#65     !jal	ra,4216 <ty_adapter_get_free_heap_size>
    3718:	85aa                	c.mv	a1,a0
    371a:	00004517          	auipc	a0,0x4
    371e:	30250513          	addi	a0,a0,770 # 7a1c <irq_handler+0x9c0>
    3722:	0fffd097          	auipc	ra,0xfffd
    3726:	e56080e7          	jalr	-426(ra) # 10000578 <bk_printf>
    372a:	c482                	c.swsp	zero,72(sp)
    372c:	c082                	c.swsp	zero,64(sp)
    372e:	02c15503          	lhu	a0,44(sp)
    3732:	04abc863          	blt	s7,a0,3782 <bspatch_core+0x190>
    3736:	46c2                	c.lwsp	a3,16(sp)
    3738:	4298                	c.lw	a4,0(a3)
    373a:	0d6dec63          	bltu	s11,s6,3812 <bspatch_core+0x220>
    373e:	996e                	c.add	s2,s11
    3740:	4c32                	c.lwsp	s8,12(sp)
    3742:	0e0c1563          	bnez	s8,382c <bspatch_core+0x23a>
    3746:	8c26                	c.mv	s8,s1
    3748:	01b4e363          	bltu	s1,s11,374e <bspatch_core+0x15c>
    374c:	8670                	exec.it	#115     !remu	s8,s11,s1
    374e:	19412c83          	lw	s9,404(sp)
    3752:	4b81                	c.li	s7,0
    3754:	010cae03          	lw	t3,16(s9)
    3758:	4c81                	c.li	s9,0
    375a:	c472                	c.swsp	t3,8(sp)
    375c:	c06e                	c.swsp	s11,0(sp)
    375e:	c202                	c.swsp	zero,4(sp)
    3760:	02c15e83          	lhu	t4,44(sp)
    3764:	0ddcc963          	blt	s9,t4,3836 <bspatch_core+0x244>
    3768:	8522                	c.mv	a0,s0
    376a:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    376c:	8210                	exec.it	#65     !jal	ra,4216 <ty_adapter_get_free_heap_size>
    376e:	85aa                	c.mv	a1,a0
    3770:	00004517          	auipc	a0,0x4
    3774:	2ac50513          	addi	a0,a0,684 # 7a1c <irq_handler+0x9c0>
    3778:	0fffd097          	auipc	ra,0xfffd
    377c:	e00080e7          	jalr	-512(ra) # 10000578 <bk_printf>
    3780:	bded                	c.j	367a <bspatch_core+0x88>
    3782:	004aac03          	lw	s8,4(s5)
    3786:	8004                	exec.it	#8     !lw	a3,408(sp)
    3788:	4661                	c.li	a2,24
    378a:	00ec                	c.addi4spn	a1,sp,76
    378c:	8556                	c.mv	a0,s5
    378e:	9c02                	c.jalr	s8
    3790:	01855a5b          	beqc	a0,24,37a4 <bspatch_core+0x1b2>
    3794:	00004517          	auipc	a0,0x4
    3798:	35050513          	addi	a0,a0,848 # 7ae4 <irq_handler+0xa88>
    379c:	0fffd097          	auipc	ra,0xfffd
    37a0:	ddc080e7          	jalr	-548(ra) # 10000578 <bk_printf>
    37a4:	00e8                	c.addi4spn	a0,sp,76
    37a6:	339d                	c.jal	350c <offtin>
    37a8:	d02a                	c.swsp	a0,32(sp)
    37aa:	08c8                	c.addi4spn	a0,sp,84
    37ac:	3385                	c.jal	350c <offtin>
    37ae:	d22a                	c.swsp	a0,36(sp)
    37b0:	8caa                	c.mv	s9,a0
    37b2:	08e8                	c.addi4spn	a0,sp,92
    37b4:	3ba1                	c.jal	350c <offtin>
    37b6:	d42a                	c.swsp	a0,40(sp)
    37b8:	01904463          	bgtz	s9,37c0 <bspatch_core+0x1ce>
    37bc:	0b85                	c.addi	s7,1
    37be:	bf85                	c.j	372e <bspatch_core+0x13c>
    37c0:	865e                	c.mv	a2,s7
    37c2:	85e6                	c.mv	a1,s9
    37c4:	00004517          	auipc	a0,0x4
    37c8:	34450513          	addi	a0,a0,836 # 7b08 <irq_handler+0xaac>
    37cc:	4c01                	c.li	s8,0
    37ce:	0fffd097          	auipc	ra,0xfffd
    37d2:	daa080e7          	jalr	-598(ra) # 10000578 <bk_printf>
    37d6:	5392                	c.lwsp	t2,36(sp)
    37d8:	fe7c52e3          	bge	s8,t2,37bc <bspatch_core+0x1ca>
    37dc:	41838cb3          	sub	s9,t2,s8
    37e0:	019d5463          	bge	s10,s9,37e8 <bspatch_core+0x1f6>
    37e4:	08000c93          	li	s9,128
    37e8:	004aa703          	lw	a4,4(s5)
    37ec:	8004                	exec.it	#8     !lw	a3,408(sp)
    37ee:	8666                	c.mv	a2,s9
    37f0:	00ec                	c.addi4spn	a1,sp,76
    37f2:	8556                	c.mv	a0,s5
    37f4:	9702                	c.jalr	a4
    37f6:	862a                	c.mv	a2,a0
    37f8:	00ac8b63          	beq	s9,a0,380e <bspatch_core+0x21c>
    37fc:	85e6                	c.mv	a1,s9
    37fe:	00004517          	auipc	a0,0x4
    3802:	33250513          	addi	a0,a0,818 # 7b30 <irq_handler+0xad4>
    3806:	0fffd097          	auipc	ra,0xfffd
    380a:	d72080e7          	jalr	-654(ra) # 10000578 <bk_printf>
    380e:	9c66                	c.add	s8,s9
    3810:	b7d9                	c.j	37d6 <bspatch_core+0x1e4>
    3812:	995a                	c.add	s2,s6
    3814:	02edfb33          	remu	s6,s11,a4
    3818:	995a                	c.add	s2,s6
    381a:	02e97833          	remu	a6,s2,a4
    381e:	f20801e3          	beqz	a6,3740 <bspatch_core+0x14e>
    3822:	012708b3          	add	a7,a4,s2
    3826:	41088933          	sub	s2,a7,a6
    382a:	bf19                	c.j	3740 <bspatch_core+0x14e>
    382c:	8670                	exec.it	#115     !remu	s8,s11,s1
    382e:	f20c10e3          	bnez	s8,374e <bspatch_core+0x15c>
    3832:	8c26                	c.mv	s8,s1
    3834:	bf29                	c.j	374e <bspatch_core+0x15c>
    3836:	004a2f03          	lw	t5,4(s4)
    383a:	8004                	exec.it	#8     !lw	a3,408(sp)
    383c:	4661                	c.li	a2,24
    383e:	00ec                	c.addi4spn	a1,sp,76
    3840:	8552                	c.mv	a0,s4
    3842:	9f02                	c.jalr	t5
    3844:	01855a5b          	beqc	a0,24,3858 <bspatch_core+0x266>
    3848:	00004517          	auipc	a0,0x4
    384c:	32850513          	addi	a0,a0,808 # 7b70 <irq_handler+0xb14>
    3850:	0fffd097          	auipc	ra,0xfffd
    3854:	d28080e7          	jalr	-728(ra) # 10000578 <bk_printf>
    3858:	00e8                	c.addi4spn	a0,sp,76
    385a:	394d                	c.jal	350c <offtin>
    385c:	d02a                	c.swsp	a0,32(sp)
    385e:	ce2a                	c.swsp	a0,28(sp)
    3860:	08c8                	c.addi4spn	a0,sp,84
    3862:	316d                	c.jal	350c <offtin>
    3864:	d22a                	c.swsp	a0,36(sp)
    3866:	cc2a                	c.swsp	a0,24(sp)
    3868:	08e8                	c.addi4spn	a0,sp,92
    386a:	314d                	c.jal	350c <offtin>
    386c:	47e2                	c.lwsp	a5,24(sp)
    386e:	4772                	c.lwsp	a4,28(sp)
    3870:	417c0333          	sub	t1,s8,s7
    3874:	00f70633          	add	a2,a4,a5
    3878:	d42a                	c.swsp	a0,40(sp)
    387a:	12c37063          	bgeu	t1,a2,399a <bspatch_core+0x3a8>
    387e:	4802                	c.lwsp	a6,0(sp)
    3880:	86de                	c.mv	a3,s7
    3882:	8662                	c.mv	a2,s8
    3884:	85e6                	c.mv	a1,s9
    3886:	00004517          	auipc	a0,0x4
    388a:	30650513          	addi	a0,a0,774 # 7b8c <irq_handler+0xb30>
    388e:	0fffd097          	auipc	ra,0xfffd
    3892:	cea080e7          	jalr	-790(ra) # 10000578 <bk_printf>
    3896:	8522                	c.mv	a0,s0
    3898:	59fd                	c.li	s3,-1
    389a:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    389c:	bbf9                	c.j	367a <bspatch_core+0x88>
    389e:	41a28b33          	sub	s6,t0,s10
    38a2:	08000693          	li	a3,128
    38a6:	0166d463          	bge	a3,s6,38ae <bspatch_core+0x2bc>
    38aa:	08000b13          	li	s6,128
    38ae:	004aa803          	lw	a6,4(s5)
    38b2:	8004                	exec.it	#8     !lw	a3,408(sp)
    38b4:	865a                	c.mv	a2,s6
    38b6:	00ec                	c.addi4spn	a1,sp,76
    38b8:	8556                	c.mv	a0,s5
    38ba:	9802                	c.jalr	a6
    38bc:	862a                	c.mv	a2,a0
    38be:	00ab0b63          	beq	s6,a0,38d4 <bspatch_core+0x2e2>
    38c2:	85da                	c.mv	a1,s6
    38c4:	00004517          	auipc	a0,0x4
    38c8:	30050513          	addi	a0,a0,768 # 7bc4 <irq_handler+0xb68>
    38cc:	0fffd097          	auipc	ra,0xfffd
    38d0:	cac080e7          	jalr	-852(ra) # 10000578 <bk_printf>
    38d4:	48a2                	c.lwsp	a7,8(sp)
    38d6:	051cea63          	bltu	s9,a7,392a <bspatch_core+0x338>
    38da:	4e26                	c.lwsp	t3,72(sp)
    38dc:	4ed2                	c.lwsp	t4,20(sp)
    38de:	8004                	exec.it	#8     !lw	a3,408(sp)
    38e0:	01ce8f33          	add	t5,t4,t3
    38e4:	865a                	c.mv	a2,s6
    38e6:	01ec                	c.addi4spn	a1,sp,204
    38e8:	01af0533          	add	a0,t5,s10
    38ec:	8c14                	exec.it	#15     !jal	ra,41d4 <ty_adapt_flash_read>
    38ee:	87aa                	c.mv	a5,a0
    38f0:	c511                	c.beqz	a0,38fc <bspatch_core+0x30a>
    38f2:	c02a                	c.swsp	a0,0(sp)
    38f4:	8522                	c.mv	a0,s0
    38f6:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    38f8:	4982                	c.lwsp	s3,0(sp)
    38fa:	b341                	c.j	367a <bspatch_core+0x88>
    38fc:	04c10093          	addi	ra,sp,76
    3900:	0cc10f93          	addi	t6,sp,204
    3904:	00ff8333          	add	t1,t6,a5
    3908:	00008283          	lb	t0,0(ra)
    390c:	00030603          	lb	a2,0(t1)
    3910:	0785                	c.addi	a5,1
    3912:	005605b3          	add	a1,a2,t0
    3916:	00b08023          	sb	a1,0(ra)
    391a:	0085                	c.addi	ra,1
    391c:	fefb12e3          	bne	s6,a5,3900 <bspatch_core+0x30e>
    3920:	865a                	c.mv	a2,s6
    3922:	00ec                	c.addi4spn	a1,sp,76
    3924:	01740533          	add	a0,s0,s7
    3928:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    392a:	9bda                	c.add	s7,s6
    392c:	9d5a                	c.add	s10,s6
    392e:	5282                	c.lwsp	t0,32(sp)
    3930:	f65d47e3          	blt	s10,t0,389e <bspatch_core+0x2ac>
    3934:	4d06                	c.lwsp	s10,64(sp)
    3936:	40a6                	c.lwsp	ra,72(sp)
    3938:	005d05b3          	add	a1,s10,t0
    393c:	005083b3          	add	t2,ra,t0
    3940:	c0ae                	c.swsp	a1,64(sp)
    3942:	5592                	c.lwsp	a1,36(sp)
    3944:	c49e                	c.swsp	t2,72(sp)
    3946:	00b05f63          	blez	a1,3964 <bspatch_core+0x372>
    394a:	8666                	c.mv	a2,s9
    394c:	00004517          	auipc	a0,0x4
    3950:	2b050513          	addi	a0,a0,688 # 7bfc <irq_handler+0xba0>
    3954:	4b01                	c.li	s6,0
    3956:	0fffd097          	auipc	ra,0xfffd
    395a:	c22080e7          	jalr	-990(ra) # 10000578 <bk_printf>
    395e:	5512                	c.lwsp	a0,36(sp)
    3960:	02ab4f63          	blt	s6,a0,399e <bspatch_core+0x3ac>
    3964:	4686                	c.lwsp	a3,64(sp)
    3966:	48a6                	c.lwsp	a7,72(sp)
    3968:	5712                	c.lwsp	a4,36(sp)
    396a:	5e22                	c.lwsp	t3,40(sp)
    396c:	00e68833          	add	a6,a3,a4
    3970:	01c88eb3          	add	t4,a7,t3
    3974:	c0c2                	c.swsp	a6,64(sp)
    3976:	c4f6                	c.swsp	t4,72(sp)
    3978:	001c8d13          	addi	s10,s9,1
    397c:	077c7763          	bgeu	s8,s7,39ea <bspatch_core+0x3f8>
    3980:	86e6                	c.mv	a3,s9
    3982:	8662                	c.mv	a2,s8
    3984:	85de                	c.mv	a1,s7
    3986:	00004517          	auipc	a0,0x4
    398a:	2da50513          	addi	a0,a0,730 # 7c60 <irq_handler+0xc04>
    398e:	0fffd097          	auipc	ra,0xfffd
    3992:	bea080e7          	jalr	-1046(ra) # 10000578 <bk_printf>
    3996:	8cea                	c.mv	s9,s10
    3998:	b3e1                	c.j	3760 <bspatch_core+0x16e>
    399a:	4d01                	c.li	s10,0
    399c:	bf49                	c.j	392e <bspatch_core+0x33c>
    399e:	41650d33          	sub	s10,a0,s6
    39a2:	08000393          	li	t2,128
    39a6:	01a3d463          	bge	t2,s10,39ae <bspatch_core+0x3bc>
    39aa:	08000d13          	li	s10,128
    39ae:	004a2703          	lw	a4,4(s4)
    39b2:	8004                	exec.it	#8     !lw	a3,408(sp)
    39b4:	866a                	c.mv	a2,s10
    39b6:	00ec                	c.addi4spn	a1,sp,76
    39b8:	8552                	c.mv	a0,s4
    39ba:	9702                	c.jalr	a4
    39bc:	862a                	c.mv	a2,a0
    39be:	00ad0b63          	beq	s10,a0,39d4 <bspatch_core+0x3e2>
    39c2:	85ea                	c.mv	a1,s10
    39c4:	00004517          	auipc	a0,0x4
    39c8:	26450513          	addi	a0,a0,612 # 7c28 <irq_handler+0xbcc>
    39cc:	0fffd097          	auipc	ra,0xfffd
    39d0:	bac080e7          	jalr	-1108(ra) # 10000578 <bk_printf>
    39d4:	4522                	c.lwsp	a0,8(sp)
    39d6:	00ace763          	bltu	s9,a0,39e4 <bspatch_core+0x3f2>
    39da:	866a                	c.mv	a2,s10
    39dc:	00ec                	c.addi4spn	a1,sp,76
    39de:	01740533          	add	a0,s0,s7
    39e2:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    39e4:	9bea                	c.add	s7,s10
    39e6:	9b6a                	c.add	s6,s10
    39e8:	bf9d                	c.j	395e <bspatch_core+0x36c>
    39ea:	fb8b96e3          	bne	s7,s8,3996 <bspatch_core+0x3a4>
    39ee:	19412f03          	lw	t5,404(sp)
    39f2:	47b2                	c.lwsp	a5,12(sp)
    39f4:	048f2f83          	lw	t6,72(t5)
    39f8:	ebbd                	c.bnez	a5,3a6e <bspatch_core+0x47c>
    39fa:	4692                	c.lwsp	a3,4(sp)
    39fc:	01f685b3          	add	a1,a3,t6
    3a00:	02c15703          	lhu	a4,44(sp)
    3a04:	19412883          	lw	a7,404(sp)
    3a08:	41a70833          	sub	a6,a4,s10
    3a0c:	0198ce03          	lbu	t3,25(a7)
    3a10:	00183713          	seqz	a4,a6
    3a14:	4f81                	c.li	t6,0
    3a16:	062e6a5b          	bnec	t3,2,3a8a <bspatch_core+0x498>
    3a1a:	4f22                	c.lwsp	t5,8(sp)
    3a1c:	01af1b63          	bne	t5,s10,3a32 <bspatch_core+0x440>
    3a20:	19812783          	lw	a5,408(sp)
    3a24:	19412503          	lw	a0,404(sp)
    3a28:	86de                	c.mv	a3,s7
    3a2a:	8622                	c.mv	a2,s0
    3a2c:	d3eff0ef          	jal	ra,2f6a <_bkup_2_frmware>
    3a30:	8faa                	c.mv	t6,a0
    3a32:	19412783          	lw	a5,404(sp)
    3a36:	86e2                	c.mv	a3,s8
    3a38:	0197c583          	lbu	a1,25(a5)
    3a3c:	8666                	c.mv	a2,s9
    3a3e:	00004517          	auipc	a0,0x4
    3a42:	25250513          	addi	a0,a0,594 # 7c90 <irq_handler+0xc34>
    3a46:	cc7e                	c.swsp	t6,24(sp)
    3a48:	0fffd097          	auipc	ra,0xfffd
    3a4c:	b30080e7          	jalr	-1232(ra) # 10000578 <bk_printf>
    3a50:	4362                	c.lwsp	t1,24(sp)
    3a52:	04030d63          	beqz	t1,3aac <bspatch_core+0x4ba>
    3a56:	8522                	c.mv	a0,s0
    3a58:	c01a                	c.swsp	t1,0(sp)
    3a5a:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    3a5c:	00004517          	auipc	a0,0x4
    3a60:	27c50513          	addi	a0,a0,636 # 7cd8 <irq_handler+0xc7c>
    3a64:	0fffd097          	auipc	ra,0xfffd
    3a68:	b14080e7          	jalr	-1260(ra) # 10000578 <bk_printf>
    3a6c:	b571                	c.j	38f8 <bspatch_core+0x306>
    3a6e:	4612                	c.lwsp	a2,4(sp)
    3a70:	40c2                	c.lwsp	ra,16(sp)
    3a72:	01f90333          	add	t1,s2,t6
    3a76:	0000a383          	lw	t2,0(ra)
    3a7a:	40c302b3          	sub	t0,t1,a2
    3a7e:	417285b3          	sub	a1,t0,s7
    3a82:	0275f533          	remu	a0,a1,t2
    3a86:	8d89                	c.sub	a1,a0
    3a88:	bfa5                	c.j	3a00 <bspatch_core+0x40e>
    3a8a:	ba1e645b          	bnec	t3,1,3a32 <bspatch_core+0x440>
    3a8e:	4ea2                	c.lwsp	t4,8(sp)
    3a90:	fbdce1e3          	bltu	s9,t4,3a32 <bspatch_core+0x440>
    3a94:	19812803          	lw	a6,408(sp)
    3a98:	19412503          	lw	a0,404(sp)
    3a9c:	87ba                	c.mv	a5,a4
    3a9e:	862e                	c.mv	a2,a1
    3aa0:	875e                	c.mv	a4,s7
    3aa2:	86a2                	c.mv	a3,s0
    3aa4:	85ea                	c.mv	a1,s10
    3aa6:	d1aff0ef          	jal	ra,2fc0 <_buff_2_bkup_2_frmware>
    3aaa:	b759                	c.j	3a30 <bspatch_core+0x43e>
    3aac:	4c12                	c.lwsp	s8,4(sp)
    3aae:	4602                	c.lwsp	a2,0(sp)
    3ab0:	017c0cb3          	add	s9,s8,s7
    3ab4:	42b2                	c.lwsp	t0,12(sp)
    3ab6:	41760bb3          	sub	s7,a2,s7
    3aba:	c266                	c.swsp	s9,4(sp)
    3abc:	c05e                	c.swsp	s7,0(sp)
    3abe:	00029963          	bnez	t0,3ad0 <bspatch_core+0x4de>
    3ac2:	8c26                	c.mv	s8,s1
    3ac4:	0174e363          	bltu	s1,s7,3aca <bspatch_core+0x4d8>
    3ac8:	8670                	exec.it	#115     !remu	s8,s11,s1
    3aca:	4b81                	c.li	s7,0
    3acc:	c482                	c.swsp	zero,72(sp)
    3ace:	b5e1                	c.j	3996 <bspatch_core+0x3a4>
    3ad0:	4582                	c.lwsp	a1,0(sp)
    3ad2:	0295f0b3          	remu	ra,a1,s1
    3ad6:	fe0099e3          	bnez	ra,3ac8 <bspatch_core+0x4d6>
    3ada:	8c26                	c.mv	s8,s1
    3adc:	b7fd                	c.j	3aca <bspatch_core+0x4d8>

00003ade <xz_patch_read>:
    3ade:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    3ae0:	842a                	c.mv	s0,a0
    3ae2:	441c                	c.lw	a5,8(s0)
    3ae4:	4548                	c.lw	a0,12(a0)
    3ae6:	84b2                	c.mv	s1,a2
    3ae8:	953e                	c.add	a0,a5
    3aea:	8c14                	exec.it	#15     !jal	ra,41d4 <ty_adapt_flash_read>
    3aec:	e519                	c.bnez	a0,3afa <xz_patch_read+0x1c>
    3aee:	8e30                	exec.it	#87     !lw	ra,12(s0)
    3af0:	8526                	c.mv	a0,s1
    3af2:	009082b3          	add	t0,ra,s1
    3af6:	00542623          	sw	t0,12(s0)
    3afa:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>

00003afc <xz_decode_read>:
    3afc:	4108                	c.lw	a0,0(a0)
    3afe:	7900006f          	j	428e <xz_read>

00003b02 <flash_crc32_cal>:
    3b02:	22a022ef          	jal	t0,5d2c <__riscv_save_10>
    3b06:	8baa                	c.mv	s7,a0
    3b08:	8aae                	c.mv	s5,a1
    3b0a:	8c32                	c.mv	s8,a2
    3b0c:	2db5                	c.jal	4188 <ty_adapt_flash_get_cfg>
    3b0e:	00052b03          	lw	s6,0(a0)
    3b12:	855a                	c.mv	a0,s6
    3b14:	8c30                	exec.it	#23     !jal	ra,41ec <ty_adapt_malloc>
    3b16:	ed09                	c.bnez	a0,3b30 <flash_crc32_cal+0x2e>
    3b18:	00004517          	auipc	a0,0x4
    3b1c:	1dc50513          	addi	a0,a0,476 # 7cf4 <irq_handler+0xc98>
    3b20:	4401                	c.li	s0,0
    3b22:	0fffd097          	auipc	ra,0xfffd
    3b26:	a56080e7          	jalr	-1450(ra) # 10000578 <bk_printf>
    3b2a:	8522                	c.mv	a0,s0
    3b2c:	2380206f          	j	5d64 <__riscv_restore_10>
    3b30:	84aa                	c.mv	s1,a0
    3b32:	4981                	c.li	s3,0
    3b34:	6e4000ef          	jal	ra,4218 <hash_crc32i_init>
    3b38:	892a                	c.mv	s2,a0
    3b3a:	413a8a33          	sub	s4,s5,s3
    3b3e:	014b7363          	bgeu	s6,s4,3b44 <flash_crc32_cal+0x42>
    3b42:	8a5a                	c.mv	s4,s6
    3b44:	86e2                	c.mv	a3,s8
    3b46:	8652                	c.mv	a2,s4
    3b48:	85a6                	c.mv	a1,s1
    3b4a:	01798533          	add	a0,s3,s7
    3b4e:	2559                	c.jal	41d4 <ty_adapt_flash_read>
    3b50:	842a                	c.mv	s0,a0
    3b52:	c501                	c.beqz	a0,3b5a <flash_crc32_cal+0x58>
    3b54:	8526                	c.mv	a0,s1
    3b56:	8400                	exec.it	#2     !jal	ra,4214 <ty_adapt_free>
    3b58:	bfc9                	c.j	3b2a <flash_crc32_cal+0x28>
    3b5a:	854a                	c.mv	a0,s2
    3b5c:	8652                	c.mv	a2,s4
    3b5e:	85a6                	c.mv	a1,s1
    3b60:	99d2                	c.add	s3,s4
    3b62:	6ba000ef          	jal	ra,421c <hash_crc32i_update>
    3b66:	892a                	c.mv	s2,a0
    3b68:	fd59e9e3          	bltu	s3,s5,3b3a <flash_crc32_cal+0x38>
    3b6c:	6da000ef          	jal	ra,4246 <hash_crc32i_finish>
    3b70:	842a                	c.mv	s0,a0
    3b72:	b7cd                	c.j	3b54 <flash_crc32_cal+0x52>

00003b74 <diff2ya_run>:
    3b74:	1c6022ef          	jal	t0,5d3a <__riscv_save_4>
    3b78:	d4010113          	addi	sp,sp,-704
    3b7c:	89aa                	c.mv	s3,a0
    3b7e:	842e                	c.mv	s0,a1
    3b80:	8a32                	c.mv	s4,a2
    3b82:	2519                	c.jal	4188 <ty_adapt_flash_get_cfg>
    3b84:	8aaa                	c.mv	s5,a0
    3b86:	00004517          	auipc	a0,0x4
    3b8a:	17e50513          	addi	a0,a0,382 # 7d04 <irq_handler+0xca8>
    3b8e:	0fffd097          	auipc	ra,0xfffd
    3b92:	9ea080e7          	jalr	-1558(ra) # 10000578 <bk_printf>
    3b96:	0109a783          	lw	a5,16(s3)
    3b9a:	e79d                	c.bnez	a5,3bc8 <diff2ya_run+0x54>
    3b9c:	480c                	c.lw	a1,16(s0)
    3b9e:	0489a503          	lw	a0,72(s3)
    3ba2:	8652                	c.mv	a2,s4
    3ba4:	3fb9                	c.jal	3b02 <flash_crc32_cal>
    3ba6:	4410                	c.lw	a2,8(s0)
    3ba8:	85aa                	c.mv	a1,a0
    3baa:	00a60f63          	beq	a2,a0,3bc8 <diff2ya_run+0x54>
    3bae:	00004517          	auipc	a0,0x4
    3bb2:	16650513          	addi	a0,a0,358 # 7d14 <irq_handler+0xcb8>
    3bb6:	0fffd097          	auipc	ra,0xfffd
    3bba:	9c2080e7          	jalr	-1598(ra) # 10000578 <bk_printf>
    3bbe:	4509                	c.li	a0,2
    3bc0:	2c010113          	addi	sp,sp,704
    3bc4:	1aa0206f          	j	5d6e <__riscv_restore_4>
    3bc8:	00004517          	auipc	a0,0x4
    3bcc:	17850513          	addi	a0,a0,376 # 7d40 <irq_handler+0xce4>
    3bd0:	0fffd097          	auipc	ra,0xfffd
    3bd4:	9a8080e7          	jalr	-1624(ra) # 10000578 <bk_printf>
    3bd8:	01c42283          	lw	t0,28(s0)
    3bdc:	4c04                	c.lw	s1,24(s0)
    3bde:	04c9a383          	lw	t2,76(s3)
    3be2:	00548333          	add	t1,s1,t0
    3be6:	007304b3          	add	s1,t1,t2
    3bea:	5010                	c.lw	a2,32(s0)
    3bec:	85a6                	c.mv	a1,s1
    3bee:	00004517          	auipc	a0,0x4
    3bf2:	16650513          	addi	a0,a0,358 # 7d54 <irq_handler+0xcf8>
    3bf6:	0fffd097          	auipc	ra,0xfffd
    3bfa:	982080e7          	jalr	-1662(ra) # 10000578 <bk_printf>
    3bfe:	500c                	c.lw	a1,32(s0)
    3c00:	8652                	c.mv	a2,s4
    3c02:	8526                	c.mv	a0,s1
    3c04:	3dfd                	c.jal	3b02 <flash_crc32_cal>
    3c06:	ce2a                	c.swsp	a0,28(sp)
    3c08:	00004517          	auipc	a0,0x4
    3c0c:	18450513          	addi	a0,a0,388 # 7d8c <irq_handler+0xd30>
    3c10:	0fffd097          	auipc	ra,0xfffd
    3c14:	968080e7          	jalr	-1688(ra) # 10000578 <bk_printf>
    3c18:	504c                	c.lw	a1,36(s0)
    3c1a:	4672                	c.lwsp	a2,28(sp)
    3c1c:	00c58c63          	beq	a1,a2,3c34 <diff2ya_run+0xc0>
    3c20:	00004517          	auipc	a0,0x4
    3c24:	17850513          	addi	a0,a0,376 # 7d98 <irq_handler+0xd3c>
    3c28:	0fffd097          	auipc	ra,0xfffd
    3c2c:	950080e7          	jalr	-1712(ra) # 10000578 <bk_printf>
    3c30:	4511                	c.li	a0,4
    3c32:	b779                	c.j	3bc0 <diff2ya_run+0x4c>
    3c34:	23cd                	c.jal	4216 <ty_adapter_get_free_heap_size>
    3c36:	85aa                	c.mv	a1,a0
    3c38:	00004517          	auipc	a0,0x4
    3c3c:	19450513          	addi	a0,a0,404 # 7dcc <irq_handler+0xd70>
    3c40:	0fffd097          	auipc	ra,0xfffd
    3c44:	938080e7          	jalr	-1736(ra) # 10000578 <bk_printf>
    3c48:	0199c583          	lbu	a1,25(s3)
    3c4c:	00004517          	auipc	a0,0x4
    3c50:	1a450513          	addi	a0,a0,420 # 7df0 <irq_handler+0xd94>
    3c54:	0fffd097          	auipc	ra,0xfffd
    3c58:	924080e7          	jalr	-1756(ra) # 10000578 <bk_printf>
    3c5c:	0199c503          	lbu	a0,25(s3)
    3c60:	4709                	c.li	a4,2
    3c62:	06a76263          	bltu	a4,a0,3cc6 <diff2ya_run+0x152>
    3c66:	e569                	c.bnez	a0,3d30 <diff2ya_run+0x1bc>
    3c68:	4901                	c.li	s2,0
    3c6a:	02c42a83          	lw	s5,44(s0)
    3c6e:	4481                	c.li	s1,0
    3c70:	484c                	c.lw	a1,20(s0)
    3c72:	0489a503          	lw	a0,72(s3)
    3c76:	16b4e763          	bltu	s1,a1,3de4 <diff2ya_run+0x270>
    3c7a:	8652                	c.mv	a2,s4
    3c7c:	3559                	c.jal	3b02 <flash_crc32_cal>
    3c7e:	5410                	c.lw	a2,40(s0)
    3c80:	03044583          	lbu	a1,48(s0)
    3c84:	86ca                	c.mv	a3,s2
    3c86:	84aa                	c.mv	s1,a0
    3c88:	00004517          	auipc	a0,0x4
    3c8c:	1d850513          	addi	a0,a0,472 # 7e60 <irq_handler+0xe04>
    3c90:	0fffd097          	auipc	ra,0xfffd
    3c94:	8e8080e7          	jalr	-1816(ra) # 10000578 <bk_printf>
    3c98:	484c                	c.lw	a1,20(s0)
    3c9a:	4810                	c.lw	a2,16(s0)
    3c9c:	00004517          	auipc	a0,0x4
    3ca0:	20050513          	addi	a0,a0,512 # 7e9c <irq_handler+0xe40>
    3ca4:	0fffd097          	auipc	ra,0xfffd
    3ca8:	8d4080e7          	jalr	-1836(ra) # 10000578 <bk_printf>
    3cac:	444c                	c.lw	a1,12(s0)
    3cae:	14958763          	beq	a1,s1,3dfc <diff2ya_run+0x288>
    3cb2:	00004517          	auipc	a0,0x4
    3cb6:	22650513          	addi	a0,a0,550 # 7ed8 <irq_handler+0xe7c>
    3cba:	0fffd097          	auipc	ra,0xfffd
    3cbe:	8be080e7          	jalr	-1858(ra) # 10000578 <bk_printf>
    3cc2:	450d                	c.li	a0,3
    3cc4:	bdf5                	c.j	3bc0 <diff2ya_run+0x4c>
    3cc6:	ba35615b          	bnec	a0,3,3c68 <diff2ya_run+0xf4>
    3cca:	5418                	c.lw	a4,40(s0)
    3ccc:	03044f83          	lbu	t6,48(s0)
    3cd0:	484c                	c.lw	a1,20(s0)
    3cd2:	893a                	c.mv	s2,a4
    3cd4:	020f8c63          	beqz	t6,3d0c <diff2ya_run+0x198>
    3cd8:	01042283          	lw	t0,16(s0)
    3cdc:	0255f863          	bgeu	a1,t0,3d0c <diff2ya_run+0x198>
    3ce0:	000aa783          	lw	a5,0(s5)
    3ce4:	00e28333          	add	t1,t0,a4
    3ce8:	02f5f933          	remu	s2,a1,a5
    3cec:	01230533          	add	a0,t1,s2
    3cf0:	02f573b3          	remu	t2,a0,a5
    3cf4:	00038663          	beqz	t2,3d00 <diff2ya_run+0x18c>
    3cf8:	00a78633          	add	a2,a5,a0
    3cfc:	40760533          	sub	a0,a2,t2
    3d00:	40b500b3          	sub	ra,a0,a1
    3d04:	02f0f6b3          	remu	a3,ra,a5
    3d08:	40d08933          	sub	s2,ra,a3
    3d0c:	00e96833          	or	a6,s2,a4
    3d10:	f4080de3          	beqz	a6,3c6a <diff2ya_run+0xf6>
    3d14:	5454                	c.lw	a3,44(s0)
    3d16:	87d2                	c.mv	a5,s4
    3d18:	864a                	c.mv	a2,s2
    3d1a:	854e                	c.mv	a0,s3
    3d1c:	8b0ff0ef          	jal	ra,2dcc <_flit_data_step>
    3d20:	84aa                	c.mv	s1,a0
    3d22:	d521                	c.beqz	a0,3c6a <diff2ya_run+0xf6>
    3d24:	85aa                	c.mv	a1,a0
    3d26:	00004517          	auipc	a0,0x4
    3d2a:	11a50513          	addi	a0,a0,282 # 7e40 <irq_handler+0xde4>
    3d2e:	a06d                	c.j	3dd8 <diff2ya_run+0x264>
    3d30:	4c4c                	c.lw	a1,28(s0)
    3d32:	01842083          	lw	ra,24(s0)
    3d36:	04c9a803          	lw	a6,76(s3)
    3d3a:	00b086b3          	add	a3,ra,a1
    3d3e:	02042e03          	lw	t3,32(s0)
    3d42:	010688b3          	add	a7,a3,a6
    3d46:	00000e97          	auipc	t4,0x0
    3d4a:	d98e8e93          	addi	t4,t4,-616 # 3ade <xz_patch_read>
    3d4e:	0884                	c.addi4spn	s1,sp,80
    3d50:	c2c6                	c.swsp	a7,68(sp)
    3d52:	c482                	c.swsp	zero,72(sp)
    3d54:	c0f2                	c.swsp	t3,64(sp)
    3d56:	c6f6                	c.swsp	t4,76(sp)
    3d58:	23d1                	c.jal	431c <xz_crc32_init>
    3d5a:	186c                	c.addi4spn	a1,sp,60
    3d5c:	8526                	c.mv	a0,s1
    3d5e:	29fd                	c.jal	425c <xz_init>
    3d60:	e919                	c.bnez	a0,3d76 <diff2ya_run+0x202>
    3d62:	00004517          	auipc	a0,0x4
    3d66:	0a650513          	addi	a0,a0,166 # 7e08 <irq_handler+0xdac>
    3d6a:	0fffd097          	auipc	ra,0xfffd
    3d6e:	80e080e7          	jalr	-2034(ra) # 10000578 <bk_printf>
    3d72:	557d                	c.li	a0,-1
    3d74:	b5b1                	c.j	3bc0 <diff2ya_run+0x4c>
    3d76:	18810913          	addi	s2,sp,392
    3d7a:	d626                	c.swsp	s1,44(sp)
    3d7c:	186c                	c.addi4spn	a1,sp,60
    3d7e:	00000497          	auipc	s1,0x0
    3d82:	d7e48493          	addi	s1,s1,-642 # 3afc <xz_decode_read>
    3d86:	854a                	c.mv	a0,s2
    3d88:	d826                	c.swsp	s1,48(sp)
    3d8a:	29c9                	c.jal	425c <xz_init>
    3d8c:	e911                	c.bnez	a0,3da0 <diff2ya_run+0x22c>
    3d8e:	00004517          	auipc	a0,0x4
    3d92:	08a50513          	addi	a0,a0,138 # 7e18 <irq_handler+0xdbc>
    3d96:	0fffc097          	auipc	ra,0xfffc
    3d9a:	7e2080e7          	jalr	2018(ra) # 10000578 <bk_printf>
    3d9e:	a001                	c.j	3d9e <diff2ya_run+0x22a>
    3da0:	0489a603          	lw	a2,72(s3)
    3da4:	8a54                	exec.it	#109     !lw	a6,44(s0)
    3da6:	541c                	c.lw	a5,40(s0)
    3da8:	03044703          	lbu	a4,48(s0)
    3dac:	4814                	c.lw	a3,16(s0)
    3dae:	484c                	c.lw	a1,20(s0)
    3db0:	03410f13          	addi	t5,sp,52
    3db4:	02c10893          	addi	a7,sp,44
    3db8:	8532                	c.mv	a0,a2
    3dba:	dc26                	c.swsp	s1,56(sp)
    3dbc:	da4a                	c.swsp	s2,52(sp)
    3dbe:	c452                	c.swsp	s4,8(sp)
    3dc0:	c24e                	c.swsp	s3,4(sp)
    3dc2:	c07a                	c.swsp	t5,0(sp)
    3dc4:	82fff0ef          	jal	ra,35f2 <bspatch_core>
    3dc8:	84aa                	c.mv	s1,a0
    3dca:	f00500e3          	beqz	a0,3cca <diff2ya_run+0x156>
    3dce:	85aa                	c.mv	a1,a0
    3dd0:	00004517          	auipc	a0,0x4
    3dd4:	05850513          	addi	a0,a0,88 # 7e28 <irq_handler+0xdcc>
    3dd8:	0fffc097          	auipc	ra,0xfffc
    3ddc:	7a0080e7          	jalr	1952(ra) # 10000578 <bk_printf>
    3de0:	8526                	c.mv	a0,s1
    3de2:	bbf9                	c.j	3bc0 <diff2ya_run+0x4c>
    3de4:	01548733          	add	a4,s1,s5
    3de8:	00e5f463          	bgeu	a1,a4,3df0 <diff2ya_run+0x27c>
    3dec:	40958ab3          	sub	s5,a1,s1
    3df0:	9526                	c.add	a0,s1
    3df2:	8652                	c.mv	a2,s4
    3df4:	85d6                	c.mv	a1,s5
    3df6:	94d6                	c.add	s1,s5
    3df8:	3329                	c.jal	3b02 <flash_crc32_cal>
    3dfa:	bd9d                	c.j	3c70 <diff2ya_run+0xfc>
    3dfc:	8652                	c.mv	a2,s4
    3dfe:	04098593          	addi	a1,s3,64
    3e02:	854e                	c.mv	a0,s3
    3e04:	f61fe0ef          	jal	ra,2d64 <bsdiff_mag_clear>
    3e08:	c919                	c.beqz	a0,3e1e <diff2ya_run+0x2aa>
    3e0a:	00004517          	auipc	a0,0x4
    3e0e:	0ee50513          	addi	a0,a0,238 # 7ef8 <irq_handler+0xe9c>
    3e12:	0fffc097          	auipc	ra,0xfffc
    3e16:	766080e7          	jalr	1894(ra) # 10000578 <bk_printf>
    3e1a:	4515                	c.li	a0,5
    3e1c:	b355                	c.j	3bc0 <diff2ya_run+0x4c>
    3e1e:	00004517          	auipc	a0,0x4
    3e22:	0f250513          	addi	a0,a0,242 # 7f10 <irq_handler+0xeb4>
    3e26:	0fffc097          	auipc	ra,0xfffc
    3e2a:	752080e7          	jalr	1874(ra) # 10000578 <bk_printf>
    3e2e:	26e5                	c.jal	4216 <ty_adapter_get_free_heap_size>
    3e30:	85aa                	c.mv	a1,a0
    3e32:	00004517          	auipc	a0,0x4
    3e36:	bea50513          	addi	a0,a0,-1046 # 7a1c <irq_handler+0x9c0>
    3e3a:	0fffc097          	auipc	ra,0xfffc
    3e3e:	73e080e7          	jalr	1854(ra) # 10000578 <bk_printf>
    3e42:	4501                	c.li	a0,0
    3e44:	bbb5                	c.j	3bc0 <diff2ya_run+0x4c>

00003e46 <ty_bsdiff_entry>:
    3e46:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    3e48:	7135                	c.addi16sp	sp,-160
    3e4a:	84aa                	c.mv	s1,a0
    3e4c:	00004517          	auipc	a0,0x4
    3e50:	10050513          	addi	a0,a0,256 # 7f4c <irq_handler+0xef0>
    3e54:	0fffc097          	auipc	ra,0xfffc
    3e58:	724080e7          	jalr	1828(ra) # 10000578 <bk_printf>
    3e5c:	05800613          	li	a2,88
    3e60:	4581                	c.li	a1,0
    3e62:	00a8                	c.addi4spn	a0,sp,72
    3e64:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    3e66:	8410                	exec.it	#3     !li	a2,64
    3e68:	4581                	c.li	a1,0
    3e6a:	0028                	c.addi4spn	a0,sp,8
    3e6c:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    3e6e:	86a6                	c.mv	a3,s1
    3e70:	0030                	c.addi4spn	a2,sp,8
    3e72:	012c                	c.addi4spn	a1,sp,136
    3e74:	00a8                	c.addi4spn	a0,sp,72
    3e76:	9c0ff0ef          	jal	ra,3036 <bsdiff_flash_init>
    3e7a:	c539                	c.beqz	a0,3ec8 <ty_bsdiff_entry+0x82>
    3e7c:	842a                	c.mv	s0,a0
    3e7e:	00004517          	auipc	a0,0x4
    3e82:	0ea50513          	addi	a0,a0,234 # 7f68 <irq_handler+0xf0c>
    3e86:	0fffc097          	auipc	ra,0xfffc
    3e8a:	6f2080e7          	jalr	1778(ra) # 10000578 <bk_printf>
    3e8e:	00540793          	addi	a5,s0,5
    3e92:	4705                	c.li	a4,1
    3e94:	02f77063          	bgeu	a4,a5,3eb4 <ty_bsdiff_entry+0x6e>
    3e98:	012c                	c.addi4spn	a1,sp,136
    3e9a:	8626                	c.mv	a2,s1
    3e9c:	00a8                	c.addi4spn	a0,sp,72
    3e9e:	ec7fe0ef          	jal	ra,2d64 <bsdiff_mag_clear>
    3ea2:	85aa                	c.mv	a1,a0
    3ea4:	00004517          	auipc	a0,0x4
    3ea8:	0fc50513          	addi	a0,a0,252 # 7fa0 <irq_handler+0xf44>
    3eac:	0fffc097          	auipc	ra,0xfffc
    3eb0:	6cc080e7          	jalr	1740(ra) # 10000578 <bk_printf>
    3eb4:	85a2                	c.mv	a1,s0
    3eb6:	00004517          	auipc	a0,0x4
    3eba:	0fe50513          	addi	a0,a0,254 # 7fb4 <irq_handler+0xf58>
    3ebe:	0fffc097          	auipc	ra,0xfffc
    3ec2:	6ba080e7          	jalr	1722(ra) # 10000578 <bk_printf>
    3ec6:	a805                	c.j	3ef6 <ty_bsdiff_entry+0xb0>
    3ec8:	00004517          	auipc	a0,0x4
    3ecc:	0b450513          	addi	a0,a0,180 # 7f7c <irq_handler+0xf20>
    3ed0:	0fffc097          	auipc	ra,0xfffc
    3ed4:	6a8080e7          	jalr	1704(ra) # 10000578 <bk_printf>
    3ed8:	002c                	c.addi4spn	a1,sp,8
    3eda:	8626                	c.mv	a2,s1
    3edc:	00a8                	c.addi4spn	a0,sp,72
    3ede:	3959                	c.jal	3b74 <diff2ya_run>
    3ee0:	842a                	c.mv	s0,a0
    3ee2:	85aa                	c.mv	a1,a0
    3ee4:	00004517          	auipc	a0,0x4
    3ee8:	0a850513          	addi	a0,a0,168 # 7f8c <irq_handler+0xf30>
    3eec:	0fffc097          	auipc	ra,0xfffc
    3ef0:	68c080e7          	jalr	1676(ra) # 10000578 <bk_printf>
    3ef4:	fc49                	c.bnez	s0,3e8e <ty_bsdiff_entry+0x48>
    3ef6:	8522                	c.mv	a0,s0
    3ef8:	610d                	c.addi16sp	sp,160
    3efa:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>

00003efc <prvInsertBlockIntoFreeList>:
    3efc:	88818e93          	addi	t4,gp,-1912 # 30000180 <xStart>
    3f00:	8776                	c.mv	a4,t4
    3f02:	000eae83          	lw	t4,0(t4)
    3f06:	feaeede3          	bltu	t4,a0,3f00 <prvInsertBlockIntoFreeList+0x4>
    3f0a:	4350                	c.lw	a2,4(a4)
    3f0c:	00c706b3          	add	a3,a4,a2
    3f10:	00d51963          	bne	a0,a3,3f22 <prvInsertBlockIntoFreeList+0x26>
    3f14:	00452283          	lw	t0,4(a0)
    3f18:	853a                	c.mv	a0,a4
    3f1a:	00c28333          	add	t1,t0,a2
    3f1e:	00672223          	sw	t1,4(a4)
    3f22:	00452383          	lw	t2,4(a0)
    3f26:	007505b3          	add	a1,a0,t2
    3f2a:	00be9f63          	bne	t4,a1,3f48 <prvInsertBlockIntoFreeList+0x4c>
    3f2e:	8841a803          	lw	a6,-1916(gp) # 3000017c <pxEnd>
    3f32:	010e8b63          	beq	t4,a6,3f48 <prvInsertBlockIntoFreeList+0x4c>
    3f36:	004ea783          	lw	a5,4(t4)
    3f3a:	007788b3          	add	a7,a5,t2
    3f3e:	01152223          	sw	a7,4(a0)
    3f42:	8a00                	exec.it	#68     !lw	t3,0(a4)
    3f44:	000e2e83          	lw	t4,0(t3)
    3f48:	01d52023          	sw	t4,0(a0)
    3f4c:	00e50363          	beq	a0,a4,3f52 <prvInsertBlockIntoFreeList+0x56>
    3f50:	c308                	c.sw	a0,0(a4)
    3f52:	8082                	c.jr	ra

00003f54 <ty_pvPortMalloc>:
    3f54:	5e7012ef          	jal	t0,5d3a <__riscv_save_4>
    3f58:	8701aa03          	lw	s4,-1936(gp) # 30000168 <xBlockAllocatedBit>
    3f5c:	00aa77b3          	and	a5,s4,a0
    3f60:	e385                	c.bnez	a5,3f80 <ty_pvPortMalloc+0x2c>
    3f62:	fff50293          	addi	t0,a0,-1
    3f66:	5759                	c.li	a4,-10
    3f68:	00576c63          	bltu	a4,t0,3f80 <ty_pvPortMalloc+0x2c>
    3f6c:	00850593          	addi	a1,a0,8
    3f70:	891d                	c.andi	a0,7
    3f72:	cd35                	c.beqz	a0,3fee <ty_pvPortMalloc+0x9a>
    3f74:	ff85f313          	andi	t1,a1,-8
    3f78:	00830393          	addi	t2,t1,8
    3f7c:	0675e863          	bltu	a1,t2,3fec <ty_pvPortMalloc+0x98>
    3f80:	4481                	c.li	s1,0
    3f82:	8526                	c.mv	a0,s1
    3f84:	5eb0106f          	j	5d6e <__riscv_restore_4>
    3f88:	8e22                	c.mv	t3,s0
    3f8a:	8426                	c.mv	s0,s1
    3f8c:	a89d                	c.j	4002 <ty_pvPortMalloc+0xae>
    3f8e:	01d52223          	sw	t4,4(a0)
    3f92:	c04c                	c.sw	a1,4(s0)
    3f94:	37a5                	c.jal	3efc <prvInsertBlockIntoFreeList>
    3f96:	00442f83          	lw	t6,4(s0)
    3f9a:	87c18293          	addi	t0,gp,-1924 # 30000174 <xMinimumEverFreeBytesRemaining>
    3f9e:	41f907b3          	sub	a5,s2,t6
    3fa2:	8630                	exec.it	#83     !lw	a4,0(t0)
    3fa4:	00f9a023          	sw	a5,0(s3)
    3fa8:	00e7f463          	bgeu	a5,a4,3fb0 <ty_pvPortMalloc+0x5c>
    3fac:	00f2a023          	sw	a5,0(t0)
    3fb0:	87818313          	addi	t1,gp,-1928 # 30000170 <xNumberOfSuccessfulAllocations>
    3fb4:	8430                	exec.it	#19     !lw	t2,0(t1)
    3fb6:	01fa6533          	or	a0,s4,t6
    3fba:	00138593          	addi	a1,t2,1
    3fbe:	0074f693          	andi	a3,s1,7
    3fc2:	c048                	c.sw	a0,4(s0)
    3fc4:	00042023          	sw	zero,0(s0)
    3fc8:	00b32023          	sw	a1,0(t1)
    3fcc:	dadd                	c.beqz	a3,3f82 <ty_pvPortMalloc+0x2e>
    3fce:	00004617          	auipc	a2,0x4
    3fd2:	00a60613          	addi	a2,a2,10 # 7fd8 <irq_handler+0xf7c>
    3fd6:	0e500593          	li	a1,229
    3fda:	00004517          	auipc	a0,0x4
    3fde:	09250513          	addi	a0,a0,146 # 806c <irq_handler+0x1010>
    3fe2:	0fffc097          	auipc	ra,0xfffc
    3fe6:	596080e7          	jalr	1430(ra) # 10000578 <bk_printf>
    3fea:	a001                	c.j	3fea <ty_pvPortMalloc+0x96>
    3fec:	859e                	c.mv	a1,t2
    3fee:	88018993          	addi	s3,gp,-1920 # 30000178 <xFreeBytesRemaining>
    3ff2:	0009a903          	lw	s2,0(s3)
    3ff6:	f8b965e3          	bltu	s2,a1,3f80 <ty_pvPortMalloc+0x2c>
    3ffa:	88818e13          	addi	t3,gp,-1912 # 30000180 <xStart>
    3ffe:	000e2403          	lw	s0,0(t3)
    4002:	4054                	c.lw	a3,4(s0)
    4004:	00b6f463          	bgeu	a3,a1,400c <ty_pvPortMalloc+0xb8>
    4008:	4004                	c.lw	s1,0(s0)
    400a:	fcbd                	c.bnez	s1,3f88 <ty_pvPortMalloc+0x34>
    400c:	8841a603          	lw	a2,-1916(gp) # 3000017c <pxEnd>
    4010:	f68608e3          	beq	a2,s0,3f80 <ty_pvPortMalloc+0x2c>
    4014:	8e10                	exec.it	#71     !lw	a7,0(s0)
    4016:	000e2803          	lw	a6,0(t3)
    401a:	011e2023          	sw	a7,0(t3)
    401e:	8070                	exec.it	#49     !lw	t3,4(s0)
    4020:	4f41                	c.li	t5,16
    4022:	40be0eb3          	sub	t4,t3,a1
    4026:	00880493          	addi	s1,a6,8
    402a:	f7df76e3          	bgeu	t5,t4,3f96 <ty_pvPortMalloc+0x42>
    402e:	00b40533          	add	a0,s0,a1
    4032:	00757093          	andi	ra,a0,7
    4036:	f4008ce3          	beqz	ra,3f8e <ty_pvPortMalloc+0x3a>
    403a:	00004617          	auipc	a2,0x4
    403e:	f9e60613          	addi	a2,a2,-98 # 7fd8 <irq_handler+0xf7c>
    4042:	0af00593          	li	a1,175
    4046:	00004517          	auipc	a0,0x4
    404a:	fba50513          	addi	a0,a0,-70 # 8000 <irq_handler+0xfa4>
    404e:	0fffc097          	auipc	ra,0xfffc
    4052:	52a080e7          	jalr	1322(ra) # 10000578 <bk_printf>
    4056:	a001                	c.j	4056 <ty_pvPortMalloc+0x102>

00004058 <ty_vPortFree>:
    4058:	c551                	c.beqz	a0,40e4 <ty_vPortFree+0x8c>
    405a:	4fb012ef          	jal	t0,5d54 <__riscv_save_0>
    405e:	ffc52703          	lw	a4,-4(a0)
    4062:	8701a783          	lw	a5,-1936(gp) # 30000168 <xBlockAllocatedBit>
    4066:	00f776b3          	and	a3,a4,a5
    406a:	e285                	c.bnez	a3,408a <ty_vPortFree+0x32>
    406c:	00004617          	auipc	a2,0x4
    4070:	f6c60613          	addi	a2,a2,-148 # 7fd8 <irq_handler+0xf7c>
    4074:	0f900593          	li	a1,249
    4078:	00004517          	auipc	a0,0x4
    407c:	06450513          	addi	a0,a0,100 # 80dc <irq_handler+0x1080>
    4080:	0fffc097          	auipc	ra,0xfffc
    4084:	4f8080e7          	jalr	1272(ra) # 10000578 <bk_printf>
    4088:	a001                	c.j	4088 <ty_vPortFree+0x30>
    408a:	ff852083          	lw	ra,-8(a0)
    408e:	02008163          	beqz	ra,40b0 <ty_vPortFree+0x58>
    4092:	00004617          	auipc	a2,0x4
    4096:	f4660613          	addi	a2,a2,-186 # 7fd8 <irq_handler+0xf7c>
    409a:	0fa00593          	li	a1,250
    409e:	00004517          	auipc	a0,0x4
    40a2:	09a50513          	addi	a0,a0,154 # 8138 <irq_handler+0x10dc>
    40a6:	0fffc097          	auipc	ra,0xfffc
    40aa:	4d2080e7          	jalr	1234(ra) # 10000578 <bk_printf>
    40ae:	a001                	c.j	40ae <ty_vPortFree+0x56>
    40b0:	88018393          	addi	t2,gp,-1920 # 30000178 <xFreeBytesRemaining>
    40b4:	0003a583          	lw	a1,0(t2)
    40b8:	fff7c293          	not	t0,a5
    40bc:	00e2f333          	and	t1,t0,a4
    40c0:	fe652e23          	sw	t1,-4(a0)
    40c4:	00658633          	add	a2,a1,t1
    40c8:	1561                	c.addi	a0,-8
    40ca:	00c3a023          	sw	a2,0(t2)
    40ce:	353d                	c.jal	3efc <prvInsertBlockIntoFreeList>
    40d0:	87418513          	addi	a0,gp,-1932 # 3000016c <xNumberOfSuccessfulFrees>
    40d4:	00052803          	lw	a6,0(a0)
    40d8:	00180893          	addi	a7,a6,1
    40dc:	01152023          	sw	a7,0(a0)
    40e0:	4990106f          	j	5d78 <__riscv_restore_0>
    40e4:	8082                	c.jr	ra

000040e6 <xPortGetFreeHeapSize>:
    40e6:	8801a503          	lw	a0,-1920(gp) # 30000178 <xFreeBytesRemaining>
    40ea:	8082                	c.jr	ra

000040ec <ty_HeapInfoInit>:
    40ec:	3ff00793          	li	a5,1023
    40f0:	02b7f363          	bgeu	a5,a1,4116 <ty_HeapInfoInit+0x2a>
    40f4:	0ff50293          	addi	t0,a0,255
    40f8:	952e                	c.add	a0,a1
    40fa:	fff50393          	addi	t2,a0,-1
    40fe:	f002f313          	andi	t1,t0,-256
    4102:	f003f593          	andi	a1,t2,-256
    4106:	40658633          	sub	a2,a1,t1
    410a:	8661a423          	sw	t1,-1944(gp) # 30000160 <g_u32TuyaHeapBase>
    410e:	86c1a623          	sw	a2,-1940(gp) # 30000164 <g_u32TuyaHeapSize>
    4112:	4501                	c.li	a0,0
    4114:	8082                	c.jr	ra
    4116:	557d                	c.li	a0,-1
    4118:	8082                	c.jr	ra

0000411a <prvHeapInit>:
    411a:	43b012ef          	jal	t0,5d54 <__riscv_save_0>
    411e:	37f9                	c.jal	40ec <ty_HeapInfoInit>
    4120:	c911                	c.beqz	a0,4134 <prvHeapInit+0x1a>
    4122:	00004517          	auipc	a0,0x4
    4126:	06250513          	addi	a0,a0,98 # 8184 <irq_handler+0x1128>
    412a:	597d                	c.li	s2,-1
    412c:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    412e:	854a                	c.mv	a0,s2
    4130:	4490106f          	j	5d78 <__riscv_restore_0>
    4134:	8681a483          	lw	s1,-1944(gp) # 30000160 <g_u32TuyaHeapBase>
    4138:	86c1a403          	lw	s0,-1940(gp) # 30000164 <g_u32TuyaHeapSize>
    413c:	8622                	c.mv	a2,s0
    413e:	85a6                	c.mv	a1,s1
    4140:	892a                	c.mv	s2,a0
    4142:	00004517          	auipc	a0,0x4
    4146:	05a50513          	addi	a0,a0,90 # 819c <irq_handler+0x1140>
    414a:	8c00                	exec.it	#6     !jal	ra,60f4 <printf>
    414c:	00848633          	add	a2,s1,s0
    4150:	ff860293          	addi	t0,a2,-8
    4154:	ff82f313          	andi	t1,t0,-8
    4158:	8801a623          	sw	zero,-1908(gp) # 30000184 <xStart+0x4>
    415c:	8891a423          	sw	s1,-1912(gp) # 30000180 <xStart>
    4160:	409307b3          	sub	a5,t1,s1
    4164:	00032223          	sw	zero,4(t1)
    4168:	00032023          	sw	zero,0(t1)
    416c:	800003b7          	lui	t2,0x80000
    4170:	8861a223          	sw	t1,-1916(gp) # 3000017c <pxEnd>
    4174:	86f1ae23          	sw	a5,-1924(gp) # 30000174 <xMinimumEverFreeBytesRemaining>
    4178:	88f1a023          	sw	a5,-1920(gp) # 30000178 <xFreeBytesRemaining>
    417c:	8671a823          	sw	t2,-1936(gp) # 30000168 <xBlockAllocatedBit>
    4180:	c0dc                	c.sw	a5,4(s1)
    4182:	0064a023          	sw	t1,0(s1)
    4186:	b765                	c.j	412e <prvHeapInit+0x14>

00004188 <ty_adapt_flash_get_cfg>:
    4188:	81018513          	addi	a0,gp,-2032 # 30000108 <cfg>
    418c:	8082                	c.jr	ra

0000418e <ty_adapt_flash_erase>:
    418e:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    4190:	fff58413          	addi	s0,a1,-1
    4194:	00a402b3          	add	t0,s0,a0
    4198:	6cc524db          	bfoz	s1,a0,27,12
    419c:	6cc2a45b          	bfoz	s0,t0,27,12
    41a0:	00947463          	bgeu	s0,s1,41a8 <ty_adapt_flash_erase+0x1a>
    41a4:	4501                	c.li	a0,0
    41a6:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>
    41a8:	00c49513          	slli	a0,s1,0xc
    41ac:	0485                	c.addi	s1,1
    41ae:	3c04a4db          	bfoz	s1,s1,15,0
    41b2:	0fffc097          	auipc	ra,0xfffc
    41b6:	0fe080e7          	jalr	254(ra) # 100002b0 <flash_erase_sector>
    41ba:	b7dd                	c.j	41a0 <ty_adapt_flash_erase+0x12>

000041bc <ty_adapt_flash_write>:
    41bc:	399012ef          	jal	t0,5d54 <__riscv_save_0>
    41c0:	87ae                	c.mv	a5,a1
    41c2:	85aa                	c.mv	a1,a0
    41c4:	853e                	c.mv	a0,a5
    41c6:	0fffc097          	auipc	ra,0xfffc
    41ca:	264080e7          	jalr	612(ra) # 1000042a <flash_write_data>
    41ce:	4501                	c.li	a0,0
    41d0:	3a90106f          	j	5d78 <__riscv_restore_0>

000041d4 <ty_adapt_flash_read>:
    41d4:	381012ef          	jal	t0,5d54 <__riscv_save_0>
    41d8:	87ae                	c.mv	a5,a1
    41da:	85aa                	c.mv	a1,a0
    41dc:	853e                	c.mv	a0,a5
    41de:	0fffc097          	auipc	ra,0xfffc
    41e2:	1be080e7          	jalr	446(ra) # 1000039c <flash_read_data>
    41e6:	4501                	c.li	a0,0
    41e8:	3910106f          	j	5d78 <__riscv_restore_0>

000041ec <ty_adapt_malloc>:
    41ec:	369012ef          	jal	t0,5d54 <__riscv_save_0>
    41f0:	89018793          	addi	a5,gp,-1904 # 30000188 <heap_init_flg.0>
    41f4:	8854                	exec.it	#45     !lbu	a4,0(a5)
    41f6:	842a                	c.mv	s0,a0
    41f8:	eb11                	c.bnez	a4,420c <ty_adapt_malloc+0x20>
    41fa:	4085                	c.li	ra,1
    41fc:	65e5                	c.lui	a1,0x19
    41fe:	2fffd517          	auipc	a0,0x2fffd
    4202:	12a50513          	addi	a0,a0,298 # 30001328 <mem_buff>
    4206:	00178023          	sb	ra,0(a5)
    420a:	3f01                	c.jal	411a <prvHeapInit>
    420c:	8522                	c.mv	a0,s0
    420e:	3399                	c.jal	3f54 <ty_pvPortMalloc>
    4210:	3690106f          	j	5d78 <__riscv_restore_0>

00004214 <ty_adapt_free>:
    4214:	b591                	c.j	4058 <ty_vPortFree>

00004216 <ty_adapter_get_free_heap_size>:
    4216:	bdc1                	c.j	40e6 <xPortGetFreeHeapSize>

00004218 <hash_crc32i_init>:
    4218:	557d                	c.li	a0,-1
    421a:	8082                	c.jr	ra

0000421c <hash_crc32i_update>:
    421c:	962e                	c.add	a2,a1
    421e:	00004717          	auipc	a4,0x4
    4222:	f9a70713          	addi	a4,a4,-102 # 81b8 <s_crc32>
    4226:	00c59363          	bne	a1,a2,422c <hash_crc32i_update+0x10>
    422a:	8082                	c.jr	ra
    422c:	0005c783          	lbu	a5,0(a1) # 19000 <_data_lmastart+0xf6ba>
    4230:	0585                	c.addi	a1,1
    4232:	00a7c2b3          	xor	t0,a5,a0
    4236:	0892a35b          	bfoz	t1,t0,2,9
    423a:	006703b3          	add	t2,a4,t1
    423e:	8654                	exec.it	#107     !lw	a3,0(t2) # 80000000 <_stack+0x4ffa0000>
    4240:	8121                	c.srli	a0,0x8
    4242:	8d35                	c.xor	a0,a3
    4244:	b7cd                	c.j	4226 <hash_crc32i_update+0xa>

00004246 <hash_crc32i_finish>:
    4246:	fff54513          	not	a0,a0
    424a:	8082                	c.jr	ra

0000424c <nb_fread>:
    424c:	02c58633          	mul	a2,a1,a2
    4250:	882a                	c.mv	a6,a0
    4252:	4a9c                	c.lw	a5,16(a3)
    4254:	8536                	c.mv	a0,a3
    4256:	85c2                	c.mv	a1,a6
    4258:	86ba                	c.mv	a3,a4
    425a:	8782                	c.jr	a5

0000425c <xz_init>:
    425c:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    425e:	13800613          	li	a2,312
    4262:	84ae                	c.mv	s1,a1
    4264:	4581                	c.li	a1,0
    4266:	842a                	c.mv	s0,a0
    4268:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    426a:	6585                	c.lui	a1,0x1
    426c:	4509                	c.li	a0,2
    426e:	237010ef          	jal	ra,5ca4 <xz_dec_init>
    4272:	cc08                	c.sw	a0,24(s0)
    4274:	c919                	c.beqz	a0,428a <xz_init+0x2e>
    4276:	4651                	c.li	a2,20
    4278:	85a6                	c.mv	a1,s1
    427a:	01c40513          	addi	a0,s0,28
    427e:	8804                	exec.it	#12     !sw	zero,4(s0)
    4280:	00042423          	sw	zero,8(s0)
    4284:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    4286:	4505                	c.li	a0,1
    4288:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>
    428a:	4501                	c.li	a0,0
    428c:	bff5                	c.j	4288 <xz_init+0x2c>

0000428e <xz_read>:
    428e:	2ad012ef          	jal	t0,5d3a <__riscv_save_4>
    4292:	c159                	c.beqz	a0,4318 <xz_read+0x8a>
    4294:	c1d1                	c.beqz	a1,4318 <xz_read+0x8a>
    4296:	84b2                	c.mv	s1,a2
    4298:	ca39                	c.beqz	a2,42ee <xz_read+0x60>
    429a:	03050993          	addi	s3,a0,48
    429e:	842a                	c.mv	s0,a0
    42a0:	8936                	c.mv	s2,a3
    42a2:	01c50a13          	addi	s4,a0,28
    42a6:	01352023          	sw	s3,0(a0)
    42aa:	c54c                	c.sw	a1,12(a0)
    42ac:	c950                	c.sw	a2,20(a0)
    42ae:	00052823          	sw	zero,16(a0)
    42b2:	4058                	c.lw	a4,4(s0)
    42b4:	441c                	c.lw	a5,8(s0)
    42b6:	00f71b63          	bne	a4,a5,42cc <xz_read+0x3e>
    42ba:	874a                	c.mv	a4,s2
    42bc:	86d2                	c.mv	a3,s4
    42be:	10000613          	li	a2,256
    42c2:	4585                	c.li	a1,1
    42c4:	854e                	c.mv	a0,s3
    42c6:	3759                	c.jal	424c <nb_fread>
    42c8:	8804                	exec.it	#12     !sw	zero,4(s0)
    42ca:	c408                	c.sw	a0,8(s0)
    42cc:	4c08                	c.lw	a0,24(s0)
    42ce:	85a2                	c.mv	a1,s0
    42d0:	336010ef          	jal	ra,5606 <xz_dec_run>
    42d4:	82aa                	c.mv	t0,a0
    42d6:	ed19                	c.bnez	a0,42f4 <xz_read+0x66>
    42d8:	4808                	c.lw	a0,16(s0)
    42da:	fc951ce3          	bne	a0,s1,42b2 <xz_read+0x24>
    42de:	13442583          	lw	a1,308(s0)
    42e2:	00042823          	sw	zero,16(s0)
    42e6:	00958633          	add	a2,a1,s1
    42ea:	12c42a23          	sw	a2,308(s0)
    42ee:	8526                	c.mv	a0,s1
    42f0:	27f0106f          	j	5d6e <__riscv_restore_4>
    42f4:	4c08                	c.lw	a0,24(s0)
    42f6:	0012ef5b          	bnec	t0,1,4314 <xz_read+0x86>
    42fa:	4814                	c.lw	a3,16(s0)
    42fc:	13442303          	lw	t1,308(s0)
    4300:	10040093          	addi	ra,s0,256
    4304:	00d303b3          	add	t2,t1,a3
    4308:	0270aa23          	sw	t2,52(ra)
    430c:	1cf010ef          	jal	ra,5cda <xz_dec_end>
    4310:	4804                	c.lw	s1,16(s0)
    4312:	bff1                	c.j	42ee <xz_read+0x60>
    4314:	1c7010ef          	jal	ra,5cda <xz_dec_end>
    4318:	4481                	c.li	s1,0
    431a:	bfd1                	c.j	42ee <xz_read+0x60>

0000431c <xz_crc32_init>:
    431c:	edb885b7          	lui	a1,0xedb88
    4320:	30016617          	auipc	a2,0x30016
    4324:	00860613          	addi	a2,a2,8 # 3001a328 <xz_crc32_table>
    4328:	4701                	c.li	a4,0
    432a:	32058293          	addi	t0,a1,800 # edb88320 <_stack+0xbdb28320>
    432e:	10000513          	li	a0,256
    4332:	87ba                	c.mv	a5,a4
    4334:	46a1                	c.li	a3,8
    4336:	0007b35b          	bfos	t1,a5,0,0
    433a:	0017d813          	srli	a6,a5,0x1
    433e:	005373b3          	and	t2,t1,t0
    4342:	16fd                	c.addi	a3,-1
    4344:	007847b3          	xor	a5,a6,t2
    4348:	f6fd                	c.bnez	a3,4336 <xz_crc32_init+0x1a>
    434a:	c21c                	c.sw	a5,0(a2)
    434c:	0705                	c.addi	a4,1
    434e:	0611                	c.addi	a2,4
    4350:	fea711e3          	bne	a4,a0,4332 <xz_crc32_init+0x16>
    4354:	8082                	c.jr	ra

00004356 <xz_crc32>:
    4356:	fff64813          	not	a6,a2
    435a:	95aa                	c.add	a1,a0
    435c:	30016717          	auipc	a4,0x30016
    4360:	fcc70713          	addi	a4,a4,-52 # 3001a328 <xz_crc32_table>
    4364:	00b51563          	bne	a0,a1,436e <xz_crc32+0x18>
    4368:	fff84513          	not	a0,a6
    436c:	8082                	c.jr	ra
    436e:	00054783          	lbu	a5,0(a0)
    4372:	0ff87693          	andi	a3,a6,255
    4376:	00d7c2b3          	xor	t0,a5,a3
    437a:	0c57035b          	lea.w	t1,a4,t0
    437e:	8430                	exec.it	#19     !lw	t2,0(t1)
    4380:	00885613          	srli	a2,a6,0x8
    4384:	0505                	c.addi	a0,1
    4386:	00c3c833          	xor	a6,t2,a2
    438a:	bfe9                	c.j	4364 <xz_crc32+0xe>

0000438c <lzma_len>:
    438c:	411c                	c.lw	a5,0(a0)
    438e:	01000737          	lui	a4,0x1000
    4392:	02e7f863          	bgeu	a5,a4,43c2 <lzma_len+0x36>
    4396:	4914                	c.lw	a3,16(a0)
    4398:	00c52383          	lw	t2,12(a0)
    439c:	00452283          	lw	t0,4(a0)
    43a0:	00879313          	slli	t1,a5,0x8
    43a4:	00168813          	addi	a6,a3,1
    43a8:	00d388b3          	add	a7,t2,a3
    43ac:	00652023          	sw	t1,0(a0)
    43b0:	01052823          	sw	a6,16(a0)
    43b4:	0008ce03          	lbu	t3,0(a7)
    43b8:	00829e93          	slli	t4,t0,0x8
    43bc:	8460                	exec.it	#50     !add	t5,t3,t4
    43be:	01e52223          	sw	t5,4(a0)
    43c2:	00052f83          	lw	t6,0(a0)
    43c6:	0005d703          	lhu	a4,0(a1)
    43ca:	00bfd793          	srli	a5,t6,0xb
    43ce:	02e782b3          	mul	t0,a5,a4
    43d2:	00452303          	lw	t1,4(a0)
    43d6:	0a537e63          	bgeu	t1,t0,4492 <lzma_len+0x106>
    43da:	00552023          	sw	t0,0(a0)
    43de:	6385                	c.lui	t2,0x1
    43e0:	0005d303          	lhu	t1,0(a1)
    43e4:	80038693          	addi	a3,t2,-2048 # 800 <__rtos_signature_freertos_v10_3+0x800>
    43e8:	40668f33          	sub	t5,a3,t1
    43ec:	405f5813          	srai	a6,t5,0x5
    43f0:	00461e13          	slli	t3,a2,0x4
    43f4:	010308b3          	add	a7,t1,a6
    43f8:	004e0e93          	addi	t4,t3,4
    43fc:	01159023          	sh	a7,0(a1)
    4400:	4289                	c.li	t0,2
    4402:	95f6                	c.add	a1,t4
    4404:	4f21                	c.li	t5,8
    4406:	06552423          	sw	t0,104(a0)
    440a:	6605                	c.lui	a2,0x1
    440c:	4785                	c.li	a5,1
    440e:	01000fb7          	lui	t6,0x1000
    4412:	80060293          	addi	t0,a2,-2048 # 800 <__rtos_signature_freertos_v10_3+0x800>
    4416:	00179313          	slli	t1,a5,0x1
    441a:	0af583db          	lea.h	t2,a1,a5
    441e:	411c                	c.lw	a5,0(a0)
    4420:	03f7f663          	bgeu	a5,t6,444c <lzma_len+0xc0>
    4424:	01052883          	lw	a7,16(a0)
    4428:	00c52803          	lw	a6,12(a0)
    442c:	4154                	c.lw	a3,4(a0)
    442e:	8864                	exec.it	#60     !slli	a4,a5,0x8
    4430:	00188e13          	addi	t3,a7,1
    4434:	01180eb3          	add	t4,a6,a7
    4438:	c118                	c.sw	a4,0(a0)
    443a:	01c52823          	sw	t3,16(a0)
    443e:	000ec603          	lbu	a2,0(t4)
    4442:	00869793          	slli	a5,a3,0x8
    4446:	00f606b3          	add	a3,a2,a5
    444a:	c154                	c.sw	a3,4(a0)
    444c:	00052803          	lw	a6,0(a0)
    4450:	0003d883          	lhu	a7,0(t2)
    4454:	00b85713          	srli	a4,a6,0xb
    4458:	03170e33          	mul	t3,a4,a7
    445c:	00452e83          	lw	t4,4(a0)
    4460:	879a                	c.mv	a5,t1
    4462:	0fcef763          	bgeu	t4,t3,4550 <lzma_len+0x1c4>
    4466:	01c52023          	sw	t3,0(a0)
    446a:	0003d883          	lhu	a7,0(t2)
    446e:	41128e33          	sub	t3,t0,a7
    4472:	405e5e93          	srai	t4,t3,0x5
    4476:	01d88333          	add	t1,a7,t4
    447a:	00639023          	sh	t1,0(t2)
    447e:	f9e7ece3          	bltu	a5,t5,4416 <lzma_len+0x8a>
    4482:	552c                	c.lw	a1,104(a0)
    4484:	41e58f33          	sub	t5,a1,t5
    4488:	00ff0fb3          	add	t6,t5,a5
    448c:	07f52423          	sw	t6,104(a0)
    4490:	8082                	c.jr	ra
    4492:	405f83b3          	sub	t2,t6,t0
    4496:	405306b3          	sub	a3,t1,t0
    449a:	00752023          	sw	t2,0(a0)
    449e:	c154                	c.sw	a3,4(a0)
    44a0:	0005d803          	lhu	a6,0(a1)
    44a4:	01000f37          	lui	t5,0x1000
    44a8:	00585893          	srli	a7,a6,0x5
    44ac:	41180e33          	sub	t3,a6,a7
    44b0:	01c59023          	sh	t3,0(a1)
    44b4:	00052e83          	lw	t4,0(a0)
    44b8:	03eef763          	bgeu	t4,t5,44e6 <lzma_len+0x15a>
    44bc:	01052303          	lw	t1,16(a0)
    44c0:	00c52283          	lw	t0,12(a0)
    44c4:	415c                	c.lw	a5,4(a0)
    44c6:	008e9f93          	slli	t6,t4,0x8
    44ca:	8c50                	exec.it	#39     !addi	t2,t1,1
    44cc:	006286b3          	add	a3,t0,t1
    44d0:	01f52023          	sw	t6,0(a0)
    44d4:	00752823          	sw	t2,16(a0)
    44d8:	0006c803          	lbu	a6,0(a3)
    44dc:	8864                	exec.it	#60     !slli	a4,a5,0x8
    44de:	00e808b3          	add	a7,a6,a4
    44e2:	01152223          	sw	a7,4(a0)
    44e6:	00052e03          	lw	t3,0(a0)
    44ea:	0025df03          	lhu	t5,2(a1)
    44ee:	00be5e93          	srli	t4,t3,0xb
    44f2:	03ee8fb3          	mul	t6,t4,t5
    44f6:	415c                	c.lw	a5,4(a0)
    44f8:	03f7f763          	bgeu	a5,t6,4526 <lzma_len+0x19a>
    44fc:	01f52023          	sw	t6,0(a0)
    4500:	6885                	c.lui	a7,0x1
    4502:	0025d803          	lhu	a6,2(a1)
    4506:	80088e13          	addi	t3,a7,-2048 # 800 <__rtos_signature_freertos_v10_3+0x800>
    450a:	410e0eb3          	sub	t4,t3,a6
    450e:	405edf93          	srai	t6,t4,0x5
    4512:	0612                	c.slli	a2,0x4
    4514:	01f807b3          	add	a5,a6,t6
    4518:	10460713          	addi	a4,a2,260
    451c:	00f59123          	sh	a5,2(a1)
    4520:	42a9                	c.li	t0,10
    4522:	95ba                	c.add	a1,a4
    4524:	b5c5                	c.j	4404 <lzma_len+0x78>
    4526:	41fe0633          	sub	a2,t3,t6
    452a:	41f78733          	sub	a4,a5,t6
    452e:	c110                	c.sw	a2,0(a0)
    4530:	c158                	c.sw	a4,4(a0)
    4532:	0025d283          	lhu	t0,2(a1)
    4536:	46c9                	c.li	a3,18
    4538:	0052d313          	srli	t1,t0,0x5
    453c:	406283b3          	sub	t2,t0,t1
    4540:	00759123          	sh	t2,2(a1)
    4544:	10000f13          	li	t5,256
    4548:	20458593          	addi	a1,a1,516
    454c:	d534                	c.sw	a3,104(a0)
    454e:	bd75                	c.j	440a <lzma_len+0x7e>
    4550:	41c80333          	sub	t1,a6,t3
    4554:	41ce8633          	sub	a2,t4,t3
    4558:	00652023          	sw	t1,0(a0)
    455c:	c150                	c.sw	a2,4(a0)
    455e:	0003d683          	lhu	a3,0(t2)
    4562:	0785                	c.addi	a5,1
    4564:	0056d813          	srli	a6,a3,0x5
    4568:	41068733          	sub	a4,a3,a6
    456c:	00e39023          	sh	a4,0(t2)
    4570:	b739                	c.j	447e <lzma_len+0xf2>

00004572 <dict_repeat>:
    4572:	4558                	c.lw	a4,12(a0)
    4574:	87aa                	c.mv	a5,a0
    4576:	06e67d63          	bgeu	a2,a4,45f0 <dict_repeat+0x7e>
    457a:	01852283          	lw	t0,24(a0)
    457e:	4501                	c.li	a0,0
    4580:	06567963          	bgeu	a2,t0,45f2 <dict_repeat+0x80>
    4584:	4b94                	c.lw	a3,16(a5)
    4586:	4788                	c.lw	a0,8(a5)
    4588:	0005a303          	lw	t1,0(a1)
    458c:	8e89                	c.sub	a3,a0
    458e:	00d37363          	bgeu	t1,a3,4594 <dict_repeat+0x22>
    4592:	869a                	c.mv	a3,t1
    4594:	40d303b3          	sub	t2,t1,a3
    4598:	0075a023          	sw	t2,0(a1)
    459c:	478c                	c.lw	a1,8(a5)
    459e:	fff64813          	not	a6,a2
    45a2:	00b80733          	add	a4,a6,a1
    45a6:	00b66463          	bltu	a2,a1,45ae <dict_repeat+0x3c>
    45aa:	4bd0                	c.lw	a2,20(a5)
    45ac:	9732                	c.add	a4,a2
    45ae:	0087ae03          	lw	t3,8(a5)
    45b2:	0007a883          	lw	a7,0(a5)
    45b6:	001e0e93          	addi	t4,t3,1
    45ba:	00e88f33          	add	t5,a7,a4
    45be:	01d7a423          	sw	t4,8(a5)
    45c2:	000f4f83          	lbu	t6,0(t5) # 1000000 <SRAM2_SIZE+0xfe58d8>
    45c6:	01c882b3          	add	t0,a7,t3
    45ca:	01f28023          	sb	t6,0(t0)
    45ce:	0147a303          	lw	t1,20(a5)
    45d2:	0705                	c.addi	a4,1
    45d4:	00671363          	bne	a4,t1,45da <dict_repeat+0x68>
    45d8:	4701                	c.li	a4,0
    45da:	16fd                	c.addi	a3,-1
    45dc:	fae9                	c.bnez	a3,45ae <dict_repeat+0x3c>
    45de:	0087a383          	lw	t2,8(a5)
    45e2:	47cc                	c.lw	a1,12(a5)
    45e4:	4505                	c.li	a0,1
    45e6:	0075f663          	bgeu	a1,t2,45f2 <dict_repeat+0x80>
    45ea:	0077a623          	sw	t2,12(a5)
    45ee:	8082                	c.jr	ra
    45f0:	4501                	c.li	a0,0
    45f2:	8082                	c.jr	ra

000045f4 <lzma_main>:
    45f4:	746012ef          	jal	t0,5d3a <__riscv_save_4>
    45f8:	5118                	c.lw	a4,32(a0)
    45fa:	551c                	c.lw	a5,40(a0)
    45fc:	842a                	c.mv	s0,a0
    45fe:	00f77b63          	bgeu	a4,a5,4614 <lzma_main+0x20>
    4602:	06852283          	lw	t0,104(a0)
    4606:	00028763          	beqz	t0,4614 <lzma_main+0x20>
    460a:	4970                	c.lw	a2,84(a0)
    460c:	06850593          	addi	a1,a0,104
    4610:	0561                	c.addi	a0,24
    4612:	3785                	c.jal	4572 <dict_repeat>
    4614:	6485                	c.lui	s1,0x1
    4616:	ae048913          	addi	s2,s1,-1312 # ae0 <__rtos_signature_freertos_v10_3+0xae0>
    461a:	800009b7          	lui	s3,0x80000
    461e:	06840a13          	addi	s4,s0,104
    4622:	9922                	c.add	s2,s0
    4624:	80048a93          	addi	s5,s1,-2048
    4628:	fff9c993          	not	s3,s3
    462c:	02042883          	lw	a7,32(s0)
    4630:	02842383          	lw	t2,40(s0)
    4634:	00042303          	lw	t1,0(s0)
    4638:	0078f763          	bgeu	a7,t2,4646 <lzma_main+0x52>
    463c:	01042803          	lw	a6,16(s0)
    4640:	4854                	c.lw	a3,20(s0)
    4642:	0306fc63          	bgeu	a3,a6,467a <lzma_main+0x86>
    4646:	010005b7          	lui	a1,0x1000
    464a:	4505                	c.li	a0,1
    464c:	3cb37a63          	bgeu	t1,a1,4a20 <lzma_main+0x42c>
    4650:	01042f83          	lw	t6,16(s0)
    4654:	00c42f03          	lw	t5,12(s0)
    4658:	8070                	exec.it	#49     !lw	t3,4(s0)
    465a:	00831613          	slli	a2,t1,0x8
    465e:	001f8713          	addi	a4,t6,1 # 1000001 <SRAM2_SIZE+0xfe58d9>
    4662:	01ff07b3          	add	a5,t5,t6
    4666:	c010                	c.sw	a2,0(s0)
    4668:	c818                	c.sw	a4,16(s0)
    466a:	0007c283          	lbu	t0,0(a5)
    466e:	008e1e93          	slli	t4,t3,0x8
    4672:	01d288b3          	add	a7,t0,t4
    4676:	8444                	exec.it	#42     !sw	a7,4(s0)
    4678:	a665                	c.j	4a20 <lzma_main+0x42c>
    467a:	5868                	c.lw	a0,116(s0)
    467c:	010006b7          	lui	a3,0x1000
    4680:	06442383          	lw	t2,100(s0)
    4684:	00a8f633          	and	a2,a7,a0
    4688:	02d37163          	bgeu	t1,a3,46aa <lzma_main+0xb6>
    468c:	8e14                	exec.it	#79     !lw	t4,12(s0)
    468e:	8070                	exec.it	#49     !lw	t3,4(s0)
    4690:	00180593          	addi	a1,a6,1
    4694:	0322                	c.slli	t1,0x8
    4696:	9876                	c.add	a6,t4
    4698:	8810                	exec.it	#5     !sw	t1,0(s0)
    469a:	c80c                	c.sw	a1,16(s0)
    469c:	00084f03          	lbu	t5,0(a6)
    46a0:	008e1f93          	slli	t6,t3,0x8
    46a4:	01ff0733          	add	a4,t5,t6
    46a8:	c058                	c.sw	a4,4(s0)
    46aa:	00439793          	slli	a5,t2,0x4
    46ae:	00c78533          	add	a0,a5,a2
    46b2:	00042283          	lw	t0,0(s0)
    46b6:	0aa406db          	lea.h	a3,s0,a0
    46ba:	0786de03          	lhu	t3,120(a3) # 1000078 <SRAM2_SIZE+0xfe5950>
    46be:	00b2d313          	srli	t1,t0,0xb
    46c2:	03c30eb3          	mul	t4,t1,t3
    46c6:	404c                	c.lw	a1,4(s0)
    46c8:	21d5fd63          	bgeu	a1,t4,48e2 <lzma_main+0x2ee>
    46cc:	41ca8fb3          	sub	t6,s5,t3
    46d0:	405fd313          	srai	t1,t6,0x5
    46d4:	006e0633          	add	a2,t3,t1
    46d8:	8a40                	exec.it	#100     !sw	t4,0(s0)
    46da:	fff88713          	addi	a4,a7,-1
    46de:	06c69c23          	sh	a2,120(a3)
    46e2:	883a                	c.mv	a6,a4
    46e4:	00089563          	bnez	a7,46ee <lzma_main+0xfa>
    46e8:	5454                	c.lw	a3,44(s0)
    46ea:	fff68813          	addi	a6,a3,-1
    46ee:	5054                	c.lw	a3,36(s0)
    46f0:	4e81                	c.li	t4,0
    46f2:	c691                	c.beqz	a3,46fe <lzma_main+0x10a>
    46f4:	01842283          	lw	t0,24(s0)
    46f8:	01028e33          	add	t3,t0,a6
    46fc:	8870                	exec.it	#53     !lbu	t4,0(t3)
    46fe:	5468                	c.lw	a0,108(s0)
    4700:	583c                	c.lw	a5,112(s0)
    4702:	4fa1                	c.li	t6,8
    4704:	00f8f5b3          	and	a1,a7,a5
    4708:	40af8333          	sub	t1,t6,a0
    470c:	00a59f33          	sll	t5,a1,a0
    4710:	006ed633          	srl	a2,t4,t1
    4714:	00cf0833          	add	a6,t5,a2
    4718:	429d                	c.li	t0,7
    471a:	0102f363          	bgeu	t0,a6,4720 <lzma_main+0x12c>
    471e:	a001                	c.j	471e <lzma_main+0x12a>
    4720:	60000e13          	li	t3,1536
    4724:	03c80eb3          	mul	t4,a6,t3
    4728:	ee448513          	addi	a0,s1,-284
    472c:	4599                	c.li	a1,6
    472e:	00ae87b3          	add	a5,t4,a0
    4732:	00f40fb3          	add	t6,s0,a5
    4736:	0c75e363          	bltu	a1,t2,47fc <lzma_main+0x208>
    473a:	4705                	c.li	a4,1
    473c:	010002b7          	lui	t0,0x1000
    4740:	0ff00e13          	li	t3,255
    4744:	8e10                	exec.it	#71     !lw	a7,0(s0)
    4746:	00171e93          	slli	t4,a4,0x1
    474a:	0aef87db          	lea.h	a5,t6,a4
    474e:	0258f663          	bgeu	a7,t0,477a <lzma_main+0x186>
    4752:	480c                	c.lw	a1,16(s0)
    4754:	4458                	c.lw	a4,12(s0)
    4756:	4054                	c.lw	a3,4(s0)
    4758:	00889613          	slli	a2,a7,0x8
    475c:	00158f13          	addi	t5,a1,1 # 1000001 <SRAM2_SIZE+0xfe58d9>
    4760:	00b703b3          	add	t2,a4,a1
    4764:	c010                	c.sw	a2,0(s0)
    4766:	01e42823          	sw	t5,16(s0)
    476a:	0003c503          	lbu	a0,0(t2)
    476e:	00869813          	slli	a6,a3,0x8
    4772:	01050333          	add	t1,a0,a6
    4776:	00642223          	sw	t1,4(s0)
    477a:	8e10                	exec.it	#71     !lw	a7,0(s0)
    477c:	0007d603          	lhu	a2,0(a5)
    4780:	00b8d693          	srli	a3,a7,0xb
    4784:	02c68f33          	mul	t5,a3,a2
    4788:	404c                	c.lw	a1,4(s0)
    478a:	8776                	c.mv	a4,t4
    478c:	05e5f763          	bgeu	a1,t5,47da <lzma_main+0x1e6>
    4790:	8c40                	exec.it	#38     !sw	t5,0(s0)
    4792:	0007d883          	lhu	a7,0(a5)
    4796:	411a86b3          	sub	a3,s5,a7
    479a:	4056d613          	srai	a2,a3,0x5
    479e:	00c88f33          	add	t5,a7,a2
    47a2:	01e79023          	sh	t5,0(a5)
    47a6:	f8ee7fe3          	bgeu	t3,a4,4744 <lzma_main+0x150>
    47aa:	02042283          	lw	t0,32(s0)
    47ae:	01842f83          	lw	t6,24(s0)
    47b2:	00128e13          	addi	t3,t0,1 # 1000001 <SRAM2_SIZE+0xfe58d9>
    47b6:	03c42023          	sw	t3,32(s0)
    47ba:	005f87b3          	add	a5,t6,t0
    47be:	00e78023          	sb	a4,0(a5)
    47c2:	500c                	c.lw	a1,32(s0)
    47c4:	5058                	c.lw	a4,36(s0)
    47c6:	00b77363          	bgeu	a4,a1,47cc <lzma_main+0x1d8>
    47ca:	d04c                	c.sw	a1,36(s0)
    47cc:	06442e83          	lw	t4,100(s0)
    47d0:	438d                	c.li	t2,3
    47d2:	0fd3ed63          	bltu	t2,t4,48cc <lzma_main+0x2d8>
    47d6:	8600                	exec.it	#66     !sw	zero,100(s0)
    47d8:	bd91                	c.j	462c <lzma_main+0x38>
    47da:	41e88eb3          	sub	t4,a7,t5
    47de:	41e583b3          	sub	t2,a1,t5
    47e2:	8a40                	exec.it	#100     !sw	t4,0(s0)
    47e4:	00742223          	sw	t2,4(s0)
    47e8:	0007d503          	lhu	a0,0(a5)
    47ec:	0705                	c.addi	a4,1
    47ee:	00555813          	srli	a6,a0,0x5
    47f2:	41050333          	sub	t1,a0,a6
    47f6:	00679023          	sh	t1,0(a5)
    47fa:	b775                	c.j	47a6 <lzma_main+0x1b2>
    47fc:	05442383          	lw	t2,84(s0)
    4800:	40770733          	sub	a4,a4,t2
    4804:	0113e563          	bltu	t2,a7,480e <lzma_main+0x21a>
    4808:	02c42883          	lw	a7,44(s0)
    480c:	9746                	c.add	a4,a7
    480e:	c691                	c.beqz	a3,481a <lzma_main+0x226>
    4810:	4c14                	c.lw	a3,24(s0)
    4812:	00e68f33          	add	t5,a3,a4
    4816:	000f4683          	lbu	a3,0(t5)
    481a:	00169e93          	slli	t4,a3,0x1
    481e:	10000593          	li	a1,256
    4822:	4705                	c.li	a4,1
    4824:	010002b7          	lui	t0,0x1000
    4828:	0ff00e13          	li	t3,255
    482c:	00bef333          	and	t1,t4,a1
    4830:	00e58633          	add	a2,a1,a4
    4834:	00042383          	lw	t2,0(s0)
    4838:	00660833          	add	a6,a2,t1
    483c:	0e86                	c.slli	t4,0x1
    483e:	0b0f87db          	lea.h	a5,t6,a6
    4842:	0253f563          	bgeu	t2,t0,486c <lzma_main+0x278>
    4846:	4810                	c.lw	a2,16(s0)
    4848:	4454                	c.lw	a3,12(s0)
    484a:	00839893          	slli	a7,t2,0x8
    484e:	4048                	c.lw	a0,4(s0)
    4850:	00160f13          	addi	t5,a2,1
    4854:	00c68833          	add	a6,a3,a2
    4858:	01142023          	sw	a7,0(s0)
    485c:	01e42823          	sw	t5,16(s0)
    4860:	00084383          	lbu	t2,0(a6)
    4864:	0522                	c.slli	a0,0x8
    4866:	00a388b3          	add	a7,t2,a0
    486a:	8444                	exec.it	#42     !sw	a7,4(s0)
    486c:	4014                	c.lw	a3,0(s0)
    486e:	0007df03          	lhu	t5,0(a5)
    4872:	00b6d613          	srli	a2,a3,0xb
    4876:	03e603b3          	mul	t2,a2,t5
    487a:	4048                	c.lw	a0,4(s0)
    487c:	0706                	c.slli	a4,0x1
    487e:	02757563          	bgeu	a0,t2,48a8 <lzma_main+0x2b4>
    4882:	00742023          	sw	t2,0(s0)
    4886:	0007df03          	lhu	t5,0(a5)
    488a:	fff34313          	not	t1,t1
    488e:	41ea83b3          	sub	t2,s5,t5
    4892:	4053d513          	srai	a0,t2,0x5
    4896:	00af0833          	add	a6,t5,a0
    489a:	0065f5b3          	and	a1,a1,t1
    489e:	01079023          	sh	a6,0(a5)
    48a2:	f8ee75e3          	bgeu	t3,a4,482c <lzma_main+0x238>
    48a6:	b711                	c.j	47aa <lzma_main+0x1b6>
    48a8:	407685b3          	sub	a1,a3,t2
    48ac:	40750833          	sub	a6,a0,t2
    48b0:	c00c                	c.sw	a1,0(s0)
    48b2:	01042223          	sw	a6,4(s0)
    48b6:	0007d883          	lhu	a7,0(a5)
    48ba:	0705                	c.addi	a4,1
    48bc:	0058d693          	srli	a3,a7,0x5
    48c0:	40d88633          	sub	a2,a7,a3
    48c4:	859a                	c.mv	a1,t1
    48c6:	00c79023          	sh	a2,0(a5)
    48ca:	bfe1                	c.j	48a2 <lzma_main+0x2ae>
    48cc:	4525                	c.li	a0,9
    48ce:	01d56763          	bltu	a0,t4,48dc <lzma_main+0x2e8>
    48d2:	ffde8813          	addi	a6,t4,-3
    48d6:	07042223          	sw	a6,100(s0)
    48da:	bb89                	c.j	462c <lzma_main+0x38>
    48dc:	ffae8813          	addi	a6,t4,-6
    48e0:	bfdd                	c.j	48d6 <lzma_main+0x2e2>
    48e2:	41d280b3          	sub	ra,t0,t4
    48e6:	41d588b3          	sub	a7,a1,t4
    48ea:	005e5813          	srli	a6,t3,0x5
    48ee:	410e0f33          	sub	t5,t3,a6
    48f2:	8a20                	exec.it	#84     !sw	ra,0(s0)
    48f4:	8444                	exec.it	#42     !sw	a7,4(s0)
    48f6:	01000fb7          	lui	t6,0x1000
    48fa:	07e69c23          	sh	t5,120(a3)
    48fe:	03f0f363          	bgeu	ra,t6,4924 <lzma_main+0x330>
    4902:	481c                	c.lw	a5,16(s0)
    4904:	8e40                	exec.it	#102     !lw	t0,12(s0)
    4906:	00809713          	slli	a4,ra,0x8
    490a:	00178513          	addi	a0,a5,1
    490e:	00f28e33          	add	t3,t0,a5
    4912:	c018                	c.sw	a4,0(s0)
    4914:	c808                	c.sw	a0,16(s0)
    4916:	000e4303          	lbu	t1,0(t3)
    491a:	00889e93          	slli	t4,a7,0x8
    491e:	01d305b3          	add	a1,t1,t4
    4922:	c04c                	c.sw	a1,4(s0)
    4924:	00042083          	lw	ra,0(s0)
    4928:	0a740f5b          	lea.h	t5,s0,t2
    492c:	1f8f5f83          	lhu	t6,504(t5)
    4930:	00b0d893          	srli	a7,ra,0xb
    4934:	03f88733          	mul	a4,a7,t6
    4938:	00442803          	lw	a6,4(s0)
    493c:	2ee87563          	bgeu	a6,a4,4c26 <lzma_main+0x632>
    4940:	41fa80b3          	sub	ra,s5,t6
    4944:	4050d593          	srai	a1,ra,0x5
    4948:	9fae                	c.add	t6,a1
    494a:	c018                	c.sw	a4,0(s0)
    494c:	1fff1c23          	sh	t6,504(t5)
    4950:	4f19                	c.li	t5,6
    4952:	431d                	c.li	t1,7
    4954:	007f7363          	bgeu	t5,t2,495a <lzma_main+0x366>
    4958:	4329                	c.li	t1,10
    495a:	4c78                	c.lw	a4,92(s0)
    495c:	05842283          	lw	t0,88(s0)
    4960:	487c                	c.lw	a5,84(s0)
    4962:	6dc40593          	addi	a1,s0,1756
    4966:	8522                	c.mv	a0,s0
    4968:	06642223          	sw	t1,100(s0)
    496c:	d038                	c.sw	a4,96(s0)
    496e:	04542e23          	sw	t0,92(s0)
    4972:	cc3c                	c.sw	a5,88(s0)
    4974:	a19ff0ef          	jal	ra,438c <lzma_len>
    4978:	06842883          	lw	a7,104(s0)
    497c:	4695                	c.li	a3,5
    497e:	0116f363          	bgeu	a3,a7,4984 <lzma_main+0x390>
    4982:	4895                	c.li	a7,5
    4984:	00789613          	slli	a2,a7,0x7
    4988:	2d860e13          	addi	t3,a2,728
    498c:	4785                	c.li	a5,1
    498e:	01000eb7          	lui	t4,0x1000
    4992:	03f00813          	li	a6,63
    4996:	400c                	c.lw	a1,0(s0)
    4998:	0afe055b          	lea.h	a0,t3,a5
    499c:	00179393          	slli	t2,a5,0x1
    49a0:	00a400b3          	add	ra,s0,a0
    49a4:	03d5f663          	bgeu	a1,t4,49d0 <lzma_main+0x3dc>
    49a8:	4818                	c.lw	a4,16(s0)
    49aa:	8860                	exec.it	#52     !lw	t1,12(s0)
    49ac:	00442f83          	lw	t6,4(s0)
    49b0:	00859f13          	slli	t5,a1,0x8
    49b4:	00170293          	addi	t0,a4,1 # 1000001 <SRAM2_SIZE+0xfe58d9>
    49b8:	00e307b3          	add	a5,t1,a4
    49bc:	8c40                	exec.it	#38     !sw	t5,0(s0)
    49be:	00542823          	sw	t0,16(s0)
    49c2:	0007c683          	lbu	a3,0(a5)
    49c6:	008f9893          	slli	a7,t6,0x8
    49ca:	01168633          	add	a2,a3,a7
    49ce:	c050                	c.sw	a2,4(s0)
    49d0:	4008                	c.lw	a0,0(s0)
    49d2:	0000df83          	lhu	t6,0(ra)
    49d6:	00b55593          	srli	a1,a0,0xb
    49da:	03f58f33          	mul	t5,a1,t6
    49de:	00442303          	lw	t1,4(s0)
    49e2:	879e                	c.mv	a5,t2
    49e4:	05e37063          	bgeu	t1,t5,4a24 <lzma_main+0x430>
    49e8:	8c40                	exec.it	#38     !sw	t5,0(s0)
    49ea:	0000d603          	lhu	a2,0(ra)
    49ee:	40ca8533          	sub	a0,s5,a2
    49f2:	40555593          	srai	a1,a0,0x5
    49f6:	00b60fb3          	add	t6,a2,a1
    49fa:	01f09023          	sh	t6,0(ra)
    49fe:	f8f87ce3          	bgeu	a6,a5,4996 <lzma_main+0x3a2>
    4a02:	fc078e13          	addi	t3,a5,-64
    4a06:	4e8d                	c.li	t4,3
    4a08:	03ceef63          	bltu	t4,t3,4a46 <lzma_main+0x452>
    4a0c:	05c42a23          	sw	t3,84(s0)
    4a10:	4870                	c.lw	a2,84(s0)
    4a12:	85d2                	c.mv	a1,s4
    4a14:	01840513          	addi	a0,s0,24
    4a18:	b5bff0ef          	jal	ra,4572 <dict_repeat>
    4a1c:	c00518e3          	bnez	a0,462c <lzma_main+0x38>
    4a20:	34e0106f          	j	5d6e <__riscv_restore_4>
    4a24:	41e503b3          	sub	t2,a0,t5
    4a28:	41e30733          	sub	a4,t1,t5
    4a2c:	00742023          	sw	t2,0(s0)
    4a30:	c058                	c.sw	a4,4(s0)
    4a32:	0000d283          	lhu	t0,0(ra)
    4a36:	0785                	c.addi	a5,1
    4a38:	0052d693          	srli	a3,t0,0x5
    4a3c:	40d288b3          	sub	a7,t0,a3
    4a40:	01109023          	sh	a7,0(ra)
    4a44:	bf6d                	c.j	49fe <lzma_main+0x40a>
    4a46:	001e7093          	andi	ra,t3,1
    4a4a:	4335                	c.li	t1,13
    4a4c:	001e5813          	srli	a6,t3,0x1
    4a50:	00208f13          	addi	t5,ra,2
    4a54:	0bc36f63          	bltu	t1,t3,4b12 <lzma_main+0x51e>
    4a58:	fff80f93          	addi	t6,a6,-1
    4a5c:	01ff1eb3          	sll	t4,t5,t6
    4a60:	41c98833          	sub	a6,s3,t3
    4a64:	01d806b3          	add	a3,a6,t4
    4a68:	5d840393          	addi	t2,s0,1496
    4a6c:	4881                	c.li	a7,0
    4a6e:	4805                	c.li	a6,1
    4a70:	010000b7          	lui	ra,0x1000
    4a74:	4305                	c.li	t1,1
    4a76:	05d42a23          	sw	t4,84(s0)
    4a7a:	400c                	c.lw	a1,0(s0)
    4a7c:	01068533          	add	a0,a3,a6
    4a80:	0aa382db          	lea.h	t0,t2,a0
    4a84:	0215f563          	bgeu	a1,ra,4aae <lzma_main+0x4ba>
    4a88:	01042e03          	lw	t3,16(s0)
    4a8c:	4458                	c.lw	a4,12(s0)
    4a8e:	4050                	c.lw	a2,4(s0)
    4a90:	00859f13          	slli	t5,a1,0x8
    4a94:	001e0793          	addi	a5,t3,1
    4a98:	01c70eb3          	add	t4,a4,t3
    4a9c:	8c40                	exec.it	#38     !sw	t5,0(s0)
    4a9e:	c81c                	c.sw	a5,16(s0)
    4aa0:	000ec503          	lbu	a0,0(t4) # 1000000 <SRAM2_SIZE+0xfe58d8>
    4aa4:	00861593          	slli	a1,a2,0x8
    4aa8:	00b50633          	add	a2,a0,a1
    4aac:	c050                	c.sw	a2,4(s0)
    4aae:	00042f03          	lw	t5,0(s0)
    4ab2:	0002de03          	lhu	t3,0(t0) # 1000000 <SRAM2_SIZE+0xfe58d8>
    4ab6:	00bf5713          	srli	a4,t5,0xb
    4aba:	03c707b3          	mul	a5,a4,t3
    4abe:	8820                	exec.it	#20     !lw	t4,4(s0)
    4ac0:	0806                	c.slli	a6,0x1
    4ac2:	02fef163          	bgeu	t4,a5,4ae4 <lzma_main+0x4f0>
    4ac6:	c01c                	c.sw	a5,0(s0)
    4ac8:	0002de83          	lhu	t4,0(t0)
    4acc:	41da8533          	sub	a0,s5,t4
    4ad0:	40555593          	srai	a1,a0,0x5
    4ad4:	00be8f33          	add	t5,t4,a1
    4ad8:	01e29023          	sh	t5,0(t0)
    4adc:	0885                	c.addi	a7,1
    4ade:	f9f8eee3          	bltu	a7,t6,4a7a <lzma_main+0x486>
    4ae2:	b73d                	c.j	4a10 <lzma_main+0x41c>
    4ae4:	40ff0533          	sub	a0,t5,a5
    4ae8:	40fe85b3          	sub	a1,t4,a5
    4aec:	c008                	c.sw	a0,0(s0)
    4aee:	c04c                	c.sw	a1,4(s0)
    4af0:	0002df03          	lhu	t5,0(t0)
    4af4:	0805                	c.addi	a6,1
    4af6:	005f5613          	srli	a2,t5,0x5
    4afa:	40cf0733          	sub	a4,t5,a2
    4afe:	00e29023          	sh	a4,0(t0)
    4b02:	05442e03          	lw	t3,84(s0)
    4b06:	011312b3          	sll	t0,t1,a7
    4b0a:	005e07b3          	add	a5,t3,t0
    4b0e:	c87c                	c.sw	a5,84(s0)
    4b10:	b7f1                	c.j	4adc <lzma_main+0x4e8>
    4b12:	ffb80793          	addi	a5,a6,-5
    4b16:	010003b7          	lui	t2,0x1000
    4b1a:	8e50                	exec.it	#103     !sw	t5,84(s0)
    4b1c:	4018                	c.lw	a4,0(s0)
    4b1e:	02777763          	bgeu	a4,t2,4b4c <lzma_main+0x558>
    4b22:	4810                	c.lw	a2,16(s0)
    4b24:	00c42883          	lw	a7,12(s0)
    4b28:	00442283          	lw	t0,4(s0)
    4b2c:	00871693          	slli	a3,a4,0x8
    4b30:	00160513          	addi	a0,a2,1
    4b34:	00c885b3          	add	a1,a7,a2
    4b38:	c014                	c.sw	a3,0(s0)
    4b3a:	c808                	c.sw	a0,16(s0)
    4b3c:	0005cf83          	lbu	t6,0(a1)
    4b40:	00829e13          	slli	t3,t0,0x8
    4b44:	01cf8eb3          	add	t4,t6,t3
    4b48:	01d42223          	sw	t4,4(s0)
    4b4c:	00042803          	lw	a6,0(s0)
    4b50:	8454                	exec.it	#43     !lw	t5,4(s0)
    4b52:	00185093          	srli	ra,a6,0x1
    4b56:	05442883          	lw	a7,84(s0)
    4b5a:	401f0333          	sub	t1,t5,ra
    4b5e:	41f35293          	srai	t0,t1,0x1f
    4b62:	0050f733          	and	a4,ra,t0
    4b66:	0b12865b          	lea.h	a2,t0,a7
    4b6a:	006706b3          	add	a3,a4,t1
    4b6e:	00160513          	addi	a0,a2,1
    4b72:	17fd                	c.addi	a5,-1
    4b74:	8a20                	exec.it	#84     !sw	ra,0(s0)
    4b76:	c054                	c.sw	a3,4(s0)
    4b78:	c868                	c.sw	a0,84(s0)
    4b7a:	f3cd                	c.bnez	a5,4b1c <lzma_main+0x528>
    4b7c:	00451393          	slli	t2,a0,0x4
    4b80:	6bc40f93          	addi	t6,s0,1724
    4b84:	4685                	c.li	a3,1
    4b86:	01000eb7          	lui	t4,0x1000
    4b8a:	4805                	c.li	a6,1
    4b8c:	04742a23          	sw	t2,84(s0)
    4b90:	400c                	c.lw	a1,0(s0)
    4b92:	00169e13          	slli	t3,a3,0x1
    4b96:	0adf80db          	lea.h	ra,t6,a3
    4b9a:	03d5f563          	bgeu	a1,t4,4bc4 <lzma_main+0x5d0>
    4b9e:	4818                	c.lw	a4,16(s0)
    4ba0:	8e40                	exec.it	#102     !lw	t0,12(s0)
    4ba2:	8454                	exec.it	#43     !lw	t5,4(s0)
    4ba4:	00859313          	slli	t1,a1,0x8
    4ba8:	00170693          	addi	a3,a4,1
    4bac:	00e288b3          	add	a7,t0,a4
    4bb0:	8810                	exec.it	#5     !sw	t1,0(s0)
    4bb2:	c814                	c.sw	a3,16(s0)
    4bb4:	0008c603          	lbu	a2,0(a7)
    4bb8:	008f1513          	slli	a0,t5,0x8
    4bbc:	00a603b3          	add	t2,a2,a0
    4bc0:	00742223          	sw	t2,4(s0)
    4bc4:	400c                	c.lw	a1,0(s0)
    4bc6:	0000d303          	lhu	t1,0(ra) # 1000000 <SRAM2_SIZE+0xfe58d8>
    4bca:	00b5df13          	srli	t5,a1,0xb
    4bce:	026f02b3          	mul	t0,t5,t1
    4bd2:	4058                	c.lw	a4,4(s0)
    4bd4:	86f2                	c.mv	a3,t3
    4bd6:	02577163          	bgeu	a4,t0,4bf8 <lzma_main+0x604>
    4bda:	8850                	exec.it	#37     !sw	t0,0(s0)
    4bdc:	0000d303          	lhu	t1,0(ra)
    4be0:	406a82b3          	sub	t0,s5,t1
    4be4:	4052d713          	srai	a4,t0,0x5
    4be8:	00e30e33          	add	t3,t1,a4
    4bec:	01c09023          	sh	t3,0(ra)
    4bf0:	0785                	c.addi	a5,1
    4bf2:	b847ef5b          	bnec	a5,4,4b90 <lzma_main+0x59c>
    4bf6:	bd29                	c.j	4a10 <lzma_main+0x41c>
    4bf8:	40558e33          	sub	t3,a1,t0
    4bfc:	405708b3          	sub	a7,a4,t0
    4c00:	01c42023          	sw	t3,0(s0)
    4c04:	8444                	exec.it	#42     !sw	a7,4(s0)
    4c06:	0000d503          	lhu	a0,0(ra)
    4c0a:	0685                	c.addi	a3,1
    4c0c:	00555613          	srli	a2,a0,0x5
    4c10:	40c503b3          	sub	t2,a0,a2
    4c14:	00709023          	sh	t2,0(ra)
    4c18:	486c                	c.lw	a1,84(s0)
    4c1a:	00f810b3          	sll	ra,a6,a5
    4c1e:	00158f33          	add	t5,a1,ra
    4c22:	8e50                	exec.it	#103     !sw	t5,84(s0)
    4c24:	b7f1                	c.j	4bf0 <lzma_main+0x5fc>
    4c26:	40e082b3          	sub	t0,ra,a4
    4c2a:	40e807b3          	sub	a5,a6,a4
    4c2e:	005fd513          	srli	a0,t6,0x5
    4c32:	40af8e33          	sub	t3,t6,a0
    4c36:	8850                	exec.it	#37     !sw	t0,0(s0)
    4c38:	c05c                	c.sw	a5,4(s0)
    4c3a:	01000337          	lui	t1,0x1000
    4c3e:	1fcf1c23          	sh	t3,504(t5)
    4c42:	0262f563          	bgeu	t0,t1,4c6c <lzma_main+0x678>
    4c46:	480c                	c.lw	a1,16(s0)
    4c48:	8e30                	exec.it	#87     !lw	ra,12(s0)
    4c4a:	00829e93          	slli	t4,t0,0x8
    4c4e:	00158f93          	addi	t6,a1,1
    4c52:	00b088b3          	add	a7,ra,a1
    4c56:	8a40                	exec.it	#100     !sw	t4,0(s0)
    4c58:	01f42823          	sw	t6,16(s0)
    4c5c:	0008c703          	lbu	a4,0(a7)
    4c60:	00879813          	slli	a6,a5,0x8
    4c64:	010702b3          	add	t0,a4,a6
    4c68:	00542223          	sw	t0,4(s0)
    4c6c:	401c                	c.lw	a5,0(s0)
    4c6e:	210f5503          	lhu	a0,528(t5)
    4c72:	00b7de13          	srli	t3,a5,0xb
    4c76:	02ae0333          	mul	t1,t3,a0
    4c7a:	8820                	exec.it	#20     !lw	t4,4(s0)
    4c7c:	046eff63          	bgeu	t4,t1,4cda <lzma_main+0x6e6>
    4c80:	40aa80b3          	sub	ra,s5,a0
    4c84:	4050d593          	srai	a1,ra,0x5
    4c88:	00b50fb3          	add	t6,a0,a1
    4c8c:	8810                	exec.it	#5     !sw	t1,0(s0)
    4c8e:	21ff1823          	sh	t6,528(t5)
    4c92:	01000f37          	lui	t5,0x1000
    4c96:	0be36063          	bltu	t1,t5,4d36 <lzma_main+0x742>
    4c9a:	00042e83          	lw	t4,0(s0)
    4c9e:	2586d083          	lhu	ra,600(a3)
    4ca2:	00bed593          	srli	a1,t4,0xb
    4ca6:	02158fb3          	mul	t6,a1,ra
    4caa:	8454                	exec.it	#43     !lw	t5,4(s0)
    4cac:	0bff7763          	bgeu	t5,t6,4d5a <lzma_main+0x766>
    4cb0:	401a8633          	sub	a2,s5,ra
    4cb4:	40565513          	srai	a0,a2,0x5
    4cb8:	00a08833          	add	a6,ra,a0
    4cbc:	01f42023          	sw	t6,0(s0)
    4cc0:	4e19                	c.li	t3,6
    4cc2:	25069c23          	sh	a6,600(a3)
    4cc6:	4ea5                	c.li	t4,9
    4cc8:	007e7363          	bgeu	t3,t2,4cce <lzma_main+0x6da>
    4ccc:	4ead                	c.li	t4,11
    4cce:	4385                	c.li	t2,1
    4cd0:	07d42223          	sw	t4,100(s0)
    4cd4:	06742423          	sw	t2,104(s0)
    4cd8:	bb25                	c.j	4a10 <lzma_main+0x41c>
    4cda:	406786b3          	sub	a3,a5,t1
    4cde:	406e80b3          	sub	ra,t4,t1
    4ce2:	00555593          	srli	a1,a0,0x5
    4ce6:	40b50fb3          	sub	t6,a0,a1
    4cea:	c014                	c.sw	a3,0(s0)
    4cec:	00142223          	sw	ra,4(s0)
    4cf0:	010008b7          	lui	a7,0x1000
    4cf4:	21ff1823          	sh	t6,528(t5) # 1000210 <SRAM2_SIZE+0xfe5ae8>
    4cf8:	0916e963          	bltu	a3,a7,4d8a <lzma_main+0x796>
    4cfc:	4014                	c.lw	a3,0(s0)
    4cfe:	228f5083          	lhu	ra,552(t5)
    4d02:	00b6d593          	srli	a1,a3,0xb
    4d06:	02158fb3          	mul	t6,a1,ra
    4d0a:	4058                	c.lw	a4,4(s0)
    4d0c:	05842883          	lw	a7,88(s0)
    4d10:	0bf77163          	bgeu	a4,t6,4db2 <lzma_main+0x7be>
    4d14:	401a8533          	sub	a0,s5,ra
    4d18:	40555813          	srai	a6,a0,0x5
    4d1c:	01008e33          	add	t3,ra,a6
    4d20:	01f42023          	sw	t6,0(s0)
    4d24:	23cf1423          	sh	t3,552(t5)
    4d28:	05442e83          	lw	t4,84(s0)
    4d2c:	05142a23          	sw	a7,84(s0)
    4d30:	05d42c23          	sw	t4,88(s0)
    4d34:	a83d                	c.j	4d72 <lzma_main+0x77e>
    4d36:	01042283          	lw	t0,16(s0)
    4d3a:	4458                	c.lw	a4,12(s0)
    4d3c:	0322                	c.slli	t1,0x8
    4d3e:	00128793          	addi	a5,t0,1
    4d42:	005708b3          	add	a7,a4,t0
    4d46:	8810                	exec.it	#5     !sw	t1,0(s0)
    4d48:	c81c                	c.sw	a5,16(s0)
    4d4a:	8640                	exec.it	#98     !lbu	a0,0(a7) # 1000000 <SRAM2_SIZE+0xfe58d8>
    4d4c:	008e9813          	slli	a6,t4,0x8
    4d50:	01050e33          	add	t3,a0,a6
    4d54:	01c42223          	sw	t3,4(s0)
    4d58:	b789                	c.j	4c9a <lzma_main+0x6a6>
    4d5a:	41fe8333          	sub	t1,t4,t6
    4d5e:	41ff0733          	sub	a4,t5,t6
    4d62:	0050d293          	srli	t0,ra,0x5
    4d66:	405087b3          	sub	a5,ra,t0
    4d6a:	8810                	exec.it	#5     !sw	t1,0(s0)
    4d6c:	c058                	c.sw	a4,4(s0)
    4d6e:	24f69c23          	sh	a5,600(a3)
    4d72:	4699                	c.li	a3,6
    4d74:	48a1                	c.li	a7,8
    4d76:	0076f363          	bgeu	a3,t2,4d7c <lzma_main+0x788>
    4d7a:	48ad                	c.li	a7,11
    4d7c:	85ca                	c.mv	a1,s2
    4d7e:	8522                	c.mv	a0,s0
    4d80:	07142223          	sw	a7,100(s0)
    4d84:	e08ff0ef          	jal	ra,438c <lzma_len>
    4d88:	b161                	c.j	4a10 <lzma_main+0x41c>
    4d8a:	481c                	c.lw	a5,16(s0)
    4d8c:	8e40                	exec.it	#102     !lw	t0,12(s0)
    4d8e:	00869713          	slli	a4,a3,0x8
    4d92:	00178813          	addi	a6,a5,1
    4d96:	00f28533          	add	a0,t0,a5
    4d9a:	c018                	c.sw	a4,0(s0)
    4d9c:	01042823          	sw	a6,16(s0)
    4da0:	00054e03          	lbu	t3,0(a0)
    4da4:	00809313          	slli	t1,ra,0x8
    4da8:	006e0eb3          	add	t4,t3,t1
    4dac:	01d42223          	sw	t4,4(s0)
    4db0:	b7b1                	c.j	4cfc <lzma_main+0x708>
    4db2:	41f682b3          	sub	t0,a3,t6
    4db6:	41f707b3          	sub	a5,a4,t6
    4dba:	0050d813          	srli	a6,ra,0x5
    4dbe:	41008533          	sub	a0,ra,a6
    4dc2:	8850                	exec.it	#37     !sw	t0,0(s0)
    4dc4:	c05c                	c.sw	a5,4(s0)
    4dc6:	01000e37          	lui	t3,0x1000
    4dca:	22af1423          	sh	a0,552(t5)
    4dce:	03c2f463          	bgeu	t0,t3,4df6 <lzma_main+0x802>
    4dd2:	4814                	c.lw	a3,16(s0)
    4dd4:	8e14                	exec.it	#79     !lw	t4,12(s0)
    4dd6:	00829313          	slli	t1,t0,0x8
    4dda:	00168093          	addi	ra,a3,1
    4dde:	00de85b3          	add	a1,t4,a3
    4de2:	8810                	exec.it	#5     !sw	t1,0(s0)
    4de4:	00142823          	sw	ra,16(s0)
    4de8:	0005cf83          	lbu	t6,0(a1)
    4dec:	8864                	exec.it	#60     !slli	a4,a5,0x8
    4dee:	00ef82b3          	add	t0,t6,a4
    4df2:	00542223          	sw	t0,4(s0)
    4df6:	401c                	c.lw	a5,0(s0)
    4df8:	240f5503          	lhu	a0,576(t5)
    4dfc:	00b7d813          	srli	a6,a5,0xb
    4e00:	02a80e33          	mul	t3,a6,a0
    4e04:	8820                	exec.it	#20     !lw	t4,4(s0)
    4e06:	05c42303          	lw	t1,92(s0)
    4e0a:	03cef063          	bgeu	t4,t3,4e2a <lzma_main+0x836>
    4e0e:	40aa8733          	sub	a4,s5,a0
    4e12:	40575293          	srai	t0,a4,0x5
    4e16:	005507b3          	add	a5,a0,t0
    4e1a:	01c42023          	sw	t3,0(s0)
    4e1e:	24ff1023          	sh	a5,576(t5)
    4e22:	05142e23          	sw	a7,92(s0)
    4e26:	889a                	c.mv	a7,t1
    4e28:	b701                	c.j	4d28 <lzma_main+0x734>
    4e2a:	41c786b3          	sub	a3,a5,t3
    4e2e:	41ce80b3          	sub	ra,t4,t3
    4e32:	00555593          	srli	a1,a0,0x5
    4e36:	40b50fb3          	sub	t6,a0,a1
    4e3a:	c014                	c.sw	a3,0(s0)
    4e3c:	00142223          	sw	ra,4(s0)
    4e40:	25ff1023          	sh	t6,576(t5)
    4e44:	06042f03          	lw	t5,96(s0)
    4e48:	06642023          	sw	t1,96(s0)
    4e4c:	837a                	c.mv	t1,t5
    4e4e:	bfd1                	c.j	4e22 <lzma_main+0x82e>

00004e50 <xz_dec_lzma2_run>:
    4e50:	6dd002ef          	jal	t0,5d2c <__riscv_save_10>
    4e54:	6a91                	c.lui	s5,0x4
    4e56:	ee8a8a13          	addi	s4,s5,-280 # 3ee8 <ty_bsdiff_entry+0xa2>
    4e5a:	ee4a8293          	addi	t0,s5,-284
    4e5e:	015509b3          	add	s3,a0,s5
    4e62:	842a                	c.mv	s0,a0
    4e64:	84ae                	c.mv	s1,a1
    4e66:	4b21                	c.li	s6,8
    4e68:	9a2a                	c.add	s4,a0
    4e6a:	00550ab3          	add	s5,a0,t0
    4e6e:	40dc                	c.lw	a5,4(s1)
    4e70:	4494                	c.lw	a3,8(s1)
    4e72:	4038                	c.lw	a4,64(s0)
    4e74:	0cd7ef63          	bltu	a5,a3,4f52 <xz_dec_lzma2_run+0x102>
    4e78:	32776d5b          	bnec	a4,7,51b2 <xz_dec_lzma2_run+0x362>
    4e7c:	0144a303          	lw	t1,20(s1)
    4e80:	4894                	c.lw	a3,16(s1)
    4e82:	04842383          	lw	t2,72(s0)
    4e86:	40d30533          	sub	a0,t1,a3
    4e8a:	00a3f363          	bgeu	t2,a0,4e90 <xz_dec_lzma2_run+0x40>
    4e8e:	851e                	c.mv	a0,t2
    4e90:	8a54                	exec.it	#109     !lw	a6,44(s0)
    4e92:	500c                	c.lw	a1,32(s0)
    4e94:	40b80633          	sub	a2,a6,a1
    4e98:	00c57463          	bgeu	a0,a2,4ea0 <xz_dec_lzma2_run+0x50>
    4e9c:	00b50833          	add	a6,a0,a1
    4ea0:	03042423          	sw	a6,40(s0)
    4ea4:	ee49a883          	lw	a7,-284(s3) # 7ffffee4 <_stack+0x4ff9fee4>
    4ea8:	04c42e03          	lw	t3,76(s0)
    4eac:	00089463          	bnez	a7,4eb4 <xz_dec_lzma2_run+0x64>
    4eb0:	3c0e1b63          	bnez	t3,5286 <xz_dec_lzma2_run+0x436>
    4eb4:	02a00913          	li	s2,42
    4eb8:	41190eb3          	sub	t4,s2,a7
    4ebc:	411e0733          	sub	a4,t3,a7
    4ec0:	0044a083          	lw	ra,4(s1)
    4ec4:	00eef363          	bgeu	t4,a4,4eca <xz_dec_lzma2_run+0x7a>
    4ec8:	8776                	c.mv	a4,t4
    4eca:	0084af03          	lw	t5,8(s1)
    4ece:	401f0933          	sub	s2,t5,ra
    4ed2:	01277363          	bgeu	a4,s2,4ed8 <xz_dec_lzma2_run+0x88>
    4ed6:	893a                	c.mv	s2,a4
    4ed8:	0004af83          	lw	t6,0(s1)
    4edc:	864a                	c.mv	a2,s2
    4ede:	001f85b3          	add	a1,t6,ra
    4ee2:	011a0533          	add	a0,s4,a7
    4ee6:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    4ee8:	ee49a283          	lw	t0,-284(s3)
    4eec:	447c                	c.lw	a5,76(s0)
    4eee:	00590333          	add	t1,s2,t0
    4ef2:	2ef31863          	bne	t1,a5,51e2 <xz_dec_lzma2_run+0x392>
    4ef6:	03f00513          	li	a0,63
    4efa:	40650633          	sub	a2,a0,t1
    4efe:	4581                	c.li	a1,0
    4f00:	006a0533          	add	a0,s4,t1
    4f04:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    4f06:	ee49a583          	lw	a1,-284(s3)
    4f0a:	01258633          	add	a2,a1,s2
    4f0e:	c850                	c.sw	a2,20(s0)
    4f10:	8522                	c.mv	a0,s0
    4f12:	01442623          	sw	s4,12(s0)
    4f16:	00042823          	sw	zero,16(s0)
    4f1a:	edaff0ef          	jal	ra,45f4 <lzma_main>
    4f1e:	c961                	c.beqz	a0,4fee <xz_dec_lzma2_run+0x19e>
    4f20:	ee49a883          	lw	a7,-284(s3)
    4f24:	01042803          	lw	a6,16(s0)
    4f28:	9946                	c.add	s2,a7
    4f2a:	0d096263          	bltu	s2,a6,4fee <xz_dec_lzma2_run+0x19e>
    4f2e:	04c42e03          	lw	t3,76(s0)
    4f32:	410e00b3          	sub	ra,t3,a6
    4f36:	04142623          	sw	ra,76(s0)
    4f3a:	33187c63          	bgeu	a6,a7,5272 <xz_dec_lzma2_run+0x422>
    4f3e:	41088633          	sub	a2,a7,a6
    4f42:	010a05b3          	add	a1,s4,a6
    4f46:	8552                	c.mv	a0,s4
    4f48:	eec9a223          	sw	a2,-284(s3)
    4f4c:	71d000ef          	jal	ra,5e68 <memmove>
    4f50:	a465                	c.j	51f8 <xz_dec_lzma2_run+0x3a8>
    4f52:	f2eb61e3          	bltu	s6,a4,4e74 <xz_dec_lzma2_run+0x24>
    4f56:	00003317          	auipc	t1,0x3
    4f5a:	66230313          	addi	t1,t1,1634 # 85b8 <s_crc32+0x400>
    4f5e:	0ce303db          	lea.w	t2,t1,a4
    4f62:	0003a503          	lw	a0,0(t2) # 1000000 <SRAM2_SIZE+0xfe58d8>
    4f66:	006505b3          	add	a1,a0,t1
    4f6a:	8582                	c.jr	a1
    4f6c:	4090                	c.lw	a2,0(s1)
    4f6e:	00178893          	addi	a7,a5,1
    4f72:	00f60e33          	add	t3,a2,a5
    4f76:	8214                	exec.it	#73     !sw	a7,4(s1)
    4f78:	8870                	exec.it	#53     !lbu	t4,0(t3) # 1000000 <SRAM2_SIZE+0xfe58d8>
    4f7a:	480e8163          	beqz	t4,53fc <xz_dec_lzma2_run+0x5ac>
    4f7e:	0df00f13          	li	t5,223
    4f82:	01df6463          	bltu	t5,t4,4f8a <xz_dec_lzma2_run+0x13a>
    4f86:	061ee15b          	bnec	t4,1,4fe8 <xz_dec_lzma2_run+0x198>
    4f8a:	10000f93          	li	t6,256
    4f8e:	03c42283          	lw	t0,60(s0)
    4f92:	05f41823          	sh	t6,80(s0)
    4f96:	00029e63          	bnez	t0,4fb2 <xz_dec_lzma2_run+0x162>
    4f9a:	00c4a303          	lw	t1,12(s1)
    4f9e:	4894                	c.lw	a3,16(s1)
    4fa0:	00d307b3          	add	a5,t1,a3
    4fa4:	cc1c                	c.sw	a5,24(s0)
    4fa6:	0144a383          	lw	t2,20(s1)
    4faa:	488c                	c.lw	a1,16(s1)
    4fac:	40b38533          	sub	a0,t2,a1
    4fb0:	d448                	c.sw	a0,44(s0)
    4fb2:	00042e23          	sw	zero,28(s0)
    4fb6:	8c74                	exec.it	#63     !sw	zero,32(s0)
    4fb8:	02042423          	sw	zero,40(s0)
    4fbc:	02042223          	sw	zero,36(s0)
    4fc0:	07f00813          	li	a6,127
    4fc4:	07d87c63          	bgeu	a6,t4,503c <xz_dec_lzma2_run+0x1ec>
    4fc8:	414eae5b          	bfoz	t3,t4,16,20
    4fcc:	4f05                	c.li	t5,1
    4fce:	0bf00713          	li	a4,191
    4fd2:	05c42423          	sw	t3,72(s0)
    4fd6:	05e42023          	sw	t5,64(s0)
    4fda:	01d77d63          	bgeu	a4,t4,4ff4 <xz_dec_lzma2_run+0x1a4>
    4fde:	4595                	c.li	a1,5
    4fe0:	040408a3          	sb	zero,81(s0)
    4fe4:	c06c                	c.sw	a1,68(s0)
    4fe6:	b561                	c.j	4e6e <xz_dec_lzma2_run+0x1e>
    4fe8:	05044703          	lbu	a4,80(s0)
    4fec:	db71                	c.beqz	a4,4fc0 <xz_dec_lzma2_run+0x170>
    4fee:	451d                	c.li	a0,7
    4ff0:	5750006f          	j	5d64 <__riscv_restore_10>
    4ff4:	05144f83          	lbu	t6,81(s0)
    4ff8:	fe0f9be3          	bnez	t6,4fee <xz_dec_lzma2_run+0x19e>
    4ffc:	4299                	c.li	t0,6
    4ffe:	09f00313          	li	t1,159
    5002:	04542223          	sw	t0,68(s0)
    5006:	e7d374e3          	bgeu	t1,t4,4e6e <xz_dec_lzma2_run+0x1e>
    500a:	07840693          	addi	a3,s0,120
    500e:	40000e93          	li	t4,1024
    5012:	8600                	exec.it	#66     !sw	zero,100(s0)
    5014:	04042a23          	sw	zero,84(s0)
    5018:	04042c23          	sw	zero,88(s0)
    501c:	04042e23          	sw	zero,92(s0)
    5020:	06042023          	sw	zero,96(s0)
    5024:	01d69023          	sh	t4,0(a3)
    5028:	0689                	c.addi	a3,2
    502a:	feda9de3          	bne	s5,a3,5024 <xz_dec_lzma2_run+0x1d4>
    502e:	57fd                	c.li	a5,-1
    5030:	4395                	c.li	t2,5
    5032:	c01c                	c.sw	a5,0(s0)
    5034:	8804                	exec.it	#12     !sw	zero,4(s0)
    5036:	00742423          	sw	t2,8(s0)
    503a:	bd15                	c.j	4e6e <xz_dec_lzma2_run+0x1e>
    503c:	4609                	c.li	a2,2
    503e:	fbd668e3          	bltu	a2,t4,4fee <xz_dec_lzma2_run+0x19e>
    5042:	488d                	c.li	a7,3
    5044:	05142023          	sw	a7,64(s0)
    5048:	05642223          	sw	s6,68(s0)
    504c:	b50d                	c.j	4e6e <xz_dec_lzma2_run+0x1e>
    504e:	0004a303          	lw	t1,0(s1)
    5052:	00178693          	addi	a3,a5,1
    5056:	979a                	c.add	a5,t1
    5058:	c0d4                	c.sw	a3,4(s1)
    505a:	0007c383          	lbu	t2,0(a5)
    505e:	4428                	c.lw	a0,72(s0)
    5060:	00839593          	slli	a1,t2,0x8
    5064:	00b50833          	add	a6,a0,a1
    5068:	4789                	c.li	a5,2
    506a:	05042423          	sw	a6,72(s0)
    506e:	c03c                	c.sw	a5,64(s0)
    5070:	bbfd                	c.j	4e6e <xz_dec_lzma2_run+0x1e>
    5072:	0004a883          	lw	a7,0(s1)
    5076:	00178e13          	addi	t3,a5,1
    507a:	01c4a223          	sw	t3,4(s1)
    507e:	00f88eb3          	add	t4,a7,a5
    5082:	4438                	c.lw	a4,72(s0)
    5084:	000ecf03          	lbu	t5,0(t4) # 1000000 <SRAM2_SIZE+0xfe58d8>
    5088:	00170f93          	addi	t6,a4,1
    508c:	01ff02b3          	add	t0,t5,t6
    5090:	478d                	c.li	a5,3
    5092:	04542423          	sw	t0,72(s0)
    5096:	bfe1                	c.j	506e <xz_dec_lzma2_run+0x21e>
    5098:	8604                	exec.it	#74     !lw	t2,0(s1)
    509a:	00178593          	addi	a1,a5,1
    509e:	00f38533          	add	a0,t2,a5
    50a2:	c0cc                	c.sw	a1,4(s1)
    50a4:	00054803          	lbu	a6,0(a0)
    50a8:	4791                	c.li	a5,4
    50aa:	00881613          	slli	a2,a6,0x8
    50ae:	c470                	c.sw	a2,76(s0)
    50b0:	bf7d                	c.j	506e <xz_dec_lzma2_run+0x21e>
    50b2:	8230                	exec.it	#81     !lw	t4,0(s1)
    50b4:	00178713          	addi	a4,a5,1
    50b8:	c0d8                	c.sw	a4,4(s1)
    50ba:	00fe8f33          	add	t5,t4,a5
    50be:	04c42283          	lw	t0,76(s0)
    50c2:	000f4f83          	lbu	t6,0(t5)
    50c6:	8254                	exec.it	#105     !addi	t1,t0,1
    50c8:	006f86b3          	add	a3,t6,t1
    50cc:	407c                	c.lw	a5,68(s0)
    50ce:	c474                	c.sw	a3,76(s0)
    50d0:	bf79                	c.j	506e <xz_dec_lzma2_run+0x21e>
    50d2:	4090                	c.lw	a2,0(s1)
    50d4:	00178813          	addi	a6,a5,1
    50d8:	00f608b3          	add	a7,a2,a5
    50dc:	0104a223          	sw	a6,4(s1)
    50e0:	8640                	exec.it	#98     !lbu	a0,0(a7)
    50e2:	0e000e13          	li	t3,224
    50e6:	f0ae64e3          	bltu	t3,a0,4fee <xz_dec_lzma2_run+0x19e>
    50ea:	02c00613          	li	a2,44
    50ee:	06042a23          	sw	zero,116(s0)
    50f2:	07442e83          	lw	t4,116(s0)
    50f6:	08a66663          	bltu	a2,a0,5182 <xz_dec_lzma2_run+0x332>
    50fa:	4f05                	c.li	t5,1
    50fc:	01df1fb3          	sll	t6,t5,t4
    5100:	ffff8293          	addi	t0,t6,-1 # ffffff <SRAM2_SIZE+0xfe58d7>
    5104:	06542a23          	sw	t0,116(s0)
    5108:	06042823          	sw	zero,112(s0)
    510c:	5838                	c.lw	a4,112(s0)
    510e:	08ab6363          	bltu	s6,a0,5194 <xz_dec_lzma2_run+0x344>
    5112:	00e507b3          	add	a5,a0,a4
    5116:	4691                	c.li	a3,4
    5118:	d468                	c.sw	a0,108(s0)
    511a:	ecf6eae3          	bltu	a3,a5,4fee <xz_dec_lzma2_run+0x19e>
    511e:	4305                	c.li	t1,1
    5120:	00e313b3          	sll	t2,t1,a4
    5124:	fff38513          	addi	a0,t2,-1
    5128:	07840613          	addi	a2,s0,120
    512c:	40000593          	li	a1,1024
    5130:	d828                	c.sw	a0,112(s0)
    5132:	8600                	exec.it	#66     !sw	zero,100(s0)
    5134:	04042a23          	sw	zero,84(s0)
    5138:	04042c23          	sw	zero,88(s0)
    513c:	04042e23          	sw	zero,92(s0)
    5140:	06042023          	sw	zero,96(s0)
    5144:	00b61023          	sh	a1,0(a2)
    5148:	0609                	c.addi	a2,2
    514a:	feca9de3          	bne	s5,a2,5144 <xz_dec_lzma2_run+0x2f4>
    514e:	587d                	c.li	a6,-1
    5150:	4895                	c.li	a7,5
    5152:	4e19                	c.li	t3,6
    5154:	8e34                	exec.it	#95     !sw	a6,0(s0)
    5156:	8804                	exec.it	#12     !sw	zero,4(s0)
    5158:	01142423          	sw	a7,8(s0)
    515c:	05c42023          	sw	t3,64(s0)
    5160:	04c42e83          	lw	t4,76(s0)
    5164:	4f11                	c.li	t5,4
    5166:	e9df74e3          	bgeu	t5,t4,4fee <xz_dec_lzma2_run+0x19e>
    516a:	00842f83          	lw	t6,8(s0)
    516e:	020f9c63          	bnez	t6,51a6 <xz_dec_lzma2_run+0x356>
    5172:	04c42283          	lw	t0,76(s0)
    5176:	479d                	c.li	a5,7
    5178:	ffb28713          	addi	a4,t0,-5
    517c:	c478                	c.sw	a4,76(s0)
    517e:	c03c                	c.sw	a5,64(s0)
    5180:	b9f5                	c.j	4e7c <xz_dec_lzma2_run+0x2c>
    5182:	fd350893          	addi	a7,a0,-45
    5186:	001e8e13          	addi	t3,t4,1
    518a:	0ff8f513          	andi	a0,a7,255
    518e:	07c42a23          	sw	t3,116(s0)
    5192:	b785                	c.j	50f2 <xz_dec_lzma2_run+0x2a2>
    5194:	ff750593          	addi	a1,a0,-9
    5198:	00170813          	addi	a6,a4,1
    519c:	0ff5f513          	andi	a0,a1,255
    51a0:	07042823          	sw	a6,112(s0)
    51a4:	b7a5                	c.j	510c <xz_dec_lzma2_run+0x2bc>
    51a6:	0044a883          	lw	a7,4(s1)
    51aa:	0084ae03          	lw	t3,8(s1)
    51ae:	01c89463          	bne	a7,t3,51b6 <xz_dec_lzma2_run+0x366>
    51b2:	4501                	c.li	a0,0
    51b4:	bd35                	c.j	4ff0 <xz_dec_lzma2_run+0x1a0>
    51b6:	0004af03          	lw	t5,0(s1)
    51ba:	8820                	exec.it	#20     !lw	t4,4(s0)
    51bc:	00188f93          	addi	t6,a7,1
    51c0:	011f02b3          	add	t0,t5,a7
    51c4:	01f4a223          	sw	t6,4(s1)
    51c8:	0002c303          	lbu	t1,0(t0)
    51cc:	441c                	c.lw	a5,8(s0)
    51ce:	008e9713          	slli	a4,t4,0x8
    51d2:	00e306b3          	add	a3,t1,a4
    51d6:	fff78393          	addi	t2,a5,-1
    51da:	c054                	c.sw	a3,4(s0)
    51dc:	00742423          	sw	t2,8(s0)
    51e0:	b769                	c.j	516a <xz_dec_lzma2_run+0x31a>
    51e2:	46d1                	c.li	a3,20
    51e4:	0866e263          	bltu	a3,t1,5268 <xz_dec_lzma2_run+0x418>
    51e8:	ee69a223          	sw	t1,-284(s3)
    51ec:	0044ab83          	lw	s7,4(s1)
    51f0:	012b8c33          	add	s8,s7,s2
    51f4:	0184a223          	sw	s8,4(s1)
    51f8:	02042b83          	lw	s7,32(s0)
    51fc:	01c42903          	lw	s2,28(s0)
    5200:	5c54                	c.lw	a3,60(s0)
    5202:	412b8c33          	sub	s8,s7,s2
    5206:	ce91                	c.beqz	a3,5222 <xz_dec_lzma2_run+0x3d2>
    5208:	02c42383          	lw	t2,44(s0)
    520c:	007b9363          	bne	s7,t2,5212 <xz_dec_lzma2_run+0x3c2>
    5210:	8c74                	exec.it	#63     !sw	zero,32(s0)
    5212:	4c0c                	c.lw	a1,24(s0)
    5214:	44c8                	c.lw	a0,12(s1)
    5216:	0104a803          	lw	a6,16(s1)
    521a:	8662                	c.mv	a2,s8
    521c:	95ca                	c.add	a1,s2
    521e:	9542                	c.add	a0,a6
    5220:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    5222:	5010                	c.lw	a2,32(s0)
    5224:	cc50                	c.sw	a2,28(s0)
    5226:	0104a883          	lw	a7,16(s1)
    522a:	01888e33          	add	t3,a7,s8
    522e:	01c4a823          	sw	t3,16(s1)
    5232:	04842e83          	lw	t4,72(s0)
    5236:	01d90733          	add	a4,s2,t4
    523a:	41770933          	sub	s2,a4,s7
    523e:	05242423          	sw	s2,72(s0)
    5242:	0c091f63          	bnez	s2,5320 <xz_dec_lzma2_run+0x4d0>
    5246:	04c42383          	lw	t2,76(s0)
    524a:	da0392e3          	bnez	t2,4fee <xz_dec_lzma2_run+0x19e>
    524e:	542c                	c.lw	a1,104(s0)
    5250:	d8059fe3          	bnez	a1,4fee <xz_dec_lzma2_run+0x19e>
    5254:	4048                	c.lw	a0,4(s0)
    5256:	d8051ce3          	bnez	a0,4fee <xz_dec_lzma2_run+0x19e>
    525a:	587d                	c.li	a6,-1
    525c:	4615                	c.li	a2,5
    525e:	8e34                	exec.it	#95     !sw	a6,0(s0)
    5260:	c410                	c.sw	a2,8(s0)
    5262:	04042023          	sw	zero,64(s0)
    5266:	b121                	c.j	4e6e <xz_dec_lzma2_run+0x1e>
    5268:	feb30393          	addi	t2,t1,-21
    526c:	00742a23          	sw	t2,20(s0)
    5270:	b145                	c.j	4f10 <xz_dec_lzma2_run+0xc0>
    5272:	0044ae83          	lw	t4,4(s1)
    5276:	411e8733          	sub	a4,t4,a7
    527a:	01070f33          	add	t5,a4,a6
    527e:	01e4a223          	sw	t5,4(s1)
    5282:	ee09a223          	sw	zero,-284(s3)
    5286:	0084af83          	lw	t6,8(s1)
    528a:	0044a283          	lw	t0,4(s1)
    528e:	47d1                	c.li	a5,20
    5290:	405f8333          	sub	t1,t6,t0
    5294:	0467fa63          	bgeu	a5,t1,52e8 <xz_dec_lzma2_run+0x498>
    5298:	4094                	c.lw	a3,0(s1)
    529a:	4468                	c.lw	a0,76(s0)
    529c:	c454                	c.sw	a3,12(s0)
    529e:	0044a383          	lw	t2,4(s1)
    52a2:	01550593          	addi	a1,a0,21
    52a6:	00742823          	sw	t2,16(s0)
    52aa:	00a38833          	add	a6,t2,a0
    52ae:	00b37563          	bgeu	t1,a1,52b8 <xz_dec_lzma2_run+0x468>
    52b2:	4490                	c.lw	a2,8(s1)
    52b4:	feb60813          	addi	a6,a2,-21
    52b8:	8522                	c.mv	a0,s0
    52ba:	01042a23          	sw	a6,20(s0)
    52be:	b36ff0ef          	jal	ra,45f4 <lzma_main>
    52c2:	d20506e3          	beqz	a0,4fee <xz_dec_lzma2_run+0x19e>
    52c6:	01042883          	lw	a7,16(s0)
    52ca:	0044ae03          	lw	t3,4(s1)
    52ce:	04c42083          	lw	ra,76(s0)
    52d2:	41c88eb3          	sub	t4,a7,t3
    52d6:	d1d0ece3          	bltu	ra,t4,4fee <xz_dec_lzma2_run+0x19e>
    52da:	411e0933          	sub	s2,t3,a7
    52de:	00190bb3          	add	s7,s2,ra
    52e2:	05742623          	sw	s7,76(s0)
    52e6:	8214                	exec.it	#73     !sw	a7,4(s1)
    52e8:	0044ac03          	lw	s8,4(s1)
    52ec:	4498                	c.lw	a4,8(s1)
    52ee:	4fd1                	c.li	t6,20
    52f0:	41870f33          	sub	t5,a4,s8
    52f4:	f1efe2e3          	bltu	t6,t5,51f8 <xz_dec_lzma2_run+0x3a8>
    52f8:	04c42903          	lw	s2,76(s0)
    52fc:	012f7363          	bgeu	t5,s2,5302 <xz_dec_lzma2_run+0x4b2>
    5300:	897a                	c.mv	s2,t5
    5302:	0004a283          	lw	t0,0(s1)
    5306:	864a                	c.mv	a2,s2
    5308:	018285b3          	add	a1,t0,s8
    530c:	8552                	c.mv	a0,s4
    530e:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    5310:	ef29a223          	sw	s2,-284(s3)
    5314:	0044a303          	lw	t1,4(s1)
    5318:	012307b3          	add	a5,t1,s2
    531c:	c0dc                	c.sw	a5,4(s1)
    531e:	bde9                	c.j	51f8 <xz_dec_lzma2_run+0x3a8>
    5320:	0104af03          	lw	t5,16(s1)
    5324:	0144af83          	lw	t6,20(s1)
    5328:	e9ff05e3          	beq	t5,t6,51b2 <xz_dec_lzma2_run+0x362>
    532c:	0044a283          	lw	t0,4(s1)
    5330:	0084a303          	lw	t1,8(s1)
    5334:	b2629de3          	bne	t0,t1,4e6e <xz_dec_lzma2_run+0x1e>
    5338:	ee49a683          	lw	a3,-284(s3)
    533c:	447c                	c.lw	a5,76(s0)
    533e:	b2f6f8e3          	bgeu	a3,a5,4e6e <xz_dec_lzma2_run+0x1e>
    5342:	bd85                	c.j	51b2 <xz_dec_lzma2_run+0x362>
    5344:	02042083          	lw	ra,32(s0)
    5348:	02c42903          	lw	s2,44(s0)
    534c:	40190933          	sub	s2,s2,ra
    5350:	01287363          	bgeu	a6,s2,5356 <xz_dec_lzma2_run+0x506>
    5354:	8942                	c.mv	s2,a6
    5356:	40cf0533          	sub	a0,t5,a2
    535a:	01257363          	bgeu	a0,s2,5360 <xz_dec_lzma2_run+0x510>
    535e:	892a                	c.mv	s2,a0
    5360:	411e0733          	sub	a4,t3,a7
    5364:	01277363          	bgeu	a4,s2,536a <xz_dec_lzma2_run+0x51a>
    5368:	893a                	c.mv	s2,a4
    536a:	41280fb3          	sub	t6,a6,s2
    536e:	05f42623          	sw	t6,76(s0)
    5372:	01842e83          	lw	t4,24(s0)
    5376:	0004a283          	lw	t0,0(s1)
    537a:	0044a303          	lw	t1,4(s1)
    537e:	864a                	c.mv	a2,s2
    5380:	006285b3          	add	a1,t0,t1
    5384:	001e8533          	add	a0,t4,ra
    5388:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    538a:	5014                	c.lw	a3,32(s0)
    538c:	02442383          	lw	t2,36(s0)
    5390:	00d907b3          	add	a5,s2,a3
    5394:	d01c                	c.sw	a5,32(s0)
    5396:	00f3f363          	bgeu	t2,a5,539c <xz_dec_lzma2_run+0x54c>
    539a:	d05c                	c.sw	a5,36(s0)
    539c:	5c4c                	c.lw	a1,60(s0)
    539e:	c19d                	c.beqz	a1,53c4 <xz_dec_lzma2_run+0x574>
    53a0:	8a54                	exec.it	#109     !lw	a6,44(s0)
    53a2:	01079363          	bne	a5,a6,53a8 <xz_dec_lzma2_run+0x558>
    53a6:	8c74                	exec.it	#63     !sw	zero,32(s0)
    53a8:	0004a883          	lw	a7,0(s1)
    53ac:	0044ae03          	lw	t3,4(s1)
    53b0:	00c4af03          	lw	t5,12(s1)
    53b4:	0104a083          	lw	ra,16(s1)
    53b8:	864a                	c.mv	a2,s2
    53ba:	01c885b3          	add	a1,a7,t3
    53be:	001f0533          	add	a0,t5,ra
    53c2:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    53c4:	5010                	c.lw	a2,32(s0)
    53c6:	cc50                	c.sw	a2,28(s0)
    53c8:	4888                	c.lw	a0,16(s1)
    53ca:	0044af83          	lw	t6,4(s1)
    53ce:	01250733          	add	a4,a0,s2
    53d2:	012f82b3          	add	t0,t6,s2
    53d6:	c898                	c.sw	a4,16(s1)
    53d8:	0054a223          	sw	t0,4(s1)
    53dc:	04c42803          	lw	a6,76(s0)
    53e0:	e80801e3          	beqz	a6,5262 <xz_dec_lzma2_run+0x412>
    53e4:	0044a883          	lw	a7,4(s1)
    53e8:	0084ae03          	lw	t3,8(s1)
    53ec:	ddc8f3e3          	bgeu	a7,t3,51b2 <xz_dec_lzma2_run+0x362>
    53f0:	4890                	c.lw	a2,16(s1)
    53f2:	0144af03          	lw	t5,20(s1)
    53f6:	f5e667e3          	bltu	a2,t5,5344 <xz_dec_lzma2_run+0x4f4>
    53fa:	bb65                	c.j	51b2 <xz_dec_lzma2_run+0x362>
    53fc:	4505                	c.li	a0,1
    53fe:	becd                	c.j	4ff0 <xz_dec_lzma2_run+0x1a0>

00005400 <xz_dec_lzma2_create>:
    5400:	155002ef          	jal	t0,5d54 <__riscv_save_0>
    5404:	84aa                	c.mv	s1,a0
    5406:	6511                	c.lui	a0,0x4
    5408:	f2850513          	addi	a0,a0,-216 # 3f28 <prvInsertBlockIntoFreeList+0x2c>
    540c:	892e                	c.mv	s2,a1
    540e:	8c54                	exec.it	#47     !jal	ra,5d84 <malloc>
    5410:	842a                	c.mv	s0,a0
    5412:	cd09                	c.beqz	a0,542c <xz_dec_lzma2_create+0x2c>
    5414:	dd44                	c.sw	s1,60(a0)
    5416:	03252a23          	sw	s2,52(a0)
    541a:	0014ec5b          	bnec	s1,1,5432 <xz_dec_lzma2_create+0x32>
    541e:	854a                	c.mv	a0,s2
    5420:	8c54                	exec.it	#47     !jal	ra,5d84 <malloc>
    5422:	cc08                	c.sw	a0,24(s0)
    5424:	e501                	c.bnez	a0,542c <xz_dec_lzma2_create+0x2c>
    5426:	8522                	c.mv	a0,s0
    5428:	4401                	c.li	s0,0
    542a:	8834                	exec.it	#29     !jal	ra,5d8a <free>
    542c:	8522                	c.mv	a0,s0
    542e:	14b0006f          	j	5d78 <__riscv_restore_0>
    5432:	be24ed5b          	bnec	s1,2,542c <xz_dec_lzma2_create+0x2c>
    5436:	00052c23          	sw	zero,24(a0)
    543a:	02052c23          	sw	zero,56(a0)
    543e:	b7fd                	c.j	542c <xz_dec_lzma2_create+0x2c>

00005440 <xz_dec_lzma2_reset>:
    5440:	02700793          	li	a5,39
    5444:	06b7e363          	bltu	a5,a1,54aa <xz_dec_lzma2_reset+0x6a>
    5448:	10d002ef          	jal	t0,5d54 <__riscv_save_0>
    544c:	0015f293          	andi	t0,a1,1
    5450:	8185                	c.srli	a1,0x1
    5452:	00228313          	addi	t1,t0,2
    5456:	00b58393          	addi	t2,a1,11
    545a:	00731633          	sll	a2,t1,t2
    545e:	5d54                	c.lw	a3,60(a0)
    5460:	d910                	c.sw	a2,48(a0)
    5462:	842a                	c.mv	s0,a0
    5464:	c695                	c.beqz	a3,5490 <xz_dec_lzma2_reset+0x50>
    5466:	5958                	c.lw	a4,52(a0)
    5468:	4511                	c.li	a0,4
    546a:	02c76163          	bltu	a4,a2,548c <xz_dec_lzma2_reset+0x4c>
    546e:	d450                	c.sw	a2,44(s0)
    5470:	0226e05b          	bnec	a3,2,5490 <xz_dec_lzma2_reset+0x50>
    5474:	5c08                	c.lw	a0,56(s0)
    5476:	00c57d63          	bgeu	a0,a2,5490 <xz_dec_lzma2_reset+0x50>
    547a:	4c08                	c.lw	a0,24(s0)
    547c:	8834                	exec.it	#29     !jal	ra,5d8a <free>
    547e:	5808                	c.lw	a0,48(s0)
    5480:	8c54                	exec.it	#47     !jal	ra,5d84 <malloc>
    5482:	cc08                	c.sw	a0,24(s0)
    5484:	e511                	c.bnez	a0,5490 <xz_dec_lzma2_reset+0x50>
    5486:	450d                	c.li	a0,3
    5488:	02042c23          	sw	zero,56(s0)
    548c:	0ed0006f          	j	5d78 <__riscv_restore_0>
    5490:	4805                	c.li	a6,1
    5492:	6891                	c.lui	a7,0x4
    5494:	06042423          	sw	zero,104(s0)
    5498:	04042023          	sw	zero,64(s0)
    549c:	05040823          	sb	a6,80(s0)
    54a0:	9446                	c.add	s0,a7
    54a2:	4501                	c.li	a0,0
    54a4:	ee042223          	sw	zero,-284(s0)
    54a8:	b7d5                	c.j	548c <xz_dec_lzma2_reset+0x4c>
    54aa:	4519                	c.li	a0,6
    54ac:	8082                	c.jr	ra

000054ae <xz_dec_lzma2_end>:
    54ae:	0a7002ef          	jal	t0,5d54 <__riscv_save_0>
    54b2:	5d5c                	c.lw	a5,60(a0)
    54b4:	842a                	c.mv	s0,a0
    54b6:	c399                	c.beqz	a5,54bc <xz_dec_lzma2_end+0xe>
    54b8:	4d08                	c.lw	a0,24(a0)
    54ba:	8834                	exec.it	#29     !jal	ra,5d8a <free>
    54bc:	8522                	c.mv	a0,s0
    54be:	8834                	exec.it	#29     !jal	ra,5d8a <free>
    54c0:	0b90006f          	j	5d78 <__riscv_restore_0>

000054c4 <get_unaligned_le32>:
    54c4:	4108                	c.lw	a0,0(a0)
    54c6:	8082                	c.jr	ra

000054c8 <dec_vli>:
    54c8:	073002ef          	jal	t0,5d3a <__riscv_save_4>
    54cc:	415c                	c.lw	a5,4(a0)
    54ce:	842a                	c.mv	s0,a0
    54d0:	8a2e                	c.mv	s4,a1
    54d2:	8932                	c.mv	s2,a2
    54d4:	89b6                	c.mv	s3,a3
    54d6:	e789                	c.bnez	a5,54e0 <dec_vli+0x18>
    54d8:	4681                	c.li	a3,0
    54da:	4701                	c.li	a4,0
    54dc:	c514                	c.sw	a3,8(a0)
    54de:	c558                	c.sw	a4,12(a0)
    54e0:	00092283          	lw	t0,0(s2)
    54e4:	0132e463          	bltu	t0,s3,54ec <dec_vli+0x24>
    54e8:	4501                	c.li	a0,0
    54ea:	a835                	c.j	5526 <dec_vli+0x5e>
    54ec:	005a00b3          	add	ra,s4,t0
    54f0:	8254                	exec.it	#105     !addi	t1,t0,1
    54f2:	0000ca83          	lbu	s5,0(ra)
    54f6:	00692023          	sw	t1,0(s2)
    54fa:	4044                	c.lw	s1,4(s0)
    54fc:	07faf513          	andi	a0,s5,127
    5500:	8626                	c.mv	a2,s1
    5502:	4581                	c.li	a1,0
    5504:	8844                	exec.it	#44     !jal	ra,5cf4 <__ashldi3>
    5506:	00842383          	lw	t2,8(s0)
    550a:	4450                	c.lw	a2,12(s0)
    550c:	00a3e533          	or	a0,t2,a0
    5510:	8dd1                	c.or	a1,a2
    5512:	1c0ab85b          	bfos	a6,s5,7,0
    5516:	c408                	c.sw	a0,8(s0)
    5518:	c44c                	c.sw	a1,12(s0)
    551a:	00084b63          	bltz	a6,5530 <dec_vli+0x68>
    551e:	000a9663          	bnez	s5,552a <dec_vli+0x62>
    5522:	c481                	c.beqz	s1,552a <dec_vli+0x62>
    5524:	451d                	c.li	a0,7
    5526:	0490006f          	j	5d6e <__riscv_restore_4>
    552a:	4505                	c.li	a0,1
    552c:	8804                	exec.it	#12     !sw	zero,4(s0)
    552e:	bfe5                	c.j	5526 <dec_vli+0x5e>
    5530:	049d                	c.addi	s1,7
    5532:	c044                	c.sw	s1,4(s0)
    5534:	bbf4e6db          	bnec	s1,63,54e0 <dec_vli+0x18>
    5538:	b7f5                	c.j	5524 <dec_vli+0x5c>

0000553a <index_update>:
    553a:	01b002ef          	jal	t0,5d54 <__riscv_save_0>
    553e:	4914                	c.lw	a3,16(a0)
    5540:	41dc                	c.lw	a5,4(a1)
    5542:	07852083          	lw	ra,120(a0)
    5546:	872e                	c.mv	a4,a1
    5548:	40d785b3          	sub	a1,a5,a3
    554c:	842a                	c.mv	s0,a0
    554e:	00b08633          	add	a2,ra,a1
    5552:	5d68                	c.lw	a0,124(a0)
    5554:	001632b3          	sltu	t0,a2,ra
    5558:	00a28333          	add	t1,t0,a0
    555c:	dc30                	c.sw	a2,120(s0)
    555e:	06642e23          	sw	t1,124(s0)
    5562:	00072383          	lw	t2,0(a4)
    5566:	4c10                	c.lw	a2,24(s0)
    5568:	00d38533          	add	a0,t2,a3
    556c:	8814                	exec.it	#13     !jal	ra,4356 <xz_crc32>
    556e:	cc08                	c.sw	a0,24(s0)
    5570:	0090006f          	j	5d78 <__riscv_restore_0>

00005574 <fill_temp>:
    5574:	7e0002ef          	jal	t0,5d54 <__riscv_save_0>
    5578:	0a052703          	lw	a4,160(a0)
    557c:	41d4                	c.lw	a3,4(a1)
    557e:	0a452403          	lw	s0,164(a0)
    5582:	459c                	c.lw	a5,8(a1)
    5584:	8c19                	c.sub	s0,a4
    5586:	40d780b3          	sub	ra,a5,a3
    558a:	84aa                	c.mv	s1,a0
    558c:	892e                	c.mv	s2,a1
    558e:	0080f363          	bgeu	ra,s0,5594 <fill_temp+0x20>
    5592:	8406                	c.mv	s0,ra
    5594:	8054                	exec.it	#41     !lw	a1,0(s2)
    5596:	0a848513          	addi	a0,s1,168
    559a:	8622                	c.mv	a2,s0
    559c:	95b6                	c.add	a1,a3
    559e:	953a                	c.add	a0,a4
    55a0:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    55a2:	00492283          	lw	t0,4(s2)
    55a6:	4501                	c.li	a0,0
    55a8:	00828333          	add	t1,t0,s0
    55ac:	00692223          	sw	t1,4(s2)
    55b0:	0a04a603          	lw	a2,160(s1)
    55b4:	0a44a383          	lw	t2,164(s1)
    55b8:	9432                	c.add	s0,a2
    55ba:	0a84a023          	sw	s0,160(s1)
    55be:	00741563          	bne	s0,t2,55c8 <fill_temp+0x54>
    55c2:	4505                	c.li	a0,1
    55c4:	0a04a023          	sw	zero,160(s1)
    55c8:	7b00006f          	j	5d78 <__riscv_restore_0>

000055cc <xz_dec_reset>:
    55cc:	788002ef          	jal	t0,5d54 <__riscv_save_0>
    55d0:	842a                	c.mv	s0,a0
    55d2:	03000613          	li	a2,48
    55d6:	4581                	c.li	a1,0
    55d8:	00052023          	sw	zero,0(a0)
    55dc:	02050223          	sb	zero,36(a0)
    55e0:	00052223          	sw	zero,4(a0)
    55e4:	00052c23          	sw	zero,24(a0)
    55e8:	04050513          	addi	a0,a0,64
    55ec:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    55ee:	03000613          	li	a2,48
    55f2:	4581                	c.li	a1,0
    55f4:	07040513          	addi	a0,s0,112
    55f8:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    55fa:	47b1                	c.li	a5,12
    55fc:	8634                	exec.it	#91     !sw	zero,160(s0)
    55fe:	0af42223          	sw	a5,164(s0)
    5602:	7760006f          	j	5d78 <__riscv_restore_0>

00005606 <xz_dec_run>:
    5606:	8024                	exec.it	#24     !jal	t0,5d24 <__riscv_save_12>
    5608:	5118                	c.lw	a4,32(a0)
    560a:	1141                	c.addi	sp,-16
    560c:	842a                	c.mv	s0,a0
    560e:	8cae                	c.mv	s9,a1
    5610:	e311                	c.bnez	a4,5614 <xz_dec_run+0xe>
    5612:	3f6d                	c.jal	55cc <xz_dec_reset>
    5614:	004ca903          	lw	s2,4(s9)
    5618:	00003a17          	auipc	s4,0x3
    561c:	fd0a0a13          	addi	s4,s4,-48 # 85e8 <s_crc32+0x430>
    5620:	05840d13          	addi	s10,s0,88
    5624:	0a840493          	addi	s1,s0,168
    5628:	5b7d                	c.li	s6,-1
    562a:	5bfd                	c.li	s7,-1
    562c:	0a040a93          	addi	s5,s0,160
    5630:	0ae40d93          	addi	s11,s0,174
    5634:	010ca983          	lw	s3,16(s9)
    5638:	01242823          	sw	s2,16(s0)
    563c:	00042283          	lw	t0,0(s0)
    5640:	46a5                	c.li	a3,9
    5642:	0056e063          	bltu	a3,t0,5642 <xz_dec_run+0x3c>
    5646:	0c5a035b          	lea.w	t1,s4,t0
    564a:	8430                	exec.it	#19     !lw	t2,0(t1)
    564c:	01438533          	add	a0,t2,s4
    5650:	8502                	c.jr	a0
    5652:	004c8a13          	addi	s4,s9,4
    5656:	4a89                	c.li	s5,2
    5658:	08840b13          	addi	s6,s0,136
    565c:	008ca683          	lw	a3,8(s9)
    5660:	000ca583          	lw	a1,0(s9)
    5664:	8652                	c.mv	a2,s4
    5666:	8522                	c.mv	a0,s0
    5668:	3585                	c.jal	54c8 <dec_vli>
    566a:	8c2a                	c.mv	s8,a0
    566c:	0015645b          	bnec	a0,1,5674 <xz_dec_run+0x6e>
    5670:	4120006f          	j	5a82 <xz_dec_run+0x47c>
    5674:	85e6                	c.mv	a1,s9
    5676:	8522                	c.mv	a0,s0
    5678:	35c9                	c.jal	553a <index_update>
    567a:	a83d                	c.j	56b8 <xz_dec_run+0xb2>
    567c:	85e6                	c.mv	a1,s9
    567e:	8522                	c.mv	a0,s0
    5680:	3dd5                	c.jal	5574 <fill_temp>
    5682:	e119                	c.bnez	a0,5688 <xz_dec_run+0x82>
    5684:	4c01                	c.li	s8,0
    5686:	a80d                	c.j	56b8 <xz_dec_run+0xb2>
    5688:	4c05                	c.li	s8,1
    568a:	4619                	c.li	a2,6
    568c:	00003597          	auipc	a1,0x3
    5690:	f5058593          	addi	a1,a1,-176 # 85dc <s_crc32+0x424>
    5694:	8526                	c.mv	a0,s1
    5696:	01842023          	sw	s8,0(s0)
    569a:	8240                	exec.it	#96     !jal	ra,5d90 <memcmp>
    569c:	5c051563          	bnez	a0,5c66 <xz_dec_run+0x660>
    56a0:	4601                	c.li	a2,0
    56a2:	4589                	c.li	a1,2
    56a4:	856e                	c.mv	a0,s11
    56a6:	8814                	exec.it	#13     !jal	ra,4356 <xz_crc32>
    56a8:	c62a                	c.swsp	a0,12(sp)
    56aa:	0b040513          	addi	a0,s0,176
    56ae:	3d19                	c.jal	54c4 <get_unaligned_le32>
    56b0:	4fb2                	c.lwsp	t6,12(sp)
    56b2:	02af8663          	beq	t6,a0,56de <xz_dec_run+0xd8>
    56b6:	4c1d                	c.li	s8,7
    56b8:	02042e03          	lw	t3,32(s0)
    56bc:	5a0e1e63          	bnez	t3,5c78 <xz_dec_run+0x672>
    56c0:	5a0c1563          	bnez	s8,5c6a <xz_dec_run+0x664>
    56c4:	004caf83          	lw	t6,4(s9)
    56c8:	008ca283          	lw	t0,8(s9)
    56cc:	4c1d                	c.li	s8,7
    56ce:	005f8363          	beq	t6,t0,56d4 <xz_dec_run+0xce>
    56d2:	4c21                	c.li	s8,8
    56d4:	012ca223          	sw	s2,4(s9)
    56d8:	013ca823          	sw	s3,16(s9)
    56dc:	ab59                	c.j	5c72 <xz_dec_run+0x66c>
    56de:	0ae44083          	lbu	ra,174(s0)
    56e2:	00008463          	beqz	ra,56ea <xz_dec_run+0xe4>
    56e6:	4c19                	c.li	s8,6
    56e8:	bfc1                	c.j	56b8 <xz_dec_run+0xb2>
    56ea:	0af44283          	lbu	t0,175(s0)
    56ee:	00542e23          	sw	t0,28(s0)
    56f2:	fe5c6ae3          	bltu	s8,t0,56e6 <xz_dec_run+0xe0>
    56f6:	004ca303          	lw	t1,4(s9)
    56fa:	008ca703          	lw	a4,8(s9)
    56fe:	f8e303e3          	beq	t1,a4,5684 <xz_dec_run+0x7e>
    5702:	000ca683          	lw	a3,0(s9)
    5706:	006683b3          	add	t2,a3,t1
    570a:	0003c503          	lbu	a0,0(t2)
    570e:	e911                	c.bnez	a0,5722 <xz_dec_run+0x11c>
    5710:	00130f93          	addi	t6,t1,1
    5714:	01fca223          	sw	t6,4(s9)
    5718:	4299                	c.li	t0,6
    571a:	00642823          	sw	t1,16(s0)
    571e:	8850                	exec.it	#37     !sw	t0,0(s0)
    5720:	bf31                	c.j	563c <xz_dec_run+0x36>
    5722:	00150793          	addi	a5,a0,1
    5726:	00279813          	slli	a6,a5,0x2
    572a:	4589                	c.li	a1,2
    572c:	03042c23          	sw	a6,56(s0)
    5730:	0b042223          	sw	a6,164(s0)
    5734:	8634                	exec.it	#91     !sw	zero,160(s0)
    5736:	c00c                	c.sw	a1,0(s0)
    5738:	85e6                	c.mv	a1,s9
    573a:	8522                	c.mv	a0,s0
    573c:	3d25                	c.jal	5574 <fill_temp>
    573e:	d139                	c.beqz	a0,5684 <xz_dec_run+0x7e>
    5740:	0a442883          	lw	a7,164(s0)
    5744:	4601                	c.li	a2,0
    5746:	ffc88593          	addi	a1,a7,-4 # 3ffc <ty_pvPortMalloc+0xa8>
    574a:	8526                	c.mv	a0,s1
    574c:	0ab42223          	sw	a1,164(s0)
    5750:	8814                	exec.it	#13     !jal	ra,4356 <xz_crc32>
    5752:	0a442603          	lw	a2,164(s0)
    5756:	8c2a                	c.mv	s8,a0
    5758:	00c48533          	add	a0,s1,a2
    575c:	c632                	c.swsp	a2,12(sp)
    575e:	339d                	c.jal	54c4 <get_unaligned_le32>
    5760:	f4ac1be3          	bne	s8,a0,56b6 <xz_dec_run+0xb0>
    5764:	0a944e83          	lbu	t4,169(s0)
    5768:	4e09                	c.li	t3,2
    576a:	03feff13          	andi	t5,t4,63
    576e:	0bc42023          	sw	t3,160(s0)
    5772:	f60f1ae3          	bnez	t5,56e6 <xz_dec_run+0xe0>
    5776:	46b2                	c.lwsp	a3,12(sp)
    5778:	2a6ef05b          	bbc	t4,6,5a18 <xz_dec_run+0x412>
    577c:	8656                	c.mv	a2,s5
    577e:	85a6                	c.mv	a1,s1
    5780:	8522                	c.mv	a0,s0
    5782:	3399                	c.jal	54c8 <dec_vli>
    5784:	b215695b          	bnec	a0,1,56b6 <xz_dec_run+0xb0>
    5788:	00842f83          	lw	t6,8(s0)
    578c:	8e30                	exec.it	#87     !lw	ra,12(s0)
    578e:	03f42423          	sw	t6,40(s0)
    5792:	02142623          	sw	ra,44(s0)
    5796:	0a940283          	lb	t0,169(s0)
    579a:	2802d463          	bgez	t0,5a22 <xz_dec_run+0x41c>
    579e:	0a442683          	lw	a3,164(s0)
    57a2:	8656                	c.mv	a2,s5
    57a4:	85a6                	c.mv	a1,s1
    57a6:	8522                	c.mv	a0,s0
    57a8:	3305                	c.jal	54c8 <dec_vli>
    57aa:	b015665b          	bnec	a0,1,56b6 <xz_dec_run+0xb0>
    57ae:	00842303          	lw	t1,8(s0)
    57b2:	4458                	c.lw	a4,12(s0)
    57b4:	02642823          	sw	t1,48(s0)
    57b8:	d858                	c.sw	a4,52(s0)
    57ba:	0a442383          	lw	t2,164(s0)
    57be:	0a042503          	lw	a0,160(s0)
    57c2:	4685                	c.li	a3,1
    57c4:	40a387b3          	sub	a5,t2,a0
    57c8:	eef6f7e3          	bgeu	a3,a5,56b6 <xz_dec_run+0xb0>
    57cc:	00150813          	addi	a6,a0,1
    57d0:	00a405b3          	add	a1,s0,a0
    57d4:	0b042023          	sw	a6,160(s0)
    57d8:	0a85c883          	lbu	a7,168(a1)
    57dc:	b018e5db          	bnec	a7,33,56e6 <xz_dec_run+0xe0>
    57e0:	00250c13          	addi	s8,a0,2
    57e4:	01040633          	add	a2,s0,a6
    57e8:	0b842023          	sw	s8,160(s0)
    57ec:	0a864e03          	lbu	t3,168(a2)
    57f0:	ae1e6b5b          	bnec	t3,1,56e6 <xz_dec_run+0xe0>
    57f4:	ed8381e3          	beq	t2,s8,56b6 <xz_dec_run+0xb0>
    57f8:	00350e93          	addi	t4,a0,3
    57fc:	01840f33          	add	t5,s0,s8
    5800:	0bd42023          	sw	t4,160(s0)
    5804:	4a842503          	lw	a0,1192(s0)
    5808:	0a8f4583          	lbu	a1,168(t5)
    580c:	c35ff0ef          	jal	ra,5440 <xz_dec_lzma2_reset>
    5810:	8c2a                	c.mv	s8,a0
    5812:	ea0513e3          	bnez	a0,56b8 <xz_dec_run+0xb2>
    5816:	0a442083          	lw	ra,164(s0)
    581a:	0a042f83          	lw	t6,160(s0)
    581e:	201fe763          	bltu	t6,ra,5a2c <xz_dec_run+0x426>
    5822:	4081                	c.li	ra,0
    5824:	4281                	c.li	t0,0
    5826:	430d                	c.li	t1,3
    5828:	8634                	exec.it	#91     !sw	zero,160(s0)
    582a:	04142023          	sw	ra,64(s0)
    582e:	04542223          	sw	t0,68(s0)
    5832:	04142423          	sw	ra,72(s0)
    5836:	04542623          	sw	t0,76(s0)
    583a:	8810                	exec.it	#5     !sw	t1,0(s0)
    583c:	004ca703          	lw	a4,4(s9)
    5840:	4a842503          	lw	a0,1192(s0)
    5844:	c818                	c.sw	a4,16(s0)
    5846:	010ca383          	lw	t2,16(s9)
    584a:	85e6                	c.mv	a1,s9
    584c:	00742a23          	sw	t2,20(s0)
    5850:	e00ff0ef          	jal	ra,4e50 <xz_dec_lzma2_run>
    5854:	481c                	c.lw	a5,16(s0)
    5856:	8c2a                	c.mv	s8,a0
    5858:	004ca503          	lw	a0,4(s9)
    585c:	04042803          	lw	a6,64(s0)
    5860:	40f506b3          	sub	a3,a0,a5
    5864:	04442883          	lw	a7,68(s0)
    5868:	010685b3          	add	a1,a3,a6
    586c:	00d5b633          	sltu	a2,a1,a3
    5870:	01160e33          	add	t3,a2,a7
    5874:	01442e83          	lw	t4,20(s0)
    5878:	c02c                	c.sw	a1,64(s0)
    587a:	05c42223          	sw	t3,68(s0)
    587e:	010caf03          	lw	t5,16(s9)
    5882:	04842283          	lw	t0,72(s0)
    5886:	41df0fb3          	sub	t6,t5,t4
    588a:	04c42303          	lw	t1,76(s0)
    588e:	005f83b3          	add	t2,t6,t0
    5892:	01f3b733          	sltu	a4,t2,t6
    5896:	00670533          	add	a0,a4,t1
    589a:	545c                	c.lw	a5,44(s0)
    589c:	04742423          	sw	t2,72(s0)
    58a0:	c468                	c.sw	a0,76(s0)
    58a2:	e1c7eae3          	bltu	a5,t3,56b6 <xz_dec_run+0xb0>
    58a6:	01c79663          	bne	a5,t3,58b2 <xz_dec_run+0x2ac>
    58aa:	02842083          	lw	ra,40(s0)
    58ae:	e0b0e4e3          	bltu	ra,a1,56b6 <xz_dec_run+0xb0>
    58b2:	5854                	c.lw	a3,52(s0)
    58b4:	e0a6e1e3          	bltu	a3,a0,56b6 <xz_dec_run+0xb0>
    58b8:	00a69663          	bne	a3,a0,58c4 <xz_dec_run+0x2be>
    58bc:	03042803          	lw	a6,48(s0)
    58c0:	de786be3          	bltu	a6,t2,56b6 <xz_dec_run+0xb0>
    58c4:	01c42883          	lw	a7,28(s0)
    58c8:	0018ed5b          	bnec	a7,1,58e2 <xz_dec_run+0x2dc>
    58cc:	010ca583          	lw	a1,16(s9)
    58d0:	00ccae03          	lw	t3,12(s9)
    58d4:	4c10                	c.lw	a2,24(s0)
    58d6:	41d585b3          	sub	a1,a1,t4
    58da:	01de0533          	add	a0,t3,t4
    58de:	8814                	exec.it	#13     !jal	ra,4356 <xz_crc32>
    58e0:	cc08                	c.sw	a0,24(s0)
    58e2:	9c1c6b5b          	bnec	s8,1,56b8 <xz_dec_run+0xb2>
    58e6:	02842c03          	lw	s8,40(s0)
    58ea:	567d                	c.li	a2,-1
    58ec:	02c42e83          	lw	t4,44(s0)
    58f0:	00cc1463          	bne	s8,a2,58f8 <xz_dec_run+0x2f2>
    58f4:	018e8a63          	beq	t4,s8,5908 <xz_dec_run+0x302>
    58f8:	04042f03          	lw	t5,64(s0)
    58fc:	db8f1de3          	bne	t5,s8,56b6 <xz_dec_run+0xb0>
    5900:	04442f83          	lw	t6,68(s0)
    5904:	dbdf99e3          	bne	t6,t4,56b6 <xz_dec_run+0xb0>
    5908:	03042283          	lw	t0,48(s0)
    590c:	577d                	c.li	a4,-1
    590e:	03442303          	lw	t1,52(s0)
    5912:	04842383          	lw	t2,72(s0)
    5916:	4468                	c.lw	a0,76(s0)
    5918:	00e29463          	bne	t0,a4,5920 <xz_dec_run+0x31a>
    591c:	00530663          	beq	t1,t0,5928 <xz_dec_run+0x322>
    5920:	d8729be3          	bne	t0,t2,56b6 <xz_dec_run+0xb0>
    5924:	d8a319e3          	bne	t1,a0,56b6 <xz_dec_run+0xb0>
    5928:	4c3c                	c.lw	a5,88(s0)
    592a:	4034                	c.lw	a3,64(s0)
    592c:	05c42083          	lw	ra,92(s0)
    5930:	04442803          	lw	a6,68(s0)
    5934:	03842e83          	lw	t4,56(s0)
    5938:	00d788b3          	add	a7,a5,a3
    593c:	01d88f33          	add	t5,a7,t4
    5940:	00f8b5b3          	sltu	a1,a7,a5
    5944:	01008e33          	add	t3,ra,a6
    5948:	01c58c33          	add	s8,a1,t3
    594c:	011f3633          	sltu	a2,t5,a7
    5950:	01860fb3          	add	t6,a2,s8
    5954:	05e42c23          	sw	t5,88(s0)
    5958:	05f42e23          	sw	t6,92(s0)
    595c:	01c42283          	lw	t0,28(s0)
    5960:	0012eb5b          	bnec	t0,1,5976 <xz_dec_run+0x370>
    5964:	004f0313          	addi	t1,t5,4
    5968:	01e33733          	sltu	a4,t1,t5
    596c:	01f707b3          	add	a5,a4,t6
    5970:	04642c23          	sw	t1,88(s0)
    5974:	cc7c                	c.sw	a5,92(s0)
    5976:	06042083          	lw	ra,96(s0)
    597a:	5074                	c.lw	a3,100(s0)
    597c:	9386                	c.add	t2,ra
    597e:	9536                	c.add	a0,a3
    5980:	5430                	c.lw	a2,104(s0)
    5982:	0013b833          	sltu	a6,t2,ra
    5986:	00a808b3          	add	a7,a6,a0
    598a:	45e1                	c.li	a1,24
    598c:	856a                	c.mv	a0,s10
    598e:	06742023          	sw	t2,96(s0)
    5992:	07142223          	sw	a7,100(s0)
    5996:	8814                	exec.it	#13     !jal	ra,4356 <xz_crc32>
    5998:	482c                	c.lw	a1,80(s0)
    599a:	05442e03          	lw	t3,84(s0)
    599e:	00158c13          	addi	s8,a1,1
    59a2:	00bc3eb3          	sltu	t4,s8,a1
    59a6:	01ce8f33          	add	t5,t4,t3
    59aa:	4611                	c.li	a2,4
    59ac:	d428                	c.sw	a0,104(s0)
    59ae:	05842823          	sw	s8,80(s0)
    59b2:	8e50                	exec.it	#103     !sw	t5,84(s0)
    59b4:	c010                	c.sw	a2,0(s0)
    59b6:	04042f83          	lw	t6,64(s0)
    59ba:	003ff293          	andi	t0,t6,3
    59be:	08029263          	bnez	t0,5a42 <xz_dec_run+0x43c>
    59c2:	4315                	c.li	t1,5
    59c4:	8810                	exec.it	#5     !sw	t1,0(s0)
    59c6:	4c58                	c.lw	a4,28(s0)
    59c8:	0417665b          	bnec	a4,1,5a14 <xz_dec_run+0x40e>
    59cc:	004ca783          	lw	a5,4(s9)
    59d0:	008ca683          	lw	a3,8(s9)
    59d4:	cad788e3          	beq	a5,a3,5684 <xz_dec_run+0x7e>
    59d8:	000cae03          	lw	t3,0(s9)
    59dc:	00442383          	lw	t2,4(s0)
    59e0:	01842803          	lw	a6,24(s0)
    59e4:	00178593          	addi	a1,a5,1
    59e8:	00fe0c33          	add	s8,t3,a5
    59ec:	00785533          	srl	a0,a6,t2
    59f0:	00bca223          	sw	a1,4(s9)
    59f4:	000c4e83          	lbu	t4,0(s8)
    59f8:	0ff57893          	andi	a7,a0,255
    59fc:	cbd89de3          	bne	a7,t4,56b6 <xz_dec_run+0xb0>
    5a00:	8454                	exec.it	#43     !lw	t5,4(s0)
    5a02:	4ffd                	c.li	t6,31
    5a04:	008f0613          	addi	a2,t5,8
    5a08:	c050                	c.sw	a2,4(s0)
    5a0a:	fccff1e3          	bgeu	t6,a2,59cc <xz_dec_run+0x3c6>
    5a0e:	00042c23          	sw	zero,24(s0)
    5a12:	8804                	exec.it	#12     !sw	zero,4(s0)
    5a14:	4285                	c.li	t0,1
    5a16:	b321                	c.j	571e <xz_dec_run+0x118>
    5a18:	03642423          	sw	s6,40(s0)
    5a1c:	03742623          	sw	s7,44(s0)
    5a20:	bb9d                	c.j	5796 <xz_dec_run+0x190>
    5a22:	03642823          	sw	s6,48(s0)
    5a26:	03742a23          	sw	s7,52(s0)
    5a2a:	bb41                	c.j	57ba <xz_dec_run+0x1b4>
    5a2c:	001f8e93          	addi	t4,t6,1
    5a30:	01f40f33          	add	t5,s0,t6
    5a34:	0bd42023          	sw	t4,160(s0)
    5a38:	0a8f4603          	lbu	a2,168(t5)
    5a3c:	dc060fe3          	beqz	a2,581a <xz_dec_run+0x214>
    5a40:	b15d                	c.j	56e6 <xz_dec_run+0xe0>
    5a42:	004ca303          	lw	t1,4(s9)
    5a46:	008ca703          	lw	a4,8(s9)
    5a4a:	c2e30de3          	beq	t1,a4,5684 <xz_dec_run+0x7e>
    5a4e:	000ca783          	lw	a5,0(s9)
    5a52:	00130693          	addi	a3,t1,1
    5a56:	006783b3          	add	t2,a5,t1
    5a5a:	00dca223          	sw	a3,4(s9)
    5a5e:	0003c803          	lbu	a6,0(t2)
    5a62:	c4081ae3          	bnez	a6,56b6 <xz_dec_run+0xb0>
    5a66:	4028                	c.lw	a0,64(s0)
    5a68:	04442883          	lw	a7,68(s0)
    5a6c:	00150e13          	addi	t3,a0,1
    5a70:	00ae35b3          	sltu	a1,t3,a0
    5a74:	01158c33          	add	s8,a1,a7
    5a78:	05c42023          	sw	t3,64(s0)
    5a7c:	05842223          	sw	s8,68(s0)
    5a80:	bf1d                	c.j	59b6 <xz_dec_run+0x3b0>
    5a82:	07042083          	lw	ra,112(s0)
    5a86:	0210d85b          	beqc	ra,1,5ab6 <xz_dec_run+0x4b0>
    5a8a:	0620d65b          	beqc	ra,2,5af6 <xz_dec_run+0x4f0>
    5a8e:	04009963          	bnez	ra,5ae0 <xz_dec_run+0x4da>
    5a92:	00842e03          	lw	t3,8(s0)
    5a96:	8e14                	exec.it	#79     !lw	t4,12(s0)
    5a98:	05042f03          	lw	t5,80(s0)
    5a9c:	09c42023          	sw	t3,128(s0)
    5aa0:	09d42223          	sw	t4,132(s0)
    5aa4:	c1cf19e3          	bne	t5,t3,56b6 <xz_dec_run+0xb0>
    5aa8:	05442f83          	lw	t6,84(s0)
    5aac:	c1df95e3          	bne	t6,t4,56b6 <xz_dec_run+0xb0>
    5ab0:	07842823          	sw	s8,112(s0)
    5ab4:	a035                	c.j	5ae0 <xz_dec_run+0x4da>
    5ab6:	08040793          	addi	a5,s0,128
    5aba:	0087a803          	lw	a6,8(a5)
    5abe:	4410                	c.lw	a2,8(s0)
    5ac0:	47cc                	c.lw	a1,12(a5)
    5ac2:	00c42883          	lw	a7,12(s0)
    5ac6:	00c80c33          	add	s8,a6,a2
    5aca:	010c3e33          	sltu	t3,s8,a6
    5ace:	01158eb3          	add	t4,a1,a7
    5ad2:	8460                	exec.it	#50     !add	t5,t3,t4
    5ad4:	0187a423          	sw	s8,8(a5)
    5ad8:	01e7a623          	sw	t5,12(a5)
    5adc:	07542823          	sw	s5,112(s0)
    5ae0:	08042c03          	lw	s8,128(s0)
    5ae4:	08442283          	lw	t0,132(s0)
    5ae8:	005c6733          	or	a4,s8,t0
    5aec:	b60718e3          	bnez	a4,565c <xz_dec_run+0x56>
    5af0:	431d                	c.li	t1,7
    5af2:	8810                	exec.it	#5     !sw	t1,0(s0)
    5af4:	a89d                	c.j	5b6a <xz_dec_run+0x564>
    5af6:	08040493          	addi	s1,s0,128
    5afa:	0104af83          	lw	t6,16(s1)
    5afe:	4418                	c.lw	a4,8(s0)
    5b00:	8860                	exec.it	#52     !lw	t1,12(s0)
    5b02:	0144a283          	lw	t0,20(s1)
    5b06:	00ef86b3          	add	a3,t6,a4
    5b0a:	00628533          	add	a0,t0,t1
    5b0e:	4c90                	c.lw	a2,24(s1)
    5b10:	01f6b3b3          	sltu	t2,a3,t6
    5b14:	00a380b3          	add	ra,t2,a0
    5b18:	45e1                	c.li	a1,24
    5b1a:	855a                	c.mv	a0,s6
    5b1c:	c894                	c.sw	a3,16(s1)
    5b1e:	0014aa23          	sw	ra,20(s1)
    5b22:	8814                	exec.it	#13     !jal	ra,4356 <xz_crc32>
    5b24:	cc88                	c.sw	a0,24(s1)
    5b26:	08042783          	lw	a5,128(s0)
    5b2a:	0044a803          	lw	a6,4(s1)
    5b2e:	fff78593          	addi	a1,a5,-1
    5b32:	0017b613          	seqz	a2,a5
    5b36:	40c808b3          	sub	a7,a6,a2
    5b3a:	08b42023          	sw	a1,128(s0)
    5b3e:	8214                	exec.it	#73     !sw	a7,4(s1)
    5b40:	bf85                	c.j	5ab0 <xz_dec_run+0x4aa>
    5b42:	008ca583          	lw	a1,8(s9)
    5b46:	00d59763          	bne	a1,a3,5b54 <xz_dec_run+0x54e>
    5b4a:	85e6                	c.mv	a1,s9
    5b4c:	8522                	c.mv	a0,s0
    5b4e:	9edff0ef          	jal	ra,553a <index_update>
    5b52:	be0d                	c.j	5684 <xz_dec_run+0x7e>
    5b54:	000ca883          	lw	a7,0(s9)
    5b58:	00168613          	addi	a2,a3,1
    5b5c:	00d88e33          	add	t3,a7,a3
    5b60:	00cca223          	sw	a2,4(s9)
    5b64:	8870                	exec.it	#53     !lbu	t4,0(t3)
    5b66:	b40e98e3          	bnez	t4,56b6 <xz_dec_run+0xb0>
    5b6a:	004ca683          	lw	a3,4(s9)
    5b6e:	01042383          	lw	t2,16(s0)
    5b72:	5c3c                	c.lw	a5,120(s0)
    5b74:	40768533          	sub	a0,a3,t2
    5b78:	00f50833          	add	a6,a0,a5
    5b7c:	00387593          	andi	a1,a6,3
    5b80:	f1e9                	c.bnez	a1,5b42 <xz_dec_run+0x53c>
    5b82:	85e6                	c.mv	a1,s9
    5b84:	8522                	c.mv	a0,s0
    5b86:	9b5ff0ef          	jal	ra,553a <index_update>
    5b8a:	4661                	c.li	a2,24
    5b8c:	08840593          	addi	a1,s0,136
    5b90:	05840513          	addi	a0,s0,88
    5b94:	2af5                	c.jal	5d90 <memcmp>
    5b96:	b20510e3          	bnez	a0,56b6 <xz_dec_run+0xb0>
    5b9a:	40a1                	c.li	ra,8
    5b9c:	8a20                	exec.it	#84     !sw	ra,0(s0)
    5b9e:	48fd                	c.li	a7,31
    5ba0:	004ca603          	lw	a2,4(s9)
    5ba4:	008cae03          	lw	t3,8(s9)
    5ba8:	adc60ee3          	beq	a2,t3,5684 <xz_dec_run+0x7e>
    5bac:	000ca283          	lw	t0,0(s9)
    5bb0:	8820                	exec.it	#20     !lw	t4,4(s0)
    5bb2:	01842f03          	lw	t5,24(s0)
    5bb6:	00160713          	addi	a4,a2,1
    5bba:	00c28333          	add	t1,t0,a2
    5bbe:	01df5fb3          	srl	t6,t5,t4
    5bc2:	00eca223          	sw	a4,4(s9)
    5bc6:	00034683          	lbu	a3,0(t1)
    5bca:	0ffffc13          	andi	s8,t6,255
    5bce:	aedc14e3          	bne	s8,a3,56b6 <xz_dec_run+0xb0>
    5bd2:	00442383          	lw	t2,4(s0)
    5bd6:	00838513          	addi	a0,t2,8
    5bda:	c048                	c.sw	a0,4(s0)
    5bdc:	fca8f2e3          	bgeu	a7,a0,5ba0 <xz_dec_run+0x59a>
    5be0:	47b1                	c.li	a5,12
    5be2:	4825                	c.li	a6,9
    5be4:	00042c23          	sw	zero,24(s0)
    5be8:	8804                	exec.it	#12     !sw	zero,4(s0)
    5bea:	0af42223          	sw	a5,164(s0)
    5bee:	8e34                	exec.it	#95     !sw	a6,0(s0)
    5bf0:	85e6                	c.mv	a1,s9
    5bf2:	8522                	c.mv	a0,s0
    5bf4:	981ff0ef          	jal	ra,5574 <fill_temp>
    5bf8:	a80506e3          	beqz	a0,5684 <xz_dec_run+0x7e>
    5bfc:	4609                	c.li	a2,2
    5bfe:	00003597          	auipc	a1,0x3
    5c02:	9e658593          	addi	a1,a1,-1562 # 85e4 <s_crc32+0x42c>
    5c06:	0b240513          	addi	a0,s0,178
    5c0a:	2259                	c.jal	5d90 <memcmp>
    5c0c:	aa0515e3          	bnez	a0,56b6 <xz_dec_run+0xb0>
    5c10:	0ac40a13          	addi	s4,s0,172
    5c14:	4601                	c.li	a2,0
    5c16:	4599                	c.li	a1,6
    5c18:	8552                	c.mv	a0,s4
    5c1a:	8814                	exec.it	#13     !jal	ra,4356 <xz_crc32>
    5c1c:	84aa                	c.mv	s1,a0
    5c1e:	0a840513          	addi	a0,s0,168
    5c22:	8a3ff0ef          	jal	ra,54c4 <get_unaligned_le32>
    5c26:	a8a498e3          	bne	s1,a0,56b6 <xz_dec_run+0xb0>
    5c2a:	8552                	c.mv	a0,s4
    5c2c:	899ff0ef          	jal	ra,54c4 <get_unaligned_le32>
    5c30:	07c42283          	lw	t0,124(s0)
    5c34:	5c38                	c.lw	a4,120(s0)
    5c36:	01e29313          	slli	t1,t0,0x1e
    5c3a:	00275793          	srli	a5,a4,0x2
    5c3e:	00f366b3          	or	a3,t1,a5
    5c42:	0022d393          	srli	t2,t0,0x2
    5c46:	a6a698e3          	bne	a3,a0,56b6 <xz_dec_run+0xb0>
    5c4a:	a60396e3          	bnez	t2,56b6 <xz_dec_run+0xb0>
    5c4e:	0b044803          	lbu	a6,176(s0)
    5c52:	a60812e3          	bnez	a6,56b6 <xz_dec_run+0xb0>
    5c56:	0b144503          	lbu	a0,177(s0)
    5c5a:	01c42883          	lw	a7,28(s0)
    5c5e:	4c05                	c.li	s8,1
    5c60:	a5150ce3          	beq	a0,a7,56b8 <xz_dec_run+0xb2>
    5c64:	bc89                	c.j	56b6 <xz_dec_run+0xb0>
    5c66:	4c15                	c.li	s8,5
    5c68:	bc81                	c.j	56b8 <xz_dec_run+0xb2>
    5c6a:	001c545b          	beqc	s8,1,5c72 <xz_dec_run+0x66c>
    5c6e:	a67ff06f          	j	56d4 <xz_dec_run+0xce>
    5c72:	8562                	c.mv	a0,s8
    5c74:	0141                	c.addi	sp,16
    5c76:	a0ed                	c.j	5d60 <__riscv_restore_12>
    5c78:	020c1363          	bnez	s8,5c9e <xz_dec_run+0x698>
    5c7c:	004ca583          	lw	a1,4(s9)
    5c80:	01259f63          	bne	a1,s2,5c9e <xz_dec_run+0x698>
    5c84:	010cae83          	lw	t4,16(s9)
    5c88:	013e9b63          	bne	t4,s3,5c9e <xz_dec_run+0x698>
    5c8c:	02444f03          	lbu	t5,36(s0)
    5c90:	000f0363          	beqz	t5,5c96 <xz_dec_run+0x690>
    5c94:	4c21                	c.li	s8,8
    5c96:	4605                	c.li	a2,1
    5c98:	02c40223          	sb	a2,36(s0)
    5c9c:	bfd9                	c.j	5c72 <xz_dec_run+0x66c>
    5c9e:	02040223          	sb	zero,36(s0)
    5ca2:	bfc1                	c.j	5c72 <xz_dec_run+0x66c>

00005ca4 <xz_dec_init>:
    5ca4:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    5ca6:	1141                	c.addi	sp,-16
    5ca8:	84aa                	c.mv	s1,a0
    5caa:	4b000513          	li	a0,1200
    5cae:	c62e                	c.swsp	a1,12(sp)
    5cb0:	28d1                	c.jal	5d84 <malloc>
    5cb2:	842a                	c.mv	s0,a0
    5cb4:	cd01                	c.beqz	a0,5ccc <xz_dec_init+0x28>
    5cb6:	45b2                	c.lwsp	a1,12(sp)
    5cb8:	d104                	c.sw	s1,32(a0)
    5cba:	8526                	c.mv	a0,s1
    5cbc:	f44ff0ef          	jal	ra,5400 <xz_dec_lzma2_create>
    5cc0:	4aa42423          	sw	a0,1192(s0)
    5cc4:	e519                	c.bnez	a0,5cd2 <xz_dec_init+0x2e>
    5cc6:	8522                	c.mv	a0,s0
    5cc8:	4401                	c.li	s0,0
    5cca:	20c1                	c.jal	5d8a <free>
    5ccc:	8522                	c.mv	a0,s0
    5cce:	0141                	c.addi	sp,16
    5cd0:	a065                	c.j	5d78 <__riscv_restore_0>
    5cd2:	8522                	c.mv	a0,s0
    5cd4:	8f9ff0ef          	jal	ra,55cc <xz_dec_reset>
    5cd8:	bfd5                	c.j	5ccc <xz_dec_init+0x28>

00005cda <xz_dec_end>:
    5cda:	c919                	c.beqz	a0,5cf0 <xz_dec_end+0x16>
    5cdc:	078002ef          	jal	t0,5d54 <__riscv_save_0>
    5ce0:	842a                	c.mv	s0,a0
    5ce2:	4a852503          	lw	a0,1192(a0)
    5ce6:	fc8ff0ef          	jal	ra,54ae <xz_dec_lzma2_end>
    5cea:	8522                	c.mv	a0,s0
    5cec:	2879                	c.jal	5d8a <free>
    5cee:	a069                	c.j	5d78 <__riscv_restore_0>
    5cf0:	8082                	c.jr	ra
    5cf2:	0001                	c.nop

00005cf4 <__ashldi3>:
    5cf4:	87aa                	c.mv	a5,a0
    5cf6:	c60d                	c.beqz	a2,5d20 <__ashldi3+0x2c>
    5cf8:	02000713          	li	a4,32
    5cfc:	8f11                	c.sub	a4,a2
    5cfe:	00e05b63          	blez	a4,5d14 <__ashldi3+0x20>
    5d02:	00e55733          	srl	a4,a0,a4
    5d06:	00c595b3          	sll	a1,a1,a2
    5d0a:	00c51533          	sll	a0,a0,a2
    5d0e:	8dd9                	c.or	a1,a4
    5d10:	00008067          	ret
    5d14:	fe060593          	addi	a1,a2,-32
    5d18:	4501                	c.li	a0,0
    5d1a:	00b795b3          	sll	a1,a5,a1
    5d1e:	8082                	c.jr	ra
    5d20:	8082                	c.jr	ra
    5d22:	0001                	c.nop

00005d24 <__riscv_save_12>:
    5d24:	7139                	c.addi16sp	sp,-64
    5d26:	4301                	c.li	t1,0
    5d28:	c66e                	c.swsp	s11,12(sp)
    5d2a:	a019                	c.j	5d30 <__riscv_save_10+0x4>

00005d2c <__riscv_save_10>:
    5d2c:	7139                	c.addi16sp	sp,-64
    5d2e:	5341                	c.li	t1,-16
    5d30:	c86a                	c.swsp	s10,16(sp)
    5d32:	ca66                	c.swsp	s9,20(sp)
    5d34:	cc62                	c.swsp	s8,24(sp)
    5d36:	ce5e                	c.swsp	s7,28(sp)
    5d38:	a019                	c.j	5d3e <__riscv_save_4+0x4>

00005d3a <__riscv_save_4>:
    5d3a:	7139                	c.addi16sp	sp,-64
    5d3c:	5301                	c.li	t1,-32
    5d3e:	d05a                	c.swsp	s6,32(sp)
    5d40:	d256                	c.swsp	s5,36(sp)
    5d42:	d452                	c.swsp	s4,40(sp)
    5d44:	d64e                	c.swsp	s3,44(sp)
    5d46:	d84a                	c.swsp	s2,48(sp)
    5d48:	da26                	c.swsp	s1,52(sp)
    5d4a:	dc22                	c.swsp	s0,56(sp)
    5d4c:	de06                	c.swsp	ra,60(sp)
    5d4e:	40610133          	sub	sp,sp,t1
    5d52:	8282                	c.jr	t0

00005d54 <__riscv_save_0>:
    5d54:	1141                	c.addi	sp,-16
    5d56:	c04a                	c.swsp	s2,0(sp)
    5d58:	c226                	c.swsp	s1,4(sp)
    5d5a:	c422                	c.swsp	s0,8(sp)
    5d5c:	c606                	c.swsp	ra,12(sp)
    5d5e:	8282                	c.jr	t0

00005d60 <__riscv_restore_12>:
    5d60:	4db2                	c.lwsp	s11,12(sp)
    5d62:	0141                	c.addi	sp,16

00005d64 <__riscv_restore_10>:
    5d64:	4d02                	c.lwsp	s10,0(sp)
    5d66:	4c92                	c.lwsp	s9,4(sp)
    5d68:	4c22                	c.lwsp	s8,8(sp)
    5d6a:	4bb2                	c.lwsp	s7,12(sp)
    5d6c:	0141                	c.addi	sp,16

00005d6e <__riscv_restore_4>:
    5d6e:	4b02                	c.lwsp	s6,0(sp)
    5d70:	4a92                	c.lwsp	s5,4(sp)
    5d72:	4a22                	c.lwsp	s4,8(sp)
    5d74:	49b2                	c.lwsp	s3,12(sp)
    5d76:	0141                	c.addi	sp,16

00005d78 <__riscv_restore_0>:
    5d78:	4902                	c.lwsp	s2,0(sp)
    5d7a:	4492                	c.lwsp	s1,4(sp)
    5d7c:	4422                	c.lwsp	s0,8(sp)
    5d7e:	40b2                	c.lwsp	ra,12(sp)
    5d80:	0141                	c.addi	sp,16
    5d82:	8082                	c.jr	ra

00005d84 <malloc>:
    5d84:	85aa                	c.mv	a1,a0
    5d86:	8650                	exec.it	#99     !lwgp	a0,-2024 # 30000110 <_impure_ptr>
    5d88:	a4f1                	c.j	6054 <_malloc_r>

00005d8a <free>:
    5d8a:	85aa                	c.mv	a1,a0
    5d8c:	8650                	exec.it	#99     !lwgp	a0,-2024 # 30000110 <_impure_ptr>
    5d8e:	ac35                	c.j	5fca <_free_r>

00005d90 <memcmp>:
    5d90:	00c507b3          	add	a5,a0,a2
    5d94:	00b566b3          	or	a3,a0,a1
    5d98:	8a8d                	c.andi	a3,3
    5d9a:	8736                	c.mv	a4,a3
    5d9c:	ee91                	c.bnez	a3,5db8 <memcmp+0x28>
    5d9e:	9a71                	c.andi	a2,-4
    5da0:	00a60633          	add	a2,a2,a0
    5da4:	00c50a63          	beq	a0,a2,5db8 <memcmp+0x28>
    5da8:	4114                	c.lw	a3,0(a0)
    5daa:	0511                	c.addi	a0,4
    5dac:	4198                	c.lw	a4,0(a1)
    5dae:	0591                	c.addi	a1,4
    5db0:	fee68ae3          	beq	a3,a4,5da4 <memcmp+0x14>
    5db4:	1571                	c.addi	a0,-4
    5db6:	15f1                	c.addi	a1,-4
    5db8:	00f50a63          	beq	a0,a5,5dcc <memcmp+0x3c>
    5dbc:	00054683          	lbu	a3,0(a0)
    5dc0:	0505                	c.addi	a0,1
    5dc2:	0005c703          	lbu	a4,0(a1)
    5dc6:	0585                	c.addi	a1,1
    5dc8:	fee688e3          	beq	a3,a4,5db8 <memcmp+0x28>
    5dcc:	40e68533          	sub	a0,a3,a4
    5dd0:	00008067          	ret

00005dd4 <memcpy>:
    5dd4:	00c583b3          	add	t2,a1,a2
    5dd8:	82aa                	c.mv	t0,a0
    5dda:	0035f713          	andi	a4,a1,3
    5dde:	00357793          	andi	a5,a0,3
    5de2:	06f71d63          	bne	a4,a5,5e5c <memcpy+0x88>
    5de6:	c30d                	c.beqz	a4,5e08 <memcpy+0x34>
    5de8:	ffc78793          	addi	a5,a5,-4
    5dec:	40f58333          	sub	t1,a1,a5
    5df0:	0663c663          	blt	t2,t1,5e5c <memcpy+0x88>
    5df4:	0005c683          	lbu	a3,0(a1)
    5df8:	0585                	c.addi	a1,1
    5dfa:	00d50023          	sb	a3,0(a0)
    5dfe:	0505                	c.addi	a0,1
    5e00:	fe659ae3          	bne	a1,t1,5df4 <memcpy+0x20>
    5e04:	04b38f63          	beq	t2,a1,5e62 <memcpy+0x8e>
    5e08:	963e                	c.add	a2,a5
    5e0a:	9a01                	c.andi	a2,-32
    5e0c:	02060a63          	beqz	a2,5e40 <memcpy+0x6c>
    5e10:	00c58333          	add	t1,a1,a2
    5e14:	499c                	c.lw	a5,16(a1)
    5e16:	49d8                	c.lw	a4,20(a1)
    5e18:	4d94                	c.lw	a3,24(a1)
    5e1a:	4dd0                	c.lw	a2,28(a1)
    5e1c:	c91c                	c.sw	a5,16(a0)
    5e1e:	c958                	c.sw	a4,20(a0)
    5e20:	cd14                	c.sw	a3,24(a0)
    5e22:	cd50                	c.sw	a2,28(a0)
    5e24:	419c                	c.lw	a5,0(a1)
    5e26:	41d8                	c.lw	a4,4(a1)
    5e28:	4594                	c.lw	a3,8(a1)
    5e2a:	45d0                	c.lw	a2,12(a1)
    5e2c:	c11c                	c.sw	a5,0(a0)
    5e2e:	c158                	c.sw	a4,4(a0)
    5e30:	c514                	c.sw	a3,8(a0)
    5e32:	c550                	c.sw	a2,12(a0)
    5e34:	02058593          	addi	a1,a1,32
    5e38:	02050513          	addi	a0,a0,32
    5e3c:	fc659ce3          	bne	a1,t1,5e14 <memcpy+0x40>
    5e40:	ffc3f313          	andi	t1,t2,-4
    5e44:	00b36c63          	bltu	t1,a1,5e5c <memcpy+0x88>
    5e48:	00458693          	addi	a3,a1,4
    5e4c:	00d36863          	bltu	t1,a3,5e5c <memcpy+0x88>
    5e50:	4194                	c.lw	a3,0(a1)
    5e52:	0591                	c.addi	a1,4
    5e54:	c114                	c.sw	a3,0(a0)
    5e56:	0511                	c.addi	a0,4
    5e58:	fe659ce3          	bne	a1,t1,5e50 <memcpy+0x7c>
    5e5c:	831e                	c.mv	t1,t2
    5e5e:	f8b39be3          	bne	t2,a1,5df4 <memcpy+0x20>
    5e62:	8516                	c.mv	a0,t0
    5e64:	8082                	c.jr	ra
    5e66:	0001                	c.nop

00005e68 <memmove>:
    5e68:	83aa                	c.mv	t2,a0
    5e6a:	00c586b3          	add	a3,a1,a2
    5e6e:	00c50333          	add	t1,a0,a2
    5e72:	02d55c63          	bge	a0,a3,5eaa <memmove+0x42>
    5e76:	0266da63          	bge	a3,t1,5eaa <memmove+0x42>
    5e7a:	57fd                	c.li	a5,-1
    5e7c:	832a                	c.mv	t1,a0
    5e7e:	9532                	c.add	a0,a2
    5e80:	95b2                	c.add	a1,a2
    5e82:	00357713          	andi	a4,a0,3
    5e86:	0035f693          	andi	a3,a1,3
    5e8a:	0ad71363          	bne	a4,a3,5f30 <memmove+0xc8>
    5e8e:	c6b1                	c.beqz	a3,5eda <memmove+0x72>
    5e90:	ffc57813          	andi	a6,a0,-4
    5e94:	09035e63          	bge	t1,a6,5f30 <memmove+0xc8>
    5e98:	02f04863          	bgtz	a5,5ec8 <memmove+0x60>
    5e9c:	953e                	c.add	a0,a5
    5e9e:	95be                	c.add	a1,a5
    5ea0:	983e                	c.add	a6,a5
    5ea2:	00178693          	addi	a3,a5,1
    5ea6:	ce99                	c.beqz	a3,5ec4 <memmove+0x5c>
    5ea8:	a095                	c.j	5f0c <memmove+0xa4>
    5eaa:	4785                	c.li	a5,1
    5eac:	00357713          	andi	a4,a0,3
    5eb0:	0035f693          	andi	a3,a1,3
    5eb4:	06e69e63          	bne	a3,a4,5f30 <memmove+0xc8>
    5eb8:	c28d                	c.beqz	a3,5eda <memmove+0x72>
    5eba:	ffc57813          	andi	a6,a0,-4
    5ebe:	0811                	c.addi	a6,4
    5ec0:	06685863          	bge	a6,t1,5f30 <memmove+0xc8>
    5ec4:	00a00733          	add	a4,zero,a0
    5ec8:	0005c683          	lbu	a3,0(a1)
    5ecc:	00d50023          	sb	a3,0(a0)
    5ed0:	953e                	c.add	a0,a5
    5ed2:	95be                	c.add	a1,a5
    5ed4:	ff051ae3          	bne	a0,a6,5ec8 <memmove+0x60>
    5ed8:	8f09                	c.sub	a4,a0
    5eda:	00f026b3          	sgtz	a3,a5
    5ede:	16fd                	c.addi	a3,-1
    5ee0:	c719                	c.beqz	a4,5eee <memmove+0x86>
    5ee2:	40d80833          	sub	a6,a6,a3
    5ee6:	04680d63          	beq	a6,t1,5f40 <memmove+0xd8>
    5eea:	8d15                	c.sub	a0,a3
    5eec:	8d95                	c.sub	a1,a3
    5eee:	8f35                	c.xor	a4,a3
    5ef0:	8f15                	c.sub	a4,a3
    5ef2:	963a                	c.add	a2,a4
    5ef4:	9a41                	c.andi	a2,-16
    5ef6:	ce0d                	c.beqz	a2,5f30 <memmove+0xc8>
    5ef8:	8e35                	c.xor	a2,a3
    5efa:	8e15                	c.sub	a2,a3
    5efc:	0106c793          	xori	a5,a3,16
    5f00:	40d787b3          	sub	a5,a5,a3
    5f04:	00c50833          	add	a6,a0,a2
    5f08:	f807c8e3          	bltz	a5,5e98 <memmove+0x30>
    5f0c:	4194                	c.lw	a3,0(a1)
    5f0e:	41d8                	c.lw	a4,4(a1)
    5f10:	0085a883          	lw	a7,8(a1)
    5f14:	45d0                	c.lw	a2,12(a1)
    5f16:	c114                	c.sw	a3,0(a0)
    5f18:	c158                	c.sw	a4,4(a0)
    5f1a:	01152423          	sw	a7,8(a0)
    5f1e:	c550                	c.sw	a2,12(a0)
    5f20:	95be                	c.add	a1,a5
    5f22:	953e                	c.add	a0,a5
    5f24:	ff0514e3          	bne	a0,a6,5f0c <memmove+0xa4>
    5f28:	00f04463          	bgtz	a5,5f30 <memmove+0xc8>
    5f2c:	8d1d                	c.sub	a0,a5
    5f2e:	8d9d                	c.sub	a1,a5
    5f30:	00f027b3          	sgtz	a5,a5
    5f34:	881a                	c.mv	a6,t1
    5f36:	17fd                	c.addi	a5,-1
    5f38:	0017e793          	ori	a5,a5,1
    5f3c:	f4651ee3          	bne	a0,t1,5e98 <memmove+0x30>
    5f40:	851e                	c.mv	a0,t2
    5f42:	8082                	c.jr	ra

00005f44 <memset>:
    5f44:	832a                	c.mv	t1,a0
    5f46:	00c503b3          	add	t2,a0,a2
    5f4a:	00357693          	andi	a3,a0,3
    5f4e:	ce81                	c.beqz	a3,5f66 <memset+0x22>
    5f50:	ffc68693          	addi	a3,a3,-4
    5f54:	40d507b3          	sub	a5,a0,a3
    5f58:	06f3e463          	bltu	t2,a5,5fc0 <memset+0x7c>
    5f5c:	00b50023          	sb	a1,0(a0)
    5f60:	0505                	c.addi	a0,1
    5f62:	fea79de3          	bne	a5,a0,5f5c <memset+0x18>
    5f66:	04750d63          	beq	a0,t2,5fc0 <memset+0x7c>
    5f6a:	05e2                	c.slli	a1,0x18
    5f6c:	0085d793          	srli	a5,a1,0x8
    5f70:	95be                	c.add	a1,a5
    5f72:	0105d793          	srli	a5,a1,0x10
    5f76:	95be                	c.add	a1,a5
    5f78:	40a38633          	sub	a2,t2,a0
    5f7c:	02000713          	li	a4,32
    5f80:	fe067693          	andi	a3,a2,-32
    5f84:	00a687b3          	add	a5,a3,a0
    5f88:	e285                	c.bnez	a3,5fa8 <memset+0x64>
    5f8a:	ffc67693          	andi	a3,a2,-4
    5f8e:	ca8d                	c.beqz	a3,5fc0 <memset+0x7c>
    5f90:	40d70633          	sub	a2,a4,a3
    5f94:	00a687b3          	add	a5,a3,a0
    5f98:	8736                	c.mv	a4,a3
    5f9a:	8205                	c.srli	a2,0x1
    5f9c:	00000697          	auipc	a3,0x0
    5fa0:	00c686b3          	add	a3,a3,a2
    5fa4:	00c68067          	jr	12(a3) # 5fa8 <memset+0x64>
    5fa8:	cd4c                	c.sw	a1,28(a0)
    5faa:	cd0c                	c.sw	a1,24(a0)
    5fac:	c94c                	c.sw	a1,20(a0)
    5fae:	c90c                	c.sw	a1,16(a0)
    5fb0:	c54c                	c.sw	a1,12(a0)
    5fb2:	c50c                	c.sw	a1,8(a0)
    5fb4:	c14c                	c.sw	a1,4(a0)
    5fb6:	c10c                	c.sw	a1,0(a0)
    5fb8:	953a                	c.add	a0,a4
    5fba:	fef517e3          	bne	a0,a5,5fa8 <memset+0x64>
    5fbe:	bf6d                	c.j	5f78 <memset+0x34>
    5fc0:	879e                	c.mv	a5,t2
    5fc2:	f8751de3          	bne	a0,t2,5f5c <memset+0x18>
    5fc6:	851a                	c.mv	a0,t1
    5fc8:	8082                	c.jr	ra

00005fca <_free_r>:
    5fca:	c5a5                	c.beqz	a1,6032 <_free_r+0x68>
    5fcc:	ffc5a703          	lw	a4,-4(a1)
    5fd0:	ffc58793          	addi	a5,a1,-4
    5fd4:	00075363          	bgez	a4,5fda <_free_r+0x10>
    5fd8:	97ba                	c.add	a5,a4
    5fda:	89bfa72b          	lwgp	a4,-1896 # 30000190 <__malloc_free_list>
    5fde:	85b6                	c.mv	a1,a3
    5fe0:	e709                	c.bnez	a4,5fea <_free_r+0x20>
    5fe2:	0007a223          	sw	zero,4(a5)
    5fe6:	8c60                	exec.it	#54     !swgp	a5,-1896 # 30000190 <__malloc_free_list>
    5fe8:	8082                	c.jr	ra
    5fea:	00e7fe63          	bgeu	a5,a4,6006 <_free_r+0x3c>
    5fee:	4390                	c.lw	a2,0(a5)
    5ff0:	00c786b3          	add	a3,a5,a2
    5ff4:	00d71663          	bne	a4,a3,6000 <_free_r+0x36>
    5ff8:	4314                	c.lw	a3,0(a4)
    5ffa:	4358                	c.lw	a4,4(a4)
    5ffc:	96b2                	c.add	a3,a2
    5ffe:	c394                	c.sw	a3,0(a5)
    6000:	c3d8                	c.sw	a4,4(a5)
    6002:	8c60                	exec.it	#54     !swgp	a5,-1896 # 30000190 <__malloc_free_list>
    6004:	8082                	c.jr	ra
    6006:	86ba                	c.mv	a3,a4
    6008:	4358                	c.lw	a4,4(a4)
    600a:	c319                	c.beqz	a4,6010 <_free_r+0x46>
    600c:	fee7fde3          	bgeu	a5,a4,6006 <_free_r+0x3c>
    6010:	4290                	c.lw	a2,0(a3)
    6012:	00c685b3          	add	a1,a3,a2
    6016:	00f59f63          	bne	a1,a5,6034 <_free_r+0x6a>
    601a:	439c                	c.lw	a5,0(a5)
    601c:	97b2                	c.add	a5,a2
    601e:	00f68633          	add	a2,a3,a5
    6022:	c29c                	c.sw	a5,0(a3)
    6024:	00c71763          	bne	a4,a2,6032 <_free_r+0x68>
    6028:	4310                	c.lw	a2,0(a4)
    602a:	97b2                	c.add	a5,a2
    602c:	c29c                	c.sw	a5,0(a3)
    602e:	435c                	c.lw	a5,4(a4)
    6030:	c2dc                	c.sw	a5,4(a3)
    6032:	8082                	c.jr	ra
    6034:	00b7f563          	bgeu	a5,a1,603e <_free_r+0x74>
    6038:	4731                	c.li	a4,12
    603a:	8a04                	exec.it	#76     !swgp	a4,-1884 # 3000019c <__mculib_REENT_errno>
    603c:	8082                	c.jr	ra
    603e:	438c                	c.lw	a1,0(a5)
    6040:	00b78633          	add	a2,a5,a1
    6044:	00c71663          	bne	a4,a2,6050 <_free_r+0x86>
    6048:	4310                	c.lw	a2,0(a4)
    604a:	4358                	c.lw	a4,4(a4)
    604c:	962e                	c.add	a2,a1
    604e:	c390                	c.sw	a2,0(a5)
    6050:	c3d8                	c.sw	a4,4(a5)
    6052:	bff9                	c.j	6030 <_free_r+0x66>

00006054 <_malloc_r>:
    6054:	d01ff2ef          	jal	t0,5d54 <__riscv_save_0>
    6058:	00358493          	addi	s1,a1,3
    605c:	98f1                	c.andi	s1,-4
    605e:	04a1                	c.addi	s1,8
    6060:	47b1                	c.li	a5,12
    6062:	04f4f163          	bgeu	s1,a5,60a4 <_malloc_r+0x50>
    6066:	44b1                	c.li	s1,12
    6068:	04b4e063          	bltu	s1,a1,60a8 <_malloc_r+0x54>
    606c:	89bfa72b          	lwgp	a4,-1896 # 30000190 <__malloc_free_list>
    6070:	86be                	c.mv	a3,a5
    6072:	843a                	c.mv	s0,a4
    6074:	ec1d                	c.bnez	s0,60b2 <_malloc_r+0x5e>
    6076:	897fa7ab          	lwgp	a5,-1900 # 3000018c <__malloc_sbrk_start>
    607a:	e789                	c.bnez	a5,6084 <_malloc_r+0x30>
    607c:	4501                	c.li	a0,0
    607e:	8234                	exec.it	#89     !jal	ra,6c4c <_sbrk>
    6080:	88afcbab          	swgp	a0,-1900 # 3000018c <__malloc_sbrk_start>
    6084:	8526                	c.mv	a0,s1
    6086:	597d                	c.li	s2,-1
    6088:	8234                	exec.it	#89     !jal	ra,6c4c <_sbrk>
    608a:	01250f63          	beq	a0,s2,60a8 <_malloc_r+0x54>
    608e:	00350413          	addi	s0,a0,3
    6092:	9871                	c.andi	s0,-4
    6094:	02850863          	beq	a0,s0,60c4 <_malloc_r+0x70>
    6098:	40a40533          	sub	a0,s0,a0
    609c:	8234                	exec.it	#89     !jal	ra,6c4c <_sbrk>
    609e:	03251363          	bne	a0,s2,60c4 <_malloc_r+0x70>
    60a2:	a019                	c.j	60a8 <_malloc_r+0x54>
    60a4:	fc04d2e3          	bgez	s1,6068 <_malloc_r+0x14>
    60a8:	4731                	c.li	a4,12
    60aa:	4501                	c.li	a0,0
    60ac:	8a04                	exec.it	#76     !swgp	a4,-1884 # 3000019c <__mculib_REENT_errno>
    60ae:	ccbff06f          	j	5d78 <__riscv_restore_0>
    60b2:	401c                	c.lw	a5,0(s0)
    60b4:	8f85                	c.sub	a5,s1
    60b6:	0207cc63          	bltz	a5,60ee <_malloc_r+0x9a>
    60ba:	462d                	c.li	a2,11
    60bc:	00f67663          	bgeu	a2,a5,60c8 <_malloc_r+0x74>
    60c0:	c01c                	c.sw	a5,0(s0)
    60c2:	943e                	c.add	s0,a5
    60c4:	c004                	c.sw	s1,0(s0)
    60c6:	a029                	c.j	60d0 <_malloc_r+0x7c>
    60c8:	405c                	c.lw	a5,4(s0)
    60ca:	02871063          	bne	a4,s0,60ea <_malloc_r+0x96>
    60ce:	8c60                	exec.it	#54     !swgp	a5,-1896 # 30000190 <__malloc_free_list>
    60d0:	00b40513          	addi	a0,s0,11
    60d4:	00440793          	addi	a5,s0,4
    60d8:	9961                	c.andi	a0,-8
    60da:	40f50733          	sub	a4,a0,a5
    60de:	fcf508e3          	beq	a0,a5,60ae <_malloc_r+0x5a>
    60e2:	943a                	c.add	s0,a4
    60e4:	8f89                	c.sub	a5,a0
    60e6:	c01c                	c.sw	a5,0(s0)
    60e8:	b7d9                	c.j	60ae <_malloc_r+0x5a>
    60ea:	c35c                	c.sw	a5,4(a4)
    60ec:	b7d5                	c.j	60d0 <_malloc_r+0x7c>
    60ee:	8722                	c.mv	a4,s0
    60f0:	4040                	c.lw	s0,4(s0)
    60f2:	b749                	c.j	6074 <_malloc_r+0x20>

000060f4 <printf>:
    60f4:	7139                	c.addi16sp	sp,-64
    60f6:	d22e                	c.swsp	a1,36(sp)
    60f8:	104c                	c.addi4spn	a1,sp,36
    60fa:	ce06                	c.swsp	ra,28(sp)
    60fc:	d432                	c.swsp	a2,40(sp)
    60fe:	d636                	c.swsp	a3,44(sp)
    6100:	d83a                	c.swsp	a4,48(sp)
    6102:	da3e                	c.swsp	a5,52(sp)
    6104:	dc42                	c.swsp	a6,56(sp)
    6106:	de46                	c.swsp	a7,60(sp)
    6108:	c62e                	c.swsp	a1,12(sp)
    610a:	22a1                	c.jal	6252 <vprintf>
    610c:	40f2                	c.lwsp	ra,28(sp)
    610e:	6121                	c.addi16sp	sp,64
    6110:	8082                	c.jr	ra

00006112 <realloc>:
    6112:	862e                	c.mv	a2,a1
    6114:	85aa                	c.mv	a1,a0
    6116:	8650                	exec.it	#99     !lwgp	a0,-2024 # 30000110 <_impure_ptr>
    6118:	a249                	c.j	629a <_realloc_r>
    611a:	0001                	c.nop

0000611c <strcmp>:
    611c:	00b56633          	or	a2,a0,a1
    6120:	8a0d                	c.andi	a2,3
    6122:	ee0d                	c.bnez	a2,615c <strcmp+0x40>
    6124:	4110                	c.lw	a2,0(a0)
    6126:	4194                	c.lw	a3,0(a1)
    6128:	22d607db          	ffzmism	a5,a2,a3
    612c:	e3b1                	c.bnez	a5,6170 <strcmp+0x54>
    612e:	4150                	c.lw	a2,4(a0)
    6130:	41d4                	c.lw	a3,4(a1)
    6132:	22d607db          	ffzmism	a5,a2,a3
    6136:	ef8d                	c.bnez	a5,6170 <strcmp+0x54>
    6138:	4510                	c.lw	a2,8(a0)
    613a:	4594                	c.lw	a3,8(a1)
    613c:	22d607db          	ffzmism	a5,a2,a3
    6140:	eb85                	c.bnez	a5,6170 <strcmp+0x54>
    6142:	4550                	c.lw	a2,12(a0)
    6144:	45d4                	c.lw	a3,12(a1)
    6146:	22d607db          	ffzmism	a5,a2,a3
    614a:	e39d                	c.bnez	a5,6170 <strcmp+0x54>
    614c:	4910                	c.lw	a2,16(a0)
    614e:	4994                	c.lw	a3,16(a1)
    6150:	22d607db          	ffzmism	a5,a2,a3
    6154:	ef91                	c.bnez	a5,6170 <strcmp+0x54>
    6156:	0551                	c.addi	a0,20
    6158:	05d1                	c.addi	a1,20
    615a:	b7e9                	c.j	6124 <strcmp+0x8>
    615c:	00054603          	lbu	a2,0(a0)
    6160:	0005c683          	lbu	a3,0(a1)
    6164:	0505                	c.addi	a0,1
    6166:	0585                	c.addi	a1,1
    6168:	00d61d63          	bne	a2,a3,6182 <strcmp+0x66>
    616c:	fe0618e3          	bnez	a2,615c <strcmp+0x40>
    6170:	078e                	c.slli	a5,0x3
    6172:	00f65633          	srl	a2,a2,a5
    6176:	0ff67613          	andi	a2,a2,255
    617a:	00f6d6b3          	srl	a3,a3,a5
    617e:	0ff6f693          	andi	a3,a3,255
    6182:	40d60533          	sub	a0,a2,a3
    6186:	8082                	c.jr	ra
    6188:	0001                	c.nop
    618a:	0001                	c.nop

0000618c <strlen>:
    618c:	87aa                	c.mv	a5,a0
    618e:	c11d                	c.beqz	a0,61b4 <strlen+0x28>
    6190:	00357693          	andi	a3,a0,3
    6194:	ca91                	c.beqz	a3,61a8 <strlen+0x1c>
    6196:	16f1                	c.addi	a3,-4
    6198:	00054583          	lbu	a1,0(a0)
    619c:	0685                	c.addi	a3,1
    619e:	c689                	c.beqz	a3,61a8 <strlen+0x1c>
    61a0:	0505                	c.addi	a0,1
    61a2:	f9fd                	c.bnez	a1,6198 <strlen+0xc>
    61a4:	157d                	c.addi	a0,-1
    61a6:	a039                	c.j	61b4 <strlen+0x28>
    61a8:	410c                	c.lw	a1,0(a0)
    61aa:	200585db          	ffb	a1,a1,zero
    61ae:	0511                	c.addi	a0,4
    61b0:	dde5                	c.beqz	a1,61a8 <strlen+0x1c>
    61b2:	952e                	c.add	a0,a1
    61b4:	8d1d                	c.sub	a0,a5
    61b6:	8082                	c.jr	ra
    61b8:	0001                	c.nop
    61ba:	0001                	c.nop

000061bc <strncmp>:
    61bc:	00c50333          	add	t1,a0,a2
    61c0:	00357693          	andi	a3,a0,3
    61c4:	00357713          	andi	a4,a0,3
    61c8:	04e69363          	bne	a3,a4,620e <strncmp+0x52>
    61cc:	02068463          	beqz	a3,61f4 <strncmp+0x38>
    61d0:	ffc68793          	addi	a5,a3,-4
    61d4:	40f503b3          	sub	t2,a0,a5
    61d8:	02734b63          	blt	t1,t2,620e <strncmp+0x52>
    61dc:	00054683          	lbu	a3,0(a0)
    61e0:	0505                	c.addi	a0,1
    61e2:	0005c703          	lbu	a4,0(a1)
    61e6:	0585                	c.addi	a1,1
    61e8:	02e69663          	bne	a3,a4,6214 <strncmp+0x58>
    61ec:	02068463          	beqz	a3,6214 <strncmp+0x58>
    61f0:	fe7516e3          	bne	a0,t2,61dc <strncmp+0x20>
    61f4:	ffc37393          	andi	t2,t1,-4
    61f8:	00755b63          	bge	a0,t2,620e <strncmp+0x52>
    61fc:	4114                	c.lw	a3,0(a0)
    61fe:	4198                	c.lw	a4,0(a1)
    6200:	22e687db          	ffzmism	a5,a3,a4
    6204:	e789                	c.bnez	a5,620e <strncmp+0x52>
    6206:	0511                	c.addi	a0,4
    6208:	0591                	c.addi	a1,4
    620a:	fe7519e3          	bne	a0,t2,61fc <strncmp+0x40>
    620e:	839a                	c.mv	t2,t1
    6210:	fca396e3          	bne	t2,a0,61dc <strncmp+0x20>
    6214:	40e68533          	sub	a0,a3,a4
    6218:	8082                	c.jr	ra

0000621a <__vprintf_help>:
    621a:	b3bff2ef          	jal	t0,5d54 <__riscv_save_0>
    621e:	8abfa7ab          	lwgp	a5,-1880 # 300001a0 <__printf_count>
    6222:	0785                	c.addi	a5,1
    6224:	8affc5ab          	swgp	a5,-1880 # 300001a0 <__printf_count>
    6228:	8a3fa5ab          	lwgp	a1,-1888 # 30000198 <__vprintf_buf>
    622c:	89ffa7ab          	lwgp	a5,-1892 # 30000194 <__vprintf_buf_len>
    6230:	00178713          	addi	a4,a5,1
    6234:	97ae                	c.add	a5,a1
    6236:	88efcfab          	swgp	a4,-1892 # 30000194 <__vprintf_buf_len>
    623a:	00a78023          	sb	a0,0(a5)
    623e:	4007685b          	bnec	a4,64,624e <__vprintf_help+0x34>
    6242:	8410                	exec.it	#3     !li	a2,64
    6244:	4505                	c.li	a0,1
    6246:	231000ef          	jal	ra,6c76 <_write>
    624a:	880fcfab          	swgp	zero,-1892 # 30000194 <__vprintf_buf_len>
    624e:	b2bff06f          	j	5d78 <__riscv_restore_0>

00006252 <vprintf>:
    6252:	1141                	c.addi	sp,-16
    6254:	c606                	c.swsp	ra,12(sp)
    6256:	c422                	c.swsp	s0,8(sp)
    6258:	c226                	c.swsp	s1,4(sp)
    625a:	0800                	c.addi4spn	s0,sp,16
    625c:	c04a                	c.swsp	s2,0(sp)
    625e:	715d                	c.addi16sp	sp,-80
    6260:	00f10793          	addi	a5,sp,15
    6264:	00006637          	lui	a2,0x6
    6268:	9bc1                	c.andi	a5,-16
    626a:	21a60613          	addi	a2,a2,538 # 621a <__vprintf_help>
    626e:	8affc1ab          	swgp	a5,-1888 # 30000198 <__vprintf_buf>
    6272:	880fcfab          	swgp	zero,-1892 # 30000194 <__vprintf_buf_len>
    6276:	285d                	c.jal	632c <__printf_impl>
    6278:	89ffa62b          	lwgp	a2,-1892 # 30000194 <__vprintf_buf_len>
    627c:	8a3fa5ab          	lwgp	a1,-1888 # 30000198 <__vprintf_buf>
    6280:	4505                	c.li	a0,1
    6282:	1f5000ef          	jal	ra,6c76 <_write>
    6286:	8abfa52b          	lwgp	a0,-1880 # 300001a0 <__printf_count>
    628a:	ff040113          	addi	sp,s0,-16
    628e:	40b2                	c.lwsp	ra,12(sp)
    6290:	4422                	c.lwsp	s0,8(sp)
    6292:	4492                	c.lwsp	s1,4(sp)
    6294:	4902                	c.lwsp	s2,0(sp)
    6296:	0141                	c.addi	sp,16
    6298:	8082                	c.jr	ra

0000629a <_realloc_r>:
    629a:	8060                	exec.it	#48     !jal	t0,5d3a <__riscv_save_4>
    629c:	84b2                	c.mv	s1,a2
    629e:	e599                	c.bnez	a1,62ac <_realloc_r+0x12>
    62a0:	85b2                	c.mv	a1,a2
    62a2:	db3ff0ef          	jal	ra,6054 <_malloc_r>
    62a6:	842a                	c.mv	s0,a0
    62a8:	8522                	c.mv	a0,s0
    62aa:	8440                	exec.it	#34     !j	5d6e <__riscv_restore_4>
    62ac:	e609                	c.bnez	a2,62b6 <_realloc_r+0x1c>
    62ae:	4401                	c.li	s0,0
    62b0:	d1bff0ef          	jal	ra,5fca <_free_r>
    62b4:	bfd5                	c.j	62a8 <_realloc_r+0xe>
    62b6:	892e                	c.mv	s2,a1
    62b8:	89aa                	c.mv	s3,a0
    62ba:	844a                	c.mv	s0,s2
    62bc:	17d000ef          	jal	ra,6c38 <_malloc_usable_size_r>
    62c0:	fe9574e3          	bgeu	a0,s1,62a8 <_realloc_r+0xe>
    62c4:	85a6                	c.mv	a1,s1
    62c6:	854e                	c.mv	a0,s3
    62c8:	d8dff0ef          	jal	ra,6054 <_malloc_r>
    62cc:	842a                	c.mv	s0,a0
    62ce:	dd69                	c.beqz	a0,62a8 <_realloc_r+0xe>
    62d0:	85ca                	c.mv	a1,s2
    62d2:	8626                	c.mv	a2,s1
    62d4:	8000                	exec.it	#0     !jal	ra,5dd4 <memcpy>
    62d6:	85ca                	c.mv	a1,s2
    62d8:	854e                	c.mv	a0,s3
    62da:	cf1ff0ef          	jal	ra,5fca <_free_r>
    62de:	b7e9                	c.j	62a8 <_realloc_r+0xe>

000062e0 <__print_integral>:
    62e0:	a75ff2ef          	jal	t0,5d54 <__riscv_save_0>
    62e4:	1101                	c.addi	sp,-32
    62e6:	842a                	c.mv	s0,a0
    62e8:	84ae                	c.mv	s1,a1
    62ea:	8932                	c.mv	s2,a2
    62ec:	c636                	c.swsp	a3,12(sp)
    62ee:	853a                	c.mv	a0,a4
    62f0:	85be                	c.mv	a1,a5
    62f2:	0830                	c.addi4spn	a2,sp,24
    62f4:	ca3e                	c.swsp	a5,20(sp)
    62f6:	21bd                	c.jal	6764 <__int_to_poly>
    62f8:	46b2                	c.lwsp	a3,12(sp)
    62fa:	882a                	c.mv	a6,a0
    62fc:	083c                	c.addi4spn	a5,sp,24
    62fe:	0858                	c.addi4spn	a4,sp,20
    6300:	864a                	c.mv	a2,s2
    6302:	85a6                	c.mv	a1,s1
    6304:	8522                	c.mv	a0,s0
    6306:	2141                	c.jal	6786 <__print_integral_poly>
    6308:	000087b7          	lui	a5,0x8
    630c:	61078793          	addi	a5,a5,1552 # 8610 <__digits>
    6310:	00a46563          	bltu	s0,a0,631a <__print_integral+0x3a>
    6314:	6105                	c.addi16sp	sp,32
    6316:	a63ff06f          	j	5d78 <__riscv_restore_0>
    631a:	00044703          	lbu	a4,0(s0)
    631e:	0405                	c.addi	s0,1
    6320:	973e                	c.add	a4,a5
    6322:	00070703          	lb	a4,0(a4)
    6326:	fee40fa3          	sb	a4,-1(s0)
    632a:	b7dd                	c.j	6310 <__print_integral+0x30>

0000632c <__printf_impl>:
    632c:	8024                	exec.it	#24     !jal	t0,5d24 <__riscv_save_12>
    632e:	a6010113          	addi	sp,sp,-1440
    6332:	6b85                	c.lui	s7,0x1
    6334:	00008c37          	lui	s8,0x8
    6338:	842e                	c.mv	s0,a1
    633a:	620c0593          	addi	a1,s8,1568 # 8620 <__digits+0x10>
    633e:	892a                	c.mv	s2,a0
    6340:	8a32                	c.mv	s4,a2
    6342:	01c10a93          	addi	s5,sp,28
    6346:	800b8b93          	addi	s7,s7,-2048 # 800 <__rtos_signature_freertos_v10_3+0x800>
    634a:	8a0fc5ab          	swgp	zero,-1880 # 300001a0 <__printf_count>
    634e:	c42e                	c.swsp	a1,8(sp)
    6350:	00094503          	lbu	a0,0(s2)
    6354:	e501                	c.bnez	a0,635c <__printf_impl+0x30>
    6356:	5a010113          	addi	sp,sp,1440
    635a:	8c34                	exec.it	#31     !j	5d60 <__riscv_restore_12>
    635c:	00190493          	addi	s1,s2,1
    6360:	005555db          	beqc	a0,37,636a <__printf_impl+0x3e>
    6364:	9a02                	c.jalr	s4
    6366:	8926                	c.mv	s2,s1
    6368:	b7e5                	c.j	6350 <__printf_impl+0x24>
    636a:	00194783          	lbu	a5,1(s2)
    636e:	0057e8db          	bnec	a5,37,637e <__printf_impl+0x52>
    6372:	02500513          	li	a0,37
    6376:	00290493          	addi	s1,s2,2
    637a:	9a02                	c.jalr	s4
    637c:	b7ed                	c.j	6366 <__printf_impl+0x3a>
    637e:	4681                	c.li	a3,0
    6380:	4701                	c.li	a4,0
    6382:	d002                	c.swsp	zero,32(sp)
    6384:	8434                	exec.it	#27     !lbu	a5,0(s1)
    6386:	0007e7db          	bnec	a5,32,6394 <__printf_impl+0x68>
    638a:	00276713          	ori	a4,a4,2
    638e:	0485                	c.addi	s1,1
    6390:	4685                	c.li	a3,1
    6392:	bfcd                	c.j	6384 <__printf_impl+0x58>
    6394:	00d7e5db          	bnec	a5,45,639e <__printf_impl+0x72>
    6398:	00476713          	ori	a4,a4,4
    639c:	bfcd                	c.j	638e <__printf_impl+0x62>
    639e:	00b7e5db          	bnec	a5,43,63a8 <__printf_impl+0x7c>
    63a2:	00876713          	ori	a4,a4,8
    63a6:	b7e5                	c.j	638e <__printf_impl+0x62>
    63a8:	0037e5db          	bnec	a5,35,63b2 <__printf_impl+0x86>
    63ac:	01076713          	ori	a4,a4,16
    63b0:	bff9                	c.j	638e <__printf_impl+0x62>
    63b2:	0107e5db          	bnec	a5,48,63bc <__printf_impl+0x90>
    63b6:	02076713          	ori	a4,a4,32
    63ba:	bfd1                	c.j	638e <__printf_impl+0x62>
    63bc:	c291                	c.beqz	a3,63c0 <__printf_impl+0x94>
    63be:	d03a                	c.swsp	a4,32(sp)
    63c0:	04a7e7db          	bnec	a5,42,640e <__printf_impl+0xe2>
    63c4:	00042983          	lw	s3,0(s0)
    63c8:	00440713          	addi	a4,s0,4
    63cc:	0009d863          	bgez	s3,63dc <__printf_impl+0xb0>
    63d0:	5782                	c.lwsp	a5,32(sp)
    63d2:	413009b3          	neg	s3,s3
    63d6:	0047e793          	ori	a5,a5,4
    63da:	d03e                	c.swsp	a5,32(sp)
    63dc:	0485                	c.addi	s1,1
    63de:	843a                	c.mv	s0,a4
    63e0:	8434                	exec.it	#27     !lbu	a5,0(s1)
    63e2:	5d7d                	c.li	s10,-1
    63e4:	06e7e2db          	bnec	a5,46,6448 <__printf_impl+0x11c>
    63e8:	0014c783          	lbu	a5,1(s1)
    63ec:	04a7d5db          	beqc	a5,42,6436 <__printf_impl+0x10a>
    63f0:	0485                	c.addi	s1,1
    63f2:	4d01                	c.li	s10,0
    63f4:	4725                	c.li	a4,9
    63f6:	4629                	c.li	a2,10
    63f8:	8434                	exec.it	#27     !lbu	a5,0(s1)
    63fa:	8644                	exec.it	#106     !addi	a5,a5,-48
    63fc:	0ff7f693          	andi	a3,a5,255
    6400:	04d76463          	bltu	a4,a3,6448 <__printf_impl+0x11c>
    6404:	02cd0d33          	mul	s10,s10,a2
    6408:	0485                	c.addi	s1,1
    640a:	9d3e                	c.add	s10,a5
    640c:	b7f5                	c.j	63f8 <__printf_impl+0xcc>
    640e:	8644                	exec.it	#106     !addi	a5,a5,-48
    6410:	8e24                	exec.it	#94     !andi	a5,a5,255
    6412:	4725                	c.li	a4,9
    6414:	4981                	c.li	s3,0
    6416:	fcf765e3          	bltu	a4,a5,63e0 <__printf_impl+0xb4>
    641a:	4725                	c.li	a4,9
    641c:	4629                	c.li	a2,10
    641e:	a029                	c.j	6428 <__printf_impl+0xfc>
    6420:	02c989b3          	mul	s3,s3,a2
    6424:	0485                	c.addi	s1,1
    6426:	99be                	c.add	s3,a5
    6428:	8434                	exec.it	#27     !lbu	a5,0(s1)
    642a:	8644                	exec.it	#106     !addi	a5,a5,-48
    642c:	0ff7f693          	andi	a3,a5,255
    6430:	fed778e3          	bgeu	a4,a3,6420 <__printf_impl+0xf4>
    6434:	b775                	c.j	63e0 <__printf_impl+0xb4>
    6436:	00042d03          	lw	s10,0(s0)
    643a:	00440793          	addi	a5,s0,4
    643e:	000d5363          	bgez	s10,6444 <__printf_impl+0x118>
    6442:	5d7d                	c.li	s10,-1
    6444:	0489                	c.addi	s1,2
    6446:	843e                	c.mv	s0,a5
    6448:	43200793          	li	a5,1074
    644c:	866a                	c.mv	a2,s10
    644e:	01a7d463          	bge	a5,s10,6456 <__printf_impl+0x12a>
    6452:	43200613          	li	a2,1074
    6456:	8434                	exec.it	#27     !lbu	a5,0(s1)
    6458:	4687e3db          	bnec	a5,104,64be <__printf_impl+0x192>
    645c:	5782                	c.lwsp	a5,32(sp)
    645e:	0014c703          	lbu	a4,1(s1)
    6462:	448768db          	bnec	a4,104,64b2 <__printf_impl+0x186>
    6466:	0407e793          	ori	a5,a5,64
    646a:	00248913          	addi	s2,s1,2
    646e:	d03e                	c.swsp	a5,32(sp)
    6470:	00094683          	lbu	a3,0(s2)
    6474:	07800793          	li	a5,120
    6478:	22d7e663          	bltu	a5,a3,66a4 <__printf_impl+0x378>
    647c:	05700793          	li	a5,87
    6480:	06d7e963          	bltu	a5,a3,64f2 <__printf_impl+0x1c6>
    6484:	4016d85b          	beqc	a3,65,6494 <__printf_impl+0x168>
    6488:	fbb68793          	addi	a5,a3,-69
    648c:	8e24                	exec.it	#94     !andi	a5,a5,255
    648e:	4709                	c.li	a4,2
    6490:	20f76a63          	bltu	a4,a5,66a4 <__printf_impl+0x378>
    6494:	00740593          	addi	a1,s0,7
    6498:	99e1                	c.andi	a1,-8
    649a:	41d8                	c.lw	a4,4(a1)
    649c:	87b6                	c.mv	a5,a3
    649e:	0206fc93          	andi	s9,a3,32
    64a2:	4194                	c.lw	a3,0(a1)
    64a4:	00858413          	addi	s0,a1,8
    64a8:	1048                	c.addi4spn	a0,sp,36
    64aa:	100c                	c.addi4spn	a1,sp,32
    64ac:	2eb9                	c.jal	680a <__printf_float_impl>
    64ae:	84aa                	c.mv	s1,a0
    64b0:	a8ad                	c.j	652a <__printf_impl+0x1fe>
    64b2:	0807e793          	ori	a5,a5,128
    64b6:	00148913          	addi	s2,s1,1
    64ba:	d03e                	c.swsp	a5,32(sp)
    64bc:	bf55                	c.j	6470 <__printf_impl+0x144>
    64be:	40c7eddb          	bnec	a5,108,64d8 <__printf_impl+0x1ac>
    64c2:	5782                	c.lwsp	a5,32(sp)
    64c4:	0014c703          	lbu	a4,1(s1)
    64c8:	40c765db          	bnec	a4,108,64d2 <__printf_impl+0x1a6>
    64cc:	2007e793          	ori	a5,a5,512
    64d0:	bf69                	c.j	646a <__printf_impl+0x13e>
    64d2:	1007e793          	ori	a5,a5,256
    64d6:	b7c5                	c.j	64b6 <__printf_impl+0x18a>
    64d8:	00148913          	addi	s2,s1,1
    64dc:	41a7e7db          	bnec	a5,122,64ea <__printf_impl+0x1be>
    64e0:	5782                	c.lwsp	a5,32(sp)
    64e2:	1007e793          	ori	a5,a5,256
    64e6:	d03e                	c.swsp	a5,32(sp)
    64e8:	b761                	c.j	6470 <__printf_impl+0x144>
    64ea:	ff47dbdb          	beqc	a5,116,64e0 <__printf_impl+0x1b4>
    64ee:	8926                	c.mv	s2,s1
    64f0:	b741                	c.j	6470 <__printf_impl+0x144>
    64f2:	fa868793          	addi	a5,a3,-88
    64f6:	8e24                	exec.it	#94     !andi	a5,a5,255
    64f8:	02000713          	li	a4,32
    64fc:	1af76463          	bltu	a4,a5,66a4 <__printf_impl+0x378>
    6500:	45a2                	c.lwsp	a1,8(sp)
    6502:	0cf587db          	lea.w	a5,a1,a5
    6506:	439c                	c.lw	a5,0(a5)
    6508:	8782                	c.jr	a5
    650a:	45a9                	c.li	a1,10
    650c:	5782                	c.lwsp	a5,32(sp)
    650e:	1067ff5b          	bbc	a5,6,662c <__printf_impl+0x300>
    6512:	00044703          	lbu	a4,0(s0)
    6516:	8030                	exec.it	#17     !addi	s11,s0,4
    6518:	4781                	c.li	a5,0
    651a:	aa25                	c.j	6652 <__printf_impl+0x326>
    651c:	401c                	c.lw	a5,0(s0)
    651e:	4c81                	c.li	s9,0
    6520:	0411                	c.addi	s0,4
    6522:	02510493          	addi	s1,sp,37
    6526:	02f10223          	sb	a5,36(sp)
    652a:	02410c13          	addi	s8,sp,36
    652e:	a025                	c.j	6556 <__printf_impl+0x22a>
    6530:	00042c03          	lw	s8,0(s0)
    6534:	85b2                	c.mv	a1,a2
    6536:	8562                	c.mv	a0,s8
    6538:	c632                	c.swsp	a2,12(sp)
    653a:	25e5                	c.jal	6c22 <strnlen>
    653c:	57fd                	c.li	a5,-1
    653e:	00440c93          	addi	s9,s0,4
    6542:	00fd0663          	beq	s10,a5,654e <__printf_impl+0x222>
    6546:	4632                	c.lwsp	a2,12(sp)
    6548:	00a65363          	bge	a2,a0,654e <__printf_impl+0x222>
    654c:	8532                	c.mv	a0,a2
    654e:	8466                	c.mv	s0,s9
    6550:	00ac04b3          	add	s1,s8,a0
    6554:	4c81                	c.li	s9,0
    6556:	5782                	c.lwsp	a5,32(sp)
    6558:	02d00693          	li	a3,45
    655c:	40a7fb5b          	bbs	a5,10,6572 <__printf_impl+0x246>
    6560:	02b00693          	li	a3,43
    6564:	4037f75b          	bbs	a5,3,6572 <__printf_impl+0x246>
    6568:	8d56                	c.mv	s10,s5
    656a:	0017f85b          	bbc	a5,1,657a <__printf_impl+0x24e>
    656e:	02000693          	li	a3,32
    6572:	01d10d13          	addi	s10,sp,29
    6576:	00d10e23          	sb	a3,28(sp)
    657a:	0177f6b3          	and	a3,a5,s7
    657e:	ca91                	c.beqz	a3,6592 <__printf_impl+0x266>
    6580:	03000693          	li	a3,48
    6584:	00dd0023          	sb	a3,0(s10)
    6588:	05800693          	li	a3,88
    658c:	00dd00a3          	sb	a3,1(s10)
    6590:	0d09                	c.addi	s10,2
    6592:	5627f25b          	bbs	a5,2,66f6 <__printf_impl+0x3ca>
    6596:	41848db3          	sub	s11,s1,s8
    659a:	4057f95b          	bbs	a5,5,65ac <__printf_impl+0x280>
    659e:	41b987b3          	sub	a5,s3,s11
    65a2:	415d06b3          	sub	a3,s10,s5
    65a6:	8f95                	c.sub	a5,a3
    65a8:	16f04463          	bgtz	a5,6710 <__printf_impl+0x3e4>
    65ac:	87d6                	c.mv	a5,s5
    65ae:	aabd                	c.j	672c <__printf_impl+0x400>
    65b0:	5682                	c.lwsp	a3,32(sp)
    65b2:	0266f45b          	bbc	a3,6,65da <__printf_impl+0x2ae>
    65b6:	00040703          	lb	a4,0(s0)
    65ba:	8030                	exec.it	#17     !addi	s11,s0,4
    65bc:	41f75793          	srai	a5,a4,0x1f
    65c0:	0407d263          	bgez	a5,6604 <__printf_impl+0x2d8>
    65c4:	00e035b3          	snez	a1,a4
    65c8:	40f007b3          	neg	a5,a5
    65cc:	40e00733          	neg	a4,a4
    65d0:	8f8d                	c.sub	a5,a1
    65d2:	4006e693          	ori	a3,a3,1024
    65d6:	d036                	c.swsp	a3,32(sp)
    65d8:	a035                	c.j	6604 <__printf_impl+0x2d8>
    65da:	0076f65b          	bbc	a3,7,65e6 <__printf_impl+0x2ba>
    65de:	00041703          	lh	a4,0(s0)
    65e2:	8030                	exec.it	#17     !addi	s11,s0,4
    65e4:	bfe1                	c.j	65bc <__printf_impl+0x290>
    65e6:	0086f55b          	bbc	a3,8,65f0 <__printf_impl+0x2c4>
    65ea:	4018                	c.lw	a4,0(s0)
    65ec:	8030                	exec.it	#17     !addi	s11,s0,4
    65ee:	b7f9                	c.j	65bc <__printf_impl+0x290>
    65f0:	be96fd5b          	bbc	a3,9,65ea <__printf_impl+0x2be>
    65f4:	041d                	c.addi	s0,7
    65f6:	9861                	c.andi	s0,-8
    65f8:	405c                	c.lw	a5,4(s0)
    65fa:	4018                	c.lw	a4,0(s0)
    65fc:	00840d93          	addi	s11,s0,8
    6600:	0007c963          	bltz	a5,6612 <__printf_impl+0x2e6>
    6604:	56fd                	c.li	a3,-1
    6606:	45a9                	c.li	a1,10
    6608:	4c81                	c.li	s9,0
    660a:	04dd1963          	bne	s10,a3,665c <__printf_impl+0x330>
    660e:	4605                	c.li	a2,1
    6610:	a0ad                	c.j	667a <__printf_impl+0x34e>
    6612:	fb4d                	c.bnez	a4,65c4 <__printf_impl+0x298>
    6614:	800005b7          	lui	a1,0x80000
    6618:	fab796e3          	bne	a5,a1,65c4 <__printf_impl+0x298>
    661c:	4701                	c.li	a4,0
    661e:	800007b7          	lui	a5,0x80000
    6622:	bf45                	c.j	65d2 <__printf_impl+0x2a6>
    6624:	45c1                	c.li	a1,16
    6626:	b5dd                	c.j	650c <__printf_impl+0x1e0>
    6628:	45a1                	c.li	a1,8
    662a:	b5cd                	c.j	650c <__printf_impl+0x1e0>
    662c:	0077f65b          	bbc	a5,7,6638 <__printf_impl+0x30c>
    6630:	00045703          	lhu	a4,0(s0)
    6634:	8030                	exec.it	#17     !addi	s11,s0,4
    6636:	b5cd                	c.j	6518 <__printf_impl+0x1ec>
    6638:	0087f55b          	bbc	a5,8,6642 <__printf_impl+0x316>
    663c:	4018                	c.lw	a4,0(s0)
    663e:	8030                	exec.it	#17     !addi	s11,s0,4
    6640:	bde1                	c.j	6518 <__printf_impl+0x1ec>
    6642:	be97fd5b          	bbc	a5,9,663c <__printf_impl+0x310>
    6646:	041d                	c.addi	s0,7
    6648:	9861                	c.andi	s0,-8
    664a:	4018                	c.lw	a4,0(s0)
    664c:	405c                	c.lw	a5,4(s0)
    664e:	00840d93          	addi	s11,s0,8
    6652:	0206fc93          	andi	s9,a3,32
    6656:	56fd                	c.li	a3,-1
    6658:	02dd0963          	beq	s10,a3,668a <__printf_impl+0x35e>
    665c:	5682                	c.lwsp	a3,32(sp)
    665e:	fdf6f693          	andi	a3,a3,-33
    6662:	d036                	c.swsp	a3,32(sp)
    6664:	0105eb5b          	bnec	a1,16,667a <__printf_impl+0x34e>
    6668:	5682                	c.lwsp	a3,32(sp)
    666a:	0046f85b          	bbc	a3,4,667a <__printf_impl+0x34e>
    666e:	00f76533          	or	a0,a4,a5
    6672:	c501                	c.beqz	a0,667a <__printf_impl+0x34e>
    6674:	0176e6b3          	or	a3,a3,s7
    6678:	d036                	c.swsp	a3,32(sp)
    667a:	86b2                	c.mv	a3,a2
    667c:	5602                	c.lwsp	a2,32(sp)
    667e:	1048                	c.addi4spn	a0,sp,36
    6680:	c61ff0ef          	jal	ra,62e0 <__print_integral>
    6684:	846e                	c.mv	s0,s11
    6686:	84aa                	c.mv	s1,a0
    6688:	b54d                	c.j	652a <__printf_impl+0x1fe>
    668a:	4605                	c.li	a2,1
    668c:	bfe1                	c.j	6664 <__printf_impl+0x338>
    668e:	5602                	c.lwsp	a2,32(sp)
    6690:	8abfa7ab          	lwgp	a5,-1880 # 300001a0 <__printf_count>
    6694:	4018                	c.lw	a4,0(s0)
    6696:	00440693          	addi	a3,s0,4
    669a:	0066795b          	bbc	a2,6,66ac <__printf_impl+0x380>
    669e:	00f70023          	sb	a5,0(a4)
    66a2:	8436                	c.mv	s0,a3
    66a4:	1044                	c.addi4spn	s1,sp,36
    66a6:	4c81                	c.li	s9,0
    66a8:	8c26                	c.mv	s8,s1
    66aa:	b575                	c.j	6556 <__printf_impl+0x22a>
    66ac:	0076755b          	bbc	a2,7,66b6 <__printf_impl+0x38a>
    66b0:	00f71023          	sh	a5,0(a4)
    66b4:	b7fd                	c.j	66a2 <__printf_impl+0x376>
    66b6:	c31c                	c.sw	a5,0(a4)
    66b8:	fe86755b          	bbs	a2,8,66a2 <__printf_impl+0x376>
    66bc:	be96735b          	bbc	a2,9,66a2 <__printf_impl+0x376>
    66c0:	87fd                	c.srai	a5,0x1f
    66c2:	c35c                	c.sw	a5,4(a4)
    66c4:	bff9                	c.j	66a2 <__printf_impl+0x376>
    66c6:	000dc503          	lbu	a0,0(s11)
    66ca:	0d85                	c.addi	s11,1
    66cc:	8044                	exec.it	#40     !or	a0,a0,s9
    66ce:	9a02                	c.jalr	s4
    66d0:	ffadebe3          	bltu	s11,s10,66c6 <__printf_impl+0x39a>
    66d4:	4781                	c.li	a5,0
    66d6:	015d6463          	bltu	s10,s5,66de <__printf_impl+0x3b2>
    66da:	41aa87b3          	sub	a5,s5,s10
    66de:	99be                	c.add	s3,a5
    66e0:	8d62                	c.mv	s10,s8
    66e2:	01a49c63          	bne	s1,s10,66fa <__printf_impl+0x3ce>
    66e6:	418484b3          	sub	s1,s1,s8
    66ea:	409984b3          	sub	s1,s3,s1
    66ee:	00904c63          	bgtz	s1,6706 <__printf_impl+0x3da>
    66f2:	0905                	c.addi	s2,1
    66f4:	b9b1                	c.j	6350 <__printf_impl+0x24>
    66f6:	8dd6                	c.mv	s11,s5
    66f8:	bfe1                	c.j	66d0 <__printf_impl+0x3a4>
    66fa:	000d4503          	lbu	a0,0(s10)
    66fe:	0d05                	c.addi	s10,1
    6700:	8044                	exec.it	#40     !or	a0,a0,s9
    6702:	9a02                	c.jalr	s4
    6704:	bff9                	c.j	66e2 <__printf_impl+0x3b6>
    6706:	02000513          	li	a0,32
    670a:	14fd                	c.addi	s1,-1
    670c:	9a02                	c.jalr	s4
    670e:	b7c5                	c.j	66ee <__printf_impl+0x3c2>
    6710:	02000513          	li	a0,32
    6714:	c63e                	c.swsp	a5,12(sp)
    6716:	9a02                	c.jalr	s4
    6718:	47b2                	c.lwsp	a5,12(sp)
    671a:	17fd                	c.addi	a5,-1
    671c:	b571                	c.j	65a8 <__printf_impl+0x27c>
    671e:	0007c503          	lbu	a0,0(a5) # 80000000 <_stack+0x4ffa0000>
    6722:	c63e                	c.swsp	a5,12(sp)
    6724:	8044                	exec.it	#40     !or	a0,a0,s9
    6726:	9a02                	c.jalr	s4
    6728:	47b2                	c.lwsp	a5,12(sp)
    672a:	0785                	c.addi	a5,1
    672c:	ffa7e9e3          	bltu	a5,s10,671e <__printf_impl+0x3f2>
    6730:	4781                	c.li	a5,0
    6732:	015d6463          	bltu	s10,s5,673a <__printf_impl+0x40e>
    6736:	41aa87b3          	sub	a5,s5,s10
    673a:	99be                	c.add	s3,a5
    673c:	5782                	c.lwsp	a5,32(sp)
    673e:	0057f65b          	bbc	a5,5,674a <__printf_impl+0x41e>
    6742:	41b989b3          	sub	s3,s3,s11
    6746:	01304a63          	bgtz	s3,675a <__printf_impl+0x42e>
    674a:	fb8484e3          	beq	s1,s8,66f2 <__printf_impl+0x3c6>
    674e:	000c4503          	lbu	a0,0(s8)
    6752:	0c05                	c.addi	s8,1
    6754:	8044                	exec.it	#40     !or	a0,a0,s9
    6756:	9a02                	c.jalr	s4
    6758:	bfcd                	c.j	674a <__printf_impl+0x41e>
    675a:	03000513          	li	a0,48
    675e:	19fd                	c.addi	s3,-1
    6760:	9a02                	c.jalr	s4
    6762:	b7d5                	c.j	6746 <__printf_impl+0x41a>

00006764 <__int_to_poly>:
    6764:	87aa                	c.mv	a5,a0
    6766:	4501                	c.li	a0,0
    6768:	00b7e733          	or	a4,a5,a1
    676c:	e311                	c.bnez	a4,6770 <__int_to_poly+0xc>
    676e:	8082                	c.jr	ra
    6770:	0aa6075b          	lea.h	a4,a2,a0
    6774:	00f71023          	sh	a5,0(a4)
    6778:	01059713          	slli	a4,a1,0x10
    677c:	83c1                	c.srli	a5,0x10
    677e:	8fd9                	c.or	a5,a4
    6780:	81c1                	c.srli	a1,0x10
    6782:	0505                	c.addi	a0,1
    6784:	b7d5                	c.j	6768 <__int_to_poly+0x4>

00006786 <__print_integral_poly>:
    6786:	8800                	exec.it	#4     !jal	t0,5d54 <__riscv_save_0>
    6788:	84ba                	c.mv	s1,a4
    678a:	fff80893          	addi	a7,a6,-1
    678e:	0b07875b          	lea.h	a4,a5,a6
    6792:	8346                	c.mv	t1,a7
    6794:	4401                	c.li	s0,0
    6796:	8e3a                	c.mv	t3,a4
    6798:	1779                	c.addi	a4,-2
    679a:	02035563          	bgez	t1,67c4 <__print_integral_poly+0x3e>
    679e:	03005f63          	blez	a6,67dc <__print_integral_poly+0x56>
    67a2:	ffee5703          	lhu	a4,-2(t3)
    67a6:	e701                	c.bnez	a4,67ae <__print_integral_poly+0x28>
    67a8:	03105a63          	blez	a7,67dc <__print_integral_poly+0x56>
    67ac:	8846                	c.mv	a6,a7
    67ae:	8726                	c.mv	a4,s1
    67b0:	16fd                	c.addi	a3,-1
    67b2:	3fd1                	c.jal	6786 <__print_integral_poly>
    67b4:	87aa                	c.mv	a5,a0
    67b6:	0505                	c.addi	a0,1
    67b8:	00878023          	sb	s0,0(a5)
    67bc:	409c                	c.lw	a5,0(s1)
    67be:	0785                	c.addi	a5,1
    67c0:	c09c                	c.sw	a5,0(s1)
    67c2:	8c10                	exec.it	#7     !j	5d78 <__riscv_restore_0>
    67c4:	00075e83          	lhu	t4,0(a4)
    67c8:	0442                	c.slli	s0,0x10
    67ca:	9476                	c.add	s0,t4
    67cc:	02b45eb3          	divu	t4,s0,a1
    67d0:	137d                	c.addi	t1,-1
    67d2:	02b47433          	remu	s0,s0,a1
    67d6:	01d71023          	sh	t4,0(a4)
    67da:	bf7d                	c.j	6798 <__print_integral_poly+0x12>
    67dc:	c011                	c.beqz	s0,67e0 <__print_integral_poly+0x5a>
    67de:	16fd                	c.addi	a3,-1
    67e0:	00d04763          	bgtz	a3,67ee <__print_integral_poly+0x68>
    67e4:	0085ee5b          	bnec	a1,8,6800 <__print_integral_poly+0x7a>
    67e8:	00467c5b          	bbc	a2,4,6800 <__print_integral_poly+0x7a>
    67ec:	4685                	c.li	a3,1
    67ee:	4781                	c.li	a5,0
    67f0:	00f50733          	add	a4,a0,a5
    67f4:	00070023          	sb	zero,0(a4)
    67f8:	0785                	c.addi	a5,1
    67fa:	fed7cbe3          	blt	a5,a3,67f0 <__print_integral_poly+0x6a>
    67fe:	9536                	c.add	a0,a3
    6800:	dc55                	c.beqz	s0,67bc <__print_integral_poly+0x36>
    6802:	00850023          	sb	s0,0(a0)
    6806:	0505                	c.addi	a0,1
    6808:	bf55                	c.j	67bc <__print_integral_poly+0x36>

0000680a <__printf_float_impl>:
    680a:	8024                	exec.it	#24     !jal	t0,5d24 <__riscv_save_12>
    680c:	0df7fc93          	andi	s9,a5,223
    6810:	7169                	c.addi16sp	sp,-304
    6812:	8b2a                	c.mv	s6,a0
    6814:	892e                	c.mv	s2,a1
    6816:	84b2                	c.mv	s1,a2
    6818:	89b6                	c.mv	s3,a3
    681a:	8abe                	c.mv	s5,a5
    681c:	467ce65b          	bnec	s9,71,6888 <__printf_float_impl+0x7e>
    6820:	0005aa03          	lw	s4,0(a1) # 80000000 <_stack+0x4ffa0000>
    6824:	010a4a13          	xori	s4,s4,16
    6828:	104a2a5b          	bfoz	s4,s4,4,4
    682c:	0604d463          	bgez	s1,6894 <__printf_float_impl+0x8a>
    6830:	44b5                	c.li	s1,13
    6832:	401cd35b          	beqc	s9,65,6838 <__printf_float_impl+0x2e>
    6836:	4499                	c.li	s1,6
    6838:	c24e                	c.swsp	s3,4(sp)
    683a:	7947245b          	bfoz	s0,a4,30,20
    683e:	4c072ddb          	bfoz	s11,a4,19,0
    6842:	00075663          	bgez	a4,684e <__printf_float_impl+0x44>
    6846:	8450                	exec.it	#35     !lw	a5,0(s2)
    6848:	4007e793          	ori	a5,a5,1024
    684c:	8260                	exec.it	#112     !sw	a5,0(s2)
    684e:	401ce85b          	bnec	s9,65,685e <__printf_float_impl+0x54>
    6852:	8450                	exec.it	#35     !lw	a5,0(s2)
    6854:	6705                	c.lui	a4,0x1
    6856:	80070713          	addi	a4,a4,-2048 # 800 <__rtos_signature_freertos_v10_3+0x800>
    685a:	8fd9                	c.or	a5,a4
    685c:	8260                	exec.it	#112     !sw	a5,0(s2)
    685e:	7ff00793          	li	a5,2047
    6862:	04f41463          	bne	s0,a5,68aa <__printf_float_impl+0xa0>
    6866:	8450                	exec.it	#35     !lw	a5,0(s2)
    6868:	013de9b3          	or	s3,s11,s3
    686c:	fdf7f793          	andi	a5,a5,-33
    6870:	8260                	exec.it	#112     !sw	a5,0(s2)
    6872:	02098763          	beqz	s3,68a0 <__printf_float_impl+0x96>
    6876:	000085b7          	lui	a1,0x8
    687a:	6a458593          	addi	a1,a1,1700 # 86a4 <__digits+0x94>
    687e:	855a                	c.mv	a0,s6
    6880:	26b5                	c.jal	6bec <strcpy>
    6882:	050d                	c.addi	a0,3
    6884:	6155                	c.addi16sp	sp,304
    6886:	8c34                	exec.it	#31     !j	5d60 <__riscv_restore_12>
    6888:	4a01                	c.li	s4,0
    688a:	fa1ce15b          	bnec	s9,65,682c <__printf_float_impl+0x22>
    688e:	01f65a13          	srli	s4,a2,0x1f
    6892:	bf69                	c.j	682c <__printf_float_impl+0x22>
    6894:	f0d5                	c.bnez	s1,6838 <__printf_float_impl+0x2e>
    6896:	fb9c8493          	addi	s1,s9,-71
    689a:	0014b493          	seqz	s1,s1
    689e:	bf69                	c.j	6838 <__printf_float_impl+0x2e>
    68a0:	000085b7          	lui	a1,0x8
    68a4:	6a858593          	addi	a1,a1,1704 # 86a8 <__digits+0x98>
    68a8:	bfd9                	c.j	687e <__printf_float_impl+0x74>
    68aa:	e811                	c.bnez	s0,68be <__printf_float_impl+0xb4>
    68ac:	013de7b3          	or	a5,s11,s3
    68b0:	c0300413          	li	s0,-1021
    68b4:	18079163          	bnez	a5,6a36 <__printf_float_impl+0x22c>
    68b8:	03500413          	li	s0,53
    68bc:	a819                	c.j	68d2 <__printf_float_impl+0xc8>
    68be:	001007b7          	lui	a5,0x100
    68c2:	01b7edb3          	or	s11,a5,s11
    68c6:	c0240413          	addi	s0,s0,-1022
    68ca:	03400793          	li	a5,52
    68ce:	1687d263          	bge	a5,s0,6a32 <__printf_float_impl+0x228>
    68d2:	fcb40413          	addi	s0,s0,-53
    68d6:	00445c13          	srli	s8,s0,0x4
    68da:	001c1b93          	slli	s7,s8,0x1
    68de:	865e                	c.mv	a2,s7
    68e0:	4581                	c.li	a1,0
    68e2:	1008                	c.addi4spn	a0,sp,32
    68e4:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    68e6:	883d                	c.andi	s0,15
    68e8:	1a0c                	c.addi4spn	a1,sp,304
    68ea:	9bae                	c.add	s7,a1
    68ec:	8622                	c.mv	a2,s0
    68ee:	854e                	c.mv	a0,s3
    68f0:	85ee                	c.mv	a1,s11
    68f2:	8844                	exec.it	#44     !jal	ra,5cf4 <__ashldi3>
    68f4:	0c05                	c.addi	s8,1
    68f6:	100c                	c.addi4spn	a1,sp,32
    68f8:	4641                	c.li	a2,16
    68fa:	eeab9823          	sh	a0,-272(s7)
    68fe:	8e01                	c.sub	a2,s0
    6900:	0b858bdb          	lea.h	s7,a1,s8
    6904:	854e                	c.mv	a0,s3
    6906:	85ee                	c.mv	a1,s11
    6908:	2661                	c.jal	6c90 <__lshrdi3>
    690a:	865e                	c.mv	a2,s7
    690c:	3da1                	c.jal	6764 <__int_to_poly>
    690e:	4d01                	c.li	s10,0
    6910:	9c2a                	c.add	s8,a0
    6912:	57fd                	c.li	a5,-1
    6914:	ce3e                	c.swsp	a5,28(sp)
    6916:	4441                	c.li	s0,16
    6918:	401cd35b          	beqc	s9,65,691e <__printf_float_impl+0x114>
    691c:	4429                	c.li	s0,10
    691e:	8862                	c.mv	a6,s8
    6920:	101c                	c.addi4spn	a5,sp,32
    6922:	4685                	c.li	a3,1
    6924:	0878                	c.addi4spn	a4,sp,28
    6926:	4601                	c.li	a2,0
    6928:	85a2                	c.mv	a1,s0
    692a:	855a                	c.mv	a0,s6
    692c:	3da9                	c.jal	6786 <__print_integral_poly>
    692e:	02e00793          	li	a5,46
    6932:	001c3c13          	seqz	s8,s8
    6936:	00f50023          	sb	a5,0(a0)
    693a:	0dbafa93          	andi	s5,s5,219
    693e:	00150693          	addi	a3,a0,1
    6942:	87e2                	c.mv	a5,s8
    6944:	401ad35b          	beqc	s5,65,694a <__printf_float_impl+0x140>
    6948:	4781                	c.li	a5,0
    694a:	00148713          	addi	a4,s1,1
    694e:	973e                	c.add	a4,a5
    6950:	fffd0313          	addi	t1,s10,-1
    6954:	1130                	c.addi4spn	a2,sp,168
    6956:	001d1593          	slli	a1,s10,0x1
    695a:	0ba6065b          	lea.h	a2,a2,s10
    695e:	881a                	c.mv	a6,t1
    6960:	4781                	c.li	a5,0
    6962:	1679                	c.addi	a2,-2
    6964:	16085963          	bgez	a6,6ad6 <__printf_float_impl+0x2cc>
    6968:	01a05863          	blez	s10,6978 <__printf_float_impl+0x16e>
    696c:	1a10                	c.addi4spn	a2,sp,304
    696e:	962e                	c.add	a2,a1
    6970:	f7665603          	lhu	a2,-138(a2)
    6974:	e211                	c.bnez	a2,6978 <__printf_float_impl+0x16e>
    6976:	8d1a                	c.mv	s10,t1
    6978:	160c0963          	beqz	s8,6aea <__printf_float_impl+0x2e0>
    697c:	4672                	c.lwsp	a2,28(sp)
    697e:	0017bc13          	seqz	s8,a5
    6982:	167d                	c.addi	a2,-1
    6984:	ce32                	c.swsp	a2,28(sp)
    6986:	00f68023          	sb	a5,0(a3)
    698a:	0685                	c.addi	a3,1
    698c:	f371                	c.bnez	a4,6950 <__printf_float_impl+0x146>
    698e:	567ce15b          	bnec	s9,71,6af0 <__printf_float_impl+0x2e6>
    6992:	47f2                	c.lwsp	a5,28(sp)
    6994:	fff48713          	addi	a4,s1,-1
    6998:	1697de63          	bge	a5,s1,6b14 <__printf_float_impl+0x30a>
    699c:	5671                	c.li	a2,-4
    699e:	16c7cb63          	blt	a5,a2,6b14 <__printf_float_impl+0x30a>
    69a2:	40f704b3          	sub	s1,a4,a5
    69a6:	04600c93          	li	s9,70
    69aa:	0489                	c.addi	s1,2
    69ac:	9526                	c.add	a0,s1
    69ae:	fff54683          	lbu	a3,-1(a0)
    69b2:	40145713          	srai	a4,s0,0x1
    69b6:	fff50793          	addi	a5,a0,-1
    69ba:	00e6a6b3          	slt	a3,a3,a4
    69be:	0016c693          	xori	a3,a3,1
    69c2:	863e                	c.mv	a2,a5
    69c4:	1acb7263          	bgeu	s6,a2,6b68 <__printf_float_impl+0x35e>
    69c8:	16069e63          	bnez	a3,6b44 <__printf_float_impl+0x33a>
    69cc:	17fd                	c.addi	a5,-1
    69ce:	00178513          	addi	a0,a5,1 # 100001 <SRAM2_SIZE+0xe58d9>
    69d2:	00fb7663          	bgeu	s6,a5,69de <__printf_float_impl+0x1d4>
    69d6:	000a0463          	beqz	s4,69de <__printf_float_impl+0x1d4>
    69da:	8854                	exec.it	#45     !lbu	a4,0(a5)
    69dc:	db65                	c.beqz	a4,69cc <__printf_float_impl+0x1c2>
    69de:	fff54783          	lbu	a5,-1(a0)
    69e2:	00e7e6db          	bnec	a5,46,69ee <__printf_float_impl+0x1e4>
    69e6:	8450                	exec.it	#35     !lw	a5,0(s2)
    69e8:	4047f35b          	bbs	a5,4,69ee <__printf_float_impl+0x1e4>
    69ec:	157d                	c.addi	a0,-1
    69ee:	00008737          	lui	a4,0x8
    69f2:	46bd                	c.li	a3,15
    69f4:	61070713          	addi	a4,a4,1552 # 8610 <__digits>
    69f8:	1aab6663          	bltu	s6,a0,6ba4 <__printf_float_impl+0x39a>
    69fc:	5a5cef5b          	bnec	s9,69,6bba <__printf_float_impl+0x3b0>
    6a00:	01950023          	sb	s9,0(a0)
    6a04:	4772                	c.lwsp	a4,28(sp)
    6a06:	02b00793          	li	a5,43
    6a0a:	00075463          	bgez	a4,6a12 <__printf_float_impl+0x208>
    6a0e:	02d00793          	li	a5,45
    6a12:	8200                	exec.it	#64     !sb	a5,1(a0)
    6a14:	47f2                	c.lwsp	a5,28(sp)
    6a16:	4689                	c.li	a3,2
    6a18:	41f7d713          	srai	a4,a5,0x1f
    6a1c:	8fb9                	c.xor	a5,a4
    6a1e:	40e78733          	sub	a4,a5,a4
    6a22:	41f75793          	srai	a5,a4,0x1f
    6a26:	4601                	c.li	a2,0
    6a28:	45a9                	c.li	a1,10
    6a2a:	0509                	c.addi	a0,2
    6a2c:	8b5ff0ef          	jal	ra,62e0 <__print_integral>
    6a30:	bd91                	c.j	6884 <__printf_float_impl+0x7a>
    6a32:	04804f63          	bgtz	s0,6a90 <__printf_float_impl+0x286>
    6a36:	40800433          	neg	s0,s0
    6a3a:	00445c13          	srli	s8,s0,0x4
    6a3e:	001c1b93          	slli	s7,s8,0x1
    6a42:	865e                	c.mv	a2,s7
    6a44:	4581                	c.li	a1,0
    6a46:	1128                	c.addi4spn	a0,sp,168
    6a48:	8010                	exec.it	#1     !jal	ra,5f44 <memset>
    6a4a:	1134                	c.addi4spn	a3,sp,168
    6a4c:	883d                	c.andi	s0,15
    6a4e:	9bb6                	c.add	s7,a3
    6a50:	02540413          	addi	s0,s0,37
    6a54:	86de                	c.mv	a3,s7
    6a56:	4781                	c.li	a5,0
    6a58:	4512                	c.lwsp	a0,4(sp)
    6a5a:	8622                	c.mv	a2,s0
    6a5c:	8d3e                	c.mv	s10,a5
    6a5e:	85ee                	c.mv	a1,s11
    6a60:	0785                	c.addi	a5,1
    6a62:	c636                	c.swsp	a3,12(sp)
    6a64:	c43e                	c.swsp	a5,8(sp)
    6a66:	242d                	c.jal	6c90 <__lshrdi3>
    6a68:	46b2                	c.lwsp	a3,12(sp)
    6a6a:	1441                	c.addi	s0,-16
    6a6c:	00a69023          	sh	a0,0(a3)
    6a70:	47a2                	c.lwsp	a5,8(sp)
    6a72:	0689                	c.addi	a3,2
    6a74:	fe0452e3          	bgez	s0,6a58 <__printf_float_impl+0x24e>
    6a78:	0afb8bdb          	lea.h	s7,s7,a5
    6a7c:	40800633          	neg	a2,s0
    6a80:	854e                	c.mv	a0,s3
    6a82:	85ee                	c.mv	a1,s11
    6a84:	0d09                	c.addi	s10,2
    6a86:	9d62                	c.add	s10,s8
    6a88:	8844                	exec.it	#44     !jal	ra,5cf4 <__ashldi3>
    6a8a:	4c01                	c.li	s8,0
    6a8c:	8660                	exec.it	#114     !sh	a0,0(s7)
    6a8e:	b551                	c.j	6912 <__printf_float_impl+0x108>
    6a90:	03500613          	li	a2,53
    6a94:	8e01                	c.sub	a2,s0
    6a96:	854e                	c.mv	a0,s3
    6a98:	85ee                	c.mv	a1,s11
    6a9a:	2add                	c.jal	6c90 <__lshrdi3>
    6a9c:	1010                	c.addi4spn	a2,sp,32
    6a9e:	cc7ff0ef          	jal	ra,6764 <__int_to_poly>
    6aa2:	02500613          	li	a2,37
    6aa6:	8c2a                	c.mv	s8,a0
    6aa8:	40860433          	sub	s0,a2,s0
    6aac:	0a810b93          	addi	s7,sp,168
    6ab0:	4d01                	c.li	s10,0
    6ab2:	0d05                	c.addi	s10,1
    6ab4:	00045963          	bgez	s0,6ac6 <__printf_float_impl+0x2bc>
    6ab8:	40800633          	neg	a2,s0
    6abc:	854e                	c.mv	a0,s3
    6abe:	85ee                	c.mv	a1,s11
    6ac0:	8844                	exec.it	#44     !jal	ra,5cf4 <__ashldi3>
    6ac2:	8660                	exec.it	#114     !sh	a0,0(s7)
    6ac4:	b5b9                	c.j	6912 <__printf_float_impl+0x108>
    6ac6:	4512                	c.lwsp	a0,4(sp)
    6ac8:	8622                	c.mv	a2,s0
    6aca:	85ee                	c.mv	a1,s11
    6acc:	22d1                	c.jal	6c90 <__lshrdi3>
    6ace:	1441                	c.addi	s0,-16
    6ad0:	8660                	exec.it	#114     !sh	a0,0(s7)
    6ad2:	0b89                	c.addi	s7,2
    6ad4:	bff9                	c.j	6ab2 <__printf_float_impl+0x2a8>
    6ad6:	00065883          	lhu	a7,0(a2)
    6ada:	187d                	c.addi	a6,-1
    6adc:	028888b3          	mul	a7,a7,s0
    6ae0:	97c6                	c.add	a5,a7
    6ae2:	00f61023          	sh	a5,0(a2)
    6ae6:	83c1                	c.srli	a5,0x10
    6ae8:	bdad                	c.j	6962 <__printf_float_impl+0x158>
    6aea:	177d                	c.addi	a4,-1
    6aec:	4c01                	c.li	s8,0
    6aee:	bd61                	c.j	6986 <__printf_float_impl+0x17c>
    6af0:	8726                	c.mv	a4,s1
    6af2:	ea6cdc5b          	beqc	s9,70,69aa <__printf_float_impl+0x1a0>
    6af6:	47f2                	c.lwsp	a5,28(sp)
    6af8:	001b0613          	addi	a2,s6,1
    6afc:	02f04363          	bgtz	a5,6b22 <__printf_float_impl+0x318>
    6b00:	c39d                	c.beqz	a5,6b26 <__printf_float_impl+0x31c>
    6b02:	40f507b3          	sub	a5,a0,a5
    6b06:	02d7e863          	bltu	a5,a3,6b36 <__printf_float_impl+0x32c>
    6b0a:	001b0783          	lb	a5,1(s6)
    6b0e:	00fb0023          	sb	a5,0(s6)
    6b12:	a811                	c.j	6b26 <__printf_float_impl+0x31c>
    6b14:	04500c93          	li	s9,69
    6b18:	bff9                	c.j	6af6 <__printf_float_impl+0x2ec>
    6b1a:	fff50783          	lb	a5,-1(a0)
    6b1e:	157d                	c.addi	a0,-1
    6b20:	8200                	exec.it	#64     !sb	a5,1(a0)
    6b22:	fec51ce3          	bne	a0,a2,6b1a <__printf_float_impl+0x310>
    6b26:	02e00793          	li	a5,46
    6b2a:	001b0513          	addi	a0,s6,1
    6b2e:	84ba                	c.mv	s1,a4
    6b30:	00fb00a3          	sb	a5,1(s6)
    6b34:	bd9d                	c.j	69aa <__printf_float_impl+0x1a0>
    6b36:	0007c603          	lbu	a2,0(a5)
    6b3a:	0785                	c.addi	a5,1
    6b3c:	0505                	c.addi	a0,1
    6b3e:	fec50fa3          	sb	a2,-1(a0)
    6b42:	b7d1                	c.j	6b06 <__printf_float_impl+0x2fc>
    6b44:	167d                	c.addi	a2,-1
    6b46:	00064703          	lbu	a4,0(a2)
    6b4a:	a6e75ddb          	beqc	a4,46,69c4 <__printf_float_impl+0x1ba>
    6b4e:	0705                	c.addi	a4,1
    6b50:	0ff77713          	andi	a4,a4,255
    6b54:	00875663          	bge	a4,s0,6b60 <__printf_float_impl+0x356>
    6b58:	4681                	c.li	a3,0
    6b5a:	00e60023          	sb	a4,0(a2)
    6b5e:	b59d                	c.j	69c4 <__printf_float_impl+0x1ba>
    6b60:	8f01                	c.sub	a4,s0
    6b62:	00e60023          	sb	a4,0(a2)
    6b66:	bdb9                	c.j	69c4 <__printf_float_impl+0x1ba>
    6b68:	873e                	c.mv	a4,a5
    6b6a:	e60681e3          	beqz	a3,69cc <__printf_float_impl+0x1c2>
    6b6e:	02eb6363          	bltu	s6,a4,6b94 <__printf_float_impl+0x38a>
    6b72:	4705                	c.li	a4,1
    6b74:	00eb0023          	sb	a4,0(s6)
    6b78:	4772                	c.lwsp	a4,28(sp)
    6b7a:	0705                	c.addi	a4,1
    6b7c:	ce3a                	c.swsp	a4,28(sp)
    6b7e:	426cd15b          	beqc	s9,70,6ba0 <__printf_float_impl+0x396>
    6b82:	001b0703          	lb	a4,1(s6)
    6b86:	00eb0123          	sb	a4,2(s6)
    6b8a:	02e00713          	li	a4,46
    6b8e:	00eb00a3          	sb	a4,1(s6)
    6b92:	bd2d                	c.j	69cc <__printf_float_impl+0x1c2>
    6b94:	fff70683          	lb	a3,-1(a4)
    6b98:	177d                	c.addi	a4,-1
    6b9a:	00d700a3          	sb	a3,1(a4)
    6b9e:	bfc1                	c.j	6b6e <__printf_float_impl+0x364>
    6ba0:	87aa                	c.mv	a5,a0
    6ba2:	b52d                	c.j	69cc <__printf_float_impl+0x1c2>
    6ba4:	000b4783          	lbu	a5,0(s6)
    6ba8:	00f6e763          	bltu	a3,a5,6bb6 <__printf_float_impl+0x3ac>
    6bac:	97ba                	c.add	a5,a4
    6bae:	00078783          	lb	a5,0(a5)
    6bb2:	00fb0023          	sb	a5,0(s6)
    6bb6:	0b05                	c.addi	s6,1
    6bb8:	b581                	c.j	69f8 <__printf_float_impl+0x1ee>
    6bba:	cc1ce55b          	bnec	s9,65,6884 <__printf_float_impl+0x7a>
    6bbe:	05000793          	li	a5,80
    6bc2:	00f50023          	sb	a5,0(a0)
    6bc6:	4772                	c.lwsp	a4,28(sp)
    6bc8:	02b00793          	li	a5,43
    6bcc:	00075463          	bgez	a4,6bd4 <__printf_float_impl+0x3ca>
    6bd0:	02d00793          	li	a5,45
    6bd4:	8200                	exec.it	#64     !sb	a5,1(a0)
    6bd6:	47f2                	c.lwsp	a5,28(sp)
    6bd8:	4685                	c.li	a3,1
    6bda:	41f7d713          	srai	a4,a5,0x1f
    6bde:	8fb9                	c.xor	a5,a4
    6be0:	8f99                	c.sub	a5,a4
    6be2:	00279713          	slli	a4,a5,0x2
    6be6:	75d7b7db          	bfos	a5,a5,29,29
    6bea:	bd35                	c.j	6a26 <__printf_float_impl+0x21c>

00006bec <strcpy>:
    6bec:	832a                	c.mv	t1,a0
    6bee:	00b566b3          	or	a3,a0,a1
    6bf2:	8a8d                	c.andi	a3,3
    6bf4:	ee91                	c.bnez	a3,6c10 <strcpy+0x24>
    6bf6:	419c                	c.lw	a5,0(a1)
    6bf8:	2007875b          	ffb	a4,a5,zero
    6bfc:	00071a63          	bnez	a4,6c10 <strcpy+0x24>
    6c00:	0591                	c.addi	a1,4
    6c02:	c11c                	c.sw	a5,0(a0)
    6c04:	0511                	c.addi	a0,4
    6c06:	419c                	c.lw	a5,0(a1)
    6c08:	2007875b          	ffb	a4,a5,zero
    6c0c:	fe070ae3          	beqz	a4,6c00 <strcpy+0x14>
    6c10:	0005c683          	lbu	a3,0(a1)
    6c14:	0585                	c.addi	a1,1
    6c16:	00d50023          	sb	a3,0(a0)
    6c1a:	0505                	c.addi	a0,1
    6c1c:	faf5                	c.bnez	a3,6c10 <strcpy+0x24>
    6c1e:	851a                	c.mv	a0,t1
    6c20:	8082                	c.jr	ra

00006c22 <strnlen>:
    6c22:	95aa                	c.add	a1,a0
    6c24:	87aa                	c.mv	a5,a0
    6c26:	00b78463          	beq	a5,a1,6c2e <strnlen+0xc>
    6c2a:	8854                	exec.it	#45     !lbu	a4,0(a5)
    6c2c:	e701                	c.bnez	a4,6c34 <strnlen+0x12>
    6c2e:	40a78533          	sub	a0,a5,a0
    6c32:	8082                	c.jr	ra
    6c34:	0785                	c.addi	a5,1
    6c36:	bfc5                	c.j	6c26 <strnlen+0x4>

00006c38 <_malloc_usable_size_r>:
    6c38:	ffc5a783          	lw	a5,-4(a1)
    6c3c:	ffc78513          	addi	a0,a5,-4
    6c40:	0007d563          	bgez	a5,6c4a <_malloc_usable_size_r+0x12>
    6c44:	95aa                	c.add	a1,a0
    6c46:	419c                	c.lw	a5,0(a1)
    6c48:	953e                	c.add	a0,a5
    6c4a:	8082                	c.jr	ra

00006c4c <_sbrk>:
    6c4c:	8affa72b          	lwgp	a4,-1876 # 300001a4 <heap_end.0>
    6c50:	e709                	c.bnez	a4,6c5a <_sbrk+0xe>
    6c52:	6313970b          	addigp	a4,106032 # 3001a728 <_end>
    6c56:	8aefc7ab          	swgp	a4,-1876 # 300001a4 <heap_end.0>
    6c5a:	8affa6ab          	lwgp	a3,-1876 # 300001a4 <heap_end.0>
    6c5e:	00d50733          	add	a4,a0,a3
    6c62:	00e17663          	bgeu	sp,a4,6c6e <_sbrk+0x22>
    6c66:	4731                	c.li	a4,12
    6c68:	557d                	c.li	a0,-1
    6c6a:	8a04                	exec.it	#76     !swgp	a4,-1884 # 3000019c <__mculib_REENT_errno>
    6c6c:	8082                	c.jr	ra
    6c6e:	8536                	c.mv	a0,a3
    6c70:	8aefc7ab          	swgp	a4,-1876 # 300001a4 <heap_end.0>
    6c74:	8082                	c.jr	ra

00006c76 <_write>:
    6c76:	04000893          	li	a7,64
    6c7a:	00000073          	ecall
    6c7e:	00055763          	bgez	a0,6c8c <_write+0x16>
    6c82:	40a00533          	neg	a0,a0
    6c86:	8aafc3ab          	swgp	a0,-1884 # 3000019c <__mculib_REENT_errno>
    6c8a:	557d                	c.li	a0,-1
    6c8c:	8082                	c.jr	ra
    6c8e:	0001                	c.nop

00006c90 <__lshrdi3>:
    6c90:	87ae                	c.mv	a5,a1
    6c92:	c60d                	c.beqz	a2,6cbc <__lshrdi3+0x2c>
    6c94:	02000593          	li	a1,32
    6c98:	8d91                	c.sub	a1,a2
    6c9a:	00b05b63          	blez	a1,6cb0 <__lshrdi3+0x20>
    6c9e:	00b79733          	sll	a4,a5,a1
    6ca2:	00c55533          	srl	a0,a0,a2
    6ca6:	00c7d5b3          	srl	a1,a5,a2
    6caa:	8d59                	c.or	a0,a4
    6cac:	00008067          	ret
    6cb0:	fe060513          	addi	a0,a2,-32
    6cb4:	4581                	c.li	a1,0
    6cb6:	00a7d533          	srl	a0,a5,a0
    6cba:	8082                	c.jr	ra
    6cbc:	8082                	c.jr	ra
    6cbe:	0001                	c.nop

Disassembly of section .exec.itable:

00006cc0 <_ITB_BASE_>:
    6cc0:	5d5050ef          	jal	ra,ca94 <_data_lmastart+0x314e>
    6cc4:	745050ef          	jal	ra,cc08 <_data_lmastart+0x32c2>
    6cc8:	214040ef          	jal	ra,aedc <_data_lmastart+0x1596>
    6ccc:	04000613          	li	a2,64
    6cd0:	555052ef          	jal	t0,ca24 <_data_lmastart+0x30de>
    6cd4:	00642023          	sw	t1,0(s0)
    6cd8:	0f4060ef          	jal	ra,cdcc <_data_lmastart+0x3486>
    6cdc:	5790506f          	j	ca54 <_data_lmastart+0x310e>
    6ce0:	19812683          	lw	a3,408(sp)
    6ce4:	578000ef          	jal	ra,725c <irq_handler+0x200>
    6ce8:	440307b7          	lui	a5,0x44030
    6cec:	18e040ef          	jal	ra,ae7a <_data_lmastart+0x1534>
    6cf0:	00042223          	sw	zero,4(s0)
    6cf4:	356040ef          	jal	ra,b04a <_data_lmastart+0x1704>
    6cf8:	44030737          	lui	a4,0x44030
    6cfc:	1d4040ef          	jal	ra,aed0 <_data_lmastart+0x158a>
    6d00:	440302b7          	lui	t0,0x44030
    6d04:	00440d93          	addi	s11,s0,4
    6d08:	458407b7          	lui	a5,0x45840
    6d0c:	00032383          	lw	t2,0(t1)
    6d10:	00442e83          	lw	t4,4(s0)
    6d14:	0057a023          	sw	t0,0(a5) # 45840000 <_stack+0x157e0000>
    6d18:	0002a303          	lw	t1,0(t0) # 44030000 <_stack+0x13fd0000>
    6d1c:	1ec040ef          	jal	ra,af08 <_data_lmastart+0x15c2>
    6d20:	525052ef          	jal	t0,ca44 <_data_lmastart+0x30fe>
    6d24:	1bc040ef          	jal	ra,aee0 <_data_lmastart+0x159a>
    6d28:	0008ae03          	lw	t3,0(a7)
    6d2c:	0004c783          	lbu	a5,0(s1)
    6d30:	41440433          	sub	s0,s0,s4
    6d34:	58b050ef          	jal	ra,cabe <_data_lmastart+0x3178>
    6d38:	02000613          	li	a2,32
    6d3c:	5610506f          	j	ca9c <_data_lmastart+0x3156>
    6d40:	0ff00593          	li	a1,255
    6d44:	458307b7          	lui	a5,0x45830
    6d48:	56f0506f          	j	cab6 <_data_lmastart+0x3170>
    6d4c:	00092783          	lw	a5,0(s2)
    6d50:	188040ef          	jal	ra,aed8 <_data_lmastart+0x1592>
    6d54:	00542023          	sw	t0,0(s0)
    6d58:	01e42023          	sw	t5,0(s0)
    6d5c:	00130393          	addi	t2,t1,1
    6d60:	01956533          	or	a0,a0,s9
    6d64:	00092583          	lw	a1,0(s2)
    6d68:	01142223          	sw	a7,4(s0)
    6d6c:	00442f03          	lw	t5,4(s0)
    6d70:	4f5050ef          	jal	ra,ca64 <_data_lmastart+0x311e>
    6d74:	0007c703          	lbu	a4,0(a5) # 45830000 <_stack+0x157d0000>
    6d78:	0004a083          	lw	ra,0(s1)
    6d7c:	585050ef          	jal	ra,cb00 <_data_lmastart+0x31ba>
    6d80:	53b052ef          	jal	t0,caba <_data_lmastart+0x3174>
    6d84:	00442e03          	lw	t3,4(s0)
    6d88:	01de0f33          	add	t5,t3,t4
    6d8c:	0007a023          	sw	zero,0(a5)
    6d90:	00c42303          	lw	t1,12(s0)
    6d94:	000e4e83          	lbu	t4,0(t3)
    6d98:	88ffcdab          	swgp	a5,-1896 # 30000190 <__malloc_free_list>
    6d9c:	00155413          	srli	s0,a0,0x1
    6da0:	fbf68393          	addi	t2,a3,-65
    6da4:	448207b7          	lui	a5,0x44820
    6da8:	0e100413          	li	s0,225
    6dac:	04000513          	li	a0,64
    6db0:	00879713          	slli	a4,a5,0x8
    6db4:	08048493          	addi	s1,s1,128
    6db8:	44010837          	lui	a6,0x44010
    6dbc:	02042023          	sw	zero,32(s0)
    6dc0:	00f500a3          	sb	a5,1(a0)
    6dc4:	216040ef          	jal	ra,afda <_data_lmastart+0x1694>
    6dc8:	06042223          	sw	zero,100(s0)
    6dcc:	03442083          	lw	ra,52(s0)
    6dd0:	00072e03          	lw	t3,0(a4) # 44030000 <_stack+0x13fd0000>
    6dd4:	fbd5f613          	andi	a2,a1,-67
    6dd8:	80c1a303          	lw	t1,-2036(gp) # 30000104 <boot_downloading>
    6ddc:	00042883          	lw	a7,0(s0)
    6de0:	00737533          	and	a0,t1,t2
    6de4:	0114a223          	sw	a7,4(s1)
    6de8:	0004a383          	lw	t2,0(s1)
    6dec:	01844583          	lbu	a1,24(s0)
    6df0:	8aefc3ab          	swgp	a4,-1884 # 3000019c <__mculib_REENT_errno>
    6df4:	8601c783          	lbu	a5,-1952(gp) # 30000158 <init_ok>
    6df8:	04247293          	andi	t0,s0,66
    6dfc:	00c42e83          	lw	t4,12(s0)
    6e00:	a7f08293          	addi	t0,ra,-1409
    6e04:	0004ae83          	lw	t4,0(s1)
    6e08:	440104b7          	lui	s1,0x44010
    6e0c:	0002a703          	lw	a4,0(t0)
    6e10:	00142023          	sw	ra,0(s0)
    6e14:	12c00513          	li	a0,300
    6e18:	fff38413          	addi	s0,t2,-1
    6e1c:	00c42083          	lw	ra,12(s0)
    6e20:	400006b7          	lui	a3,0x40000
    6e24:	44d060ef          	jal	ra,da70 <_data_lmastart+0x412a>
    6e28:	018cc0b7          	lui	ra,0x18cc
    6e2c:	0a042023          	sw	zero,160(s0)
    6e30:	fe477293          	andi	t0,a4,-28
    6e34:	02a353b3          	divu	t2,t1,a0
    6e38:	0ff7f793          	andi	a5,a5,255
    6e3c:	01042023          	sw	a6,0(s0)
    6e40:	591050ef          	jal	ra,cbd0 <_data_lmastart+0x328a>
    6e44:	0047a303          	lw	t1,4(a5) # 44820004 <_stack+0x147c0004>
    6e48:	0008c503          	lbu	a0,0(a7)
    6e4c:	81bfa52b          	lwgp	a0,-2024 # 30000110 <_impure_ptr>
    6e50:	01d42023          	sw	t4,0(s0)
    6e54:	fff50413          	addi	s0,a0,-1
    6e58:	00c42283          	lw	t0,12(s0)
    6e5c:	05e42a23          	sw	t5,84(s0)
    6e60:	00148533          	add	a0,s1,ra
    6e64:	00128313          	addi	t1,t0,1
    6e68:	fd078793          	addi	a5,a5,-48
    6e6c:	0003a683          	lw	a3,0(t2)
    6e70:	00540333          	add	t1,s0,t0
    6e74:	02c42803          	lw	a6,44(s0)
    6e78:	80818493          	addi	s1,gp,-2040 # 30000100 <uart_dl_port>
    6e7c:	00d373b3          	and	t2,t1,a3
    6e80:	00f92023          	sw	a5,0(s2)
    6e84:	08080893          	addi	a7,a6,128 # 44010080 <_stack+0x13fb0080>
    6e88:	00ab9023          	sh	a0,0(s7)
    6e8c:	029dfc33          	remu	s8,s11,s1
    6e90:	0134a023          	sw	s3,0(s1) # 44010000 <_stack+0x13fb0000>

Disassembly of section .text_*driver_flash.o:

10000000 <bk_flash_lock>:
10000000:	8082                	c.jr	ra

10000002 <bk_flash_unlock>:
10000002:	8082                	c.jr	ra

10000004 <flash_get_current_flash_config>:
10000004:	00001717          	auipc	a4,0x1
10000008:	e1870713          	addi	a4,a4,-488 # 10000e1c <flash_config>
1000000c:	8441a603          	lw	a2,-1980(gp) # 3000013c <flash_id>
10000010:	4781                	c.li	a5,0
10000012:	86ba                	c.mv	a3,a4
10000014:	430c                	c.lw	a1,0(a4)
10000016:	00c59a63          	bne	a1,a2,1000002a <flash_get_current_flash_config+0x26>
1000001a:	4331                	c.li	t1,12
1000001c:	026783b3          	mul	t2,a5,t1
10000020:	007682b3          	add	t0,a3,t2
10000024:	8251ae23          	sw	t0,-1988(gp) # 30000134 <flash_current_config>
10000028:	8082                	c.jr	ra
1000002a:	0785                	c.addi	a5,1
1000002c:	0731                	c.addi	a4,12
1000002e:	bf07e35b          	bnec	a5,16,10000014 <flash_get_current_flash_config+0x10>
10000032:	00001297          	auipc	t0,0x1
10000036:	eaa28293          	addi	t0,t0,-342 # 10000edc <flash_config+0xc0>
1000003a:	b7ed                	c.j	10000024 <flash_get_current_flash_config+0x20>

1000003c <set_flash_clk>:
1000003c:	8c04                	exec.it	#14     !lui	a4,0x44030
1000003e:	4f5c                	c.lw	a5,28(a4)
10000040:	893d                	c.andi	a0,15
10000042:	ff07f293          	andi	t0,a5,-16
10000046:	00a2e333          	or	t1,t0,a0
1000004a:	00672e23          	sw	t1,28(a4) # 4403001c <_stack+0x13fd001c>
1000004e:	8082                	c.jr	ra

10000050 <get_flash_ID>:
10000050:	8404                	exec.it	#10     !lui	a5,0x44030
10000052:	4398                	c.lw	a4,0(a5)
10000054:	fe074fe3          	bltz	a4,10000052 <get_flash_ID+0x2>
10000058:	740002b7          	lui	t0,0x74000
1000005c:	44030337          	lui	t1,0x44030
10000060:	8830                	exec.it	#21     !sw	t0,0(a5) # 44030000 <_stack+0x13fd0000>
10000062:	8430                	exec.it	#19     !lw	t2,0(t1) # 44030000 <_stack+0x13fd0000>
10000064:	fe03cfe3          	bltz	t2,10000062 <get_flash_ID+0x12>
10000068:	01032503          	lw	a0,16(t1)
1000006c:	84a1a223          	sw	a0,-1980(gp) # 3000013c <flash_id>
10000070:	0ff57593          	andi	a1,a0,255
10000074:	fff58613          	addi	a2,a1,-1
10000078:	4689                	c.li	a3,2
1000007a:	00c69833          	sll	a6,a3,a2
1000007e:	8501a023          	sw	a6,-1984(gp) # 30000138 <flash_size>
10000082:	8082                	c.jr	ra

10000084 <printf_flash_ID>:
10000084:	f0006317          	auipc	t1,0xf0006
10000088:	cd0302e7          	jalr	t0,-816(t1) # 5d54 <__riscv_save_0>
1000008c:	00001517          	auipc	a0,0x1
10000090:	d8450513          	addi	a0,a0,-636 # 10000e10 <fal_partition_erase_all+0x8>
10000094:	21d5                	c.jal	10000578 <bk_printf>
10000096:	8441a503          	lw	a0,-1980(gp) # 3000013c <flash_id>
1000009a:	2b89                	c.jal	100005ec <bk_print_hex>
1000009c:	f0006317          	auipc	t1,0xf0006
100000a0:	cdc30067          	jr	-804(t1) # 5d78 <__riscv_restore_0>

100000a4 <flash_read_sr>:
100000a4:	86aa                	c.mv	a3,a0
100000a6:	8404                	exec.it	#10     !lui	a5,0x44030
100000a8:	4398                	c.lw	a4,0(a5)
100000aa:	fe074fe3          	bltz	a4,100000a8 <flash_read_sr+0x4>
100000ae:	630002b7          	lui	t0,0x63000
100000b2:	44030337          	lui	t1,0x44030
100000b6:	8830                	exec.it	#21     !sw	t0,0(a5) # 44030000 <_stack+0x13fd0000>
100000b8:	8430                	exec.it	#19     !lw	t2,0(t1) # 44030000 <_stack+0x13fd0000>
100000ba:	fe03cfe3          	bltz	t2,100000b8 <flash_read_sr+0x14>
100000be:	01432503          	lw	a0,20(t1)
100000c2:	0ff57513          	andi	a0,a0,255
100000c6:	0226e25b          	bnec	a3,2,100000ea <flash_read_sr+0x46>
100000ca:	660005b7          	lui	a1,0x66000
100000ce:	44030637          	lui	a2,0x44030
100000d2:	00b32023          	sw	a1,0(t1)
100000d6:	00062803          	lw	a6,0(a2) # 44030000 <_stack+0x13fd0000>
100000da:	fe084ee3          	bltz	a6,100000d6 <flash_read_sr+0x32>
100000de:	01462883          	lw	a7,20(a2)
100000e2:	20f8ae5b          	bfoz	t3,a7,8,15
100000e6:	00ae6533          	or	a0,t3,a0
100000ea:	8082                	c.jr	ra

100000ec <flash_write_enable>:
100000ec:	8404                	exec.it	#10     !lui	a5,0x44030
100000ee:	61000737          	lui	a4,0x61000
100000f2:	8020                	exec.it	#16     !lui	t0,0x44030
100000f4:	c398                	c.sw	a4,0(a5)
100000f6:	8c20                	exec.it	#22     !lw	t1,0(t0) # 44030000 <_stack+0x13fd0000>
100000f8:	fe034fe3          	bltz	t1,100000f6 <flash_write_enable+0xa>
100000fc:	8082                	c.jr	ra

100000fe <flash_write_disable>:
100000fe:	8404                	exec.it	#10     !lui	a5,0x44030
10000100:	62000737          	lui	a4,0x62000
10000104:	8020                	exec.it	#16     !lui	t0,0x44030
10000106:	c398                	c.sw	a4,0(a5)
10000108:	8c20                	exec.it	#22     !lw	t1,0(t0) # 44030000 <_stack+0x13fd0000>
1000010a:	fe034fe3          	bltz	t1,10000108 <flash_write_disable+0xa>
1000010e:	8082                	c.jr	ra

10000110 <flash_write_sr>:
10000110:	8404                	exec.it	#10     !lui	a5,0x44030
10000112:	4398                	c.lw	a4,0(a5)
10000114:	fe074fe3          	bltz	a4,10000112 <flash_write_sr+0x2>
10000118:	01c7a283          	lw	t0,28(a5) # 4403001c <_stack+0x13fd001c>
1000011c:	fc0006b7          	lui	a3,0xfc000
10000120:	3ff68313          	addi	t1,a3,1023 # fc0003ff <_stack+0xcbfa03ff>
10000124:	0062f3b3          	and	t2,t0,t1
10000128:	05aa                	c.slli	a1,0xa
1000012a:	0075e633          	or	a2,a1,t2
1000012e:	44030837          	lui	a6,0x44030
10000132:	cfd0                	c.sw	a2,28(a5)
10000134:	00082883          	lw	a7,0(a6) # 44030000 <_stack+0x13fd0000>
10000138:	fe08cee3          	bltz	a7,10000134 <flash_write_sr+0x24>
1000013c:	64000e37          	lui	t3,0x64000
10000140:	0015565b          	beqc	a0,1,1000014c <flash_write_sr+0x3c>
10000144:	0025665b          	bnec	a0,2,10000150 <flash_write_sr+0x40>
10000148:	67000e37          	lui	t3,0x67000
1000014c:	01c82023          	sw	t3,0(a6)
10000150:	44030537          	lui	a0,0x44030
10000154:	00052e83          	lw	t4,0(a0) # 44030000 <_stack+0x13fd0000>
10000158:	fe0ecee3          	bltz	t4,10000154 <flash_write_sr+0x44>
1000015c:	8082                	c.jr	ra

1000015e <clr_flash_protect>:
1000015e:	f0006317          	auipc	t1,0xf0006
10000162:	bf6302e7          	jalr	t0,-1034(t1) # 5d54 <__riscv_save_0>
10000166:	4509                	c.li	a0,2
10000168:	3f35                	c.jal	100000a4 <flash_read_sr>
1000016a:	c519                	c.beqz	a0,10000178 <clr_flash_protect+0x1a>
1000016c:	83c1a783          	lw	a5,-1988(gp) # 30000134 <flash_current_config>
10000170:	0047c503          	lbu	a0,4(a5)
10000174:	4581                	c.li	a1,0
10000176:	3f69                	c.jal	10000110 <flash_write_sr>
10000178:	f0006317          	auipc	t1,0xf0006
1000017c:	c0030067          	jr	-1024(t1) # 5d78 <__riscv_restore_0>

10000180 <flash_wp_256k>:
10000180:	4115565b          	beqc	a0,81,1000018c <flash_wp_256k+0xc>
10000184:	0a100793          	li	a5,161
10000188:	00f51563          	bne	a0,a5,10000192 <flash_wp_256k+0x12>
1000018c:	45e1                	c.li	a1,24
1000018e:	4505                	c.li	a0,1
10000190:	b741                	c.j	10000110 <flash_write_sr>
10000192:	02c00593          	li	a1,44
10000196:	4509                	c.li	a0,2
10000198:	bfe5                	c.j	10000190 <flash_wp_256k+0x10>

1000019a <set_flash_protect>:
1000019a:	f0006317          	auipc	t1,0xf0006
1000019e:	bba302e7          	jalr	t0,-1094(t1) # 5d54 <__riscv_save_0>
100001a2:	83c18493          	addi	s1,gp,-1988 # 30000134 <flash_current_config>
100001a6:	409c                	c.lw	a5,0(s1)
100001a8:	e11d                	c.bnez	a0,100001ce <set_flash_protect+0x34>
100001aa:	0087d403          	lhu	s0,8(a5)
100001ae:	0047c503          	lbu	a0,4(a5)
100001b2:	040a                	c.slli	s0,0x2
100001b4:	3dc5                	c.jal	100000a4 <flash_read_sr>
100001b6:	00850863          	beq	a0,s0,100001c6 <set_flash_protect+0x2c>
100001ba:	8c44                	exec.it	#46     !lw	ra,0(s1)
100001bc:	3c0425db          	bfoz	a1,s0,15,0
100001c0:	0040c503          	lbu	a0,4(ra) # 18cc004 <SRAM2_SIZE+0x18b18dc>
100001c4:	37b1                	c.jal	10000110 <flash_write_sr>
100001c6:	f0006317          	auipc	t1,0xf0006
100001ca:	bb230067          	jr	-1102(t1) # 5d78 <__riscv_restore_0>
100001ce:	0067d403          	lhu	s0,6(a5)
100001d2:	bff1                	c.j	100001ae <set_flash_protect+0x14>

100001d4 <flash_init>:
100001d4:	8404                	exec.it	#10     !lui	a5,0x44030
100001d6:	4731                	c.li	a4,12
100001d8:	8020                	exec.it	#16     !lui	t0,0x44030
100001da:	cfd8                	c.sw	a4,28(a5)
100001dc:	8c20                	exec.it	#22     !lw	t1,0(t0) # 44030000 <_stack+0x13fd0000>
100001de:	fe034fe3          	bltz	t1,100001dc <flash_init+0x8>
100001e2:	8082                	c.jr	ra

100001e4 <set_flash_qe>:
100001e4:	8404                	exec.it	#10     !lui	a5,0x44030
100001e6:	4398                	c.lw	a4,0(a5)
100001e8:	fe074fe3          	bltz	a4,100001e6 <set_flash_qe+0x2>
100001ec:	01c7a283          	lw	t0,28(a5) # 4403001c <_stack+0x13fd001c>
100001f0:	fc0006b7          	lui	a3,0xfc000
100001f4:	3ff68313          	addi	t1,a3,1023 # fc0003ff <_stack+0xcbfa03ff>
100001f8:	0062f3b3          	and	t2,t0,t1
100001fc:	00080537          	lui	a0,0x80
10000200:	00a3e5b3          	or	a1,t2,a0
10000204:	cfcc                	c.sw	a1,28(a5)
10000206:	4390                	c.lw	a2,0(a5)
10000208:	670008b7          	lui	a7,0x67000
1000020c:	5c06285b          	bfoz	a6,a2,23,0
10000210:	01186e33          	or	t3,a6,a7
10000214:	44030eb7          	lui	t4,0x44030
10000218:	01c7a023          	sw	t3,0(a5)
1000021c:	000eaf03          	lw	t5,0(t4) # 44030000 <_stack+0x13fd0000>
10000220:	fe0f4ee3          	bltz	t5,1000021c <set_flash_qe+0x38>
10000224:	8082                	c.jr	ra

10000226 <clr_flash_qwfr>:
10000226:	8c04                	exec.it	#14     !lui	a4,0x44030
10000228:	4f5c                	c.lw	a5,28(a4)
1000022a:	8224                	exec.it	#88     !lui	a3,0x40000
1000022c:	e0f7f293          	andi	t0,a5,-497
10000230:	00572e23          	sw	t0,28(a4) # 4403001c <_stack+0x13fd001c>
10000234:	00072303          	lw	t1,0(a4)
10000238:	36000537          	lui	a0,0x36000
1000023c:	8e54                	exec.it	#111     !and	t2,t1,a3
1000023e:	00a3e5b3          	or	a1,t2,a0
10000242:	c30c                	c.sw	a1,0(a4)
10000244:	4310                	c.lw	a2,0(a4)
10000246:	fe064fe3          	bltz	a2,10000244 <clr_flash_qwfr+0x1e>
1000024a:	8082                	c.jr	ra

1000024c <flash_set_line_mode>:
1000024c:	0015635b          	bnec	a0,1,10000252 <flash_set_line_mode+0x6>
10000250:	bfd9                	c.j	10000226 <clr_flash_qwfr>
10000252:	0225605b          	bnec	a0,2,10000272 <flash_set_line_mode+0x26>
10000256:	440303b7          	lui	t2,0x44030
1000025a:	01c3a503          	lw	a0,28(t2) # 4403001c <_stack+0x13fd001c>
1000025e:	e0f57593          	andi	a1,a0,-497
10000262:	0105e613          	ori	a2,a1,16
10000266:	00c3ae23          	sw	a2,28(t2)
1000026a:	8654                	exec.it	#107     !lw	a3,0(t2)
1000026c:	fe06cfe3          	bltz	a3,1000026a <flash_set_line_mode+0x1e>
10000270:	8082                	c.jr	ra
10000272:	00456b5b          	bnec	a0,4,10000288 <flash_set_line_mode+0x3c>
10000276:	8c04                	exec.it	#14     !lui	a4,0x44030
10000278:	4f5c                	c.lw	a5,28(a4)
1000027a:	e0f7f293          	andi	t0,a5,-497
1000027e:	0a02e313          	ori	t1,t0,160
10000282:	00672e23          	sw	t1,28(a4) # 4403001c <_stack+0x13fd001c>
10000286:	8082                	c.jr	ra
10000288:	8082                	c.jr	ra

1000028a <flash_set_clk>:
1000028a:	8c04                	exec.it	#14     !lui	a4,0x44030
1000028c:	431c                	c.lw	a5,0(a4)
1000028e:	fe07cfe3          	bltz	a5,1000028c <flash_set_clk+0x2>
10000292:	01c72283          	lw	t0,28(a4) # 4403001c <_stack+0x13fd001c>
10000296:	893d                	c.andi	a0,15
10000298:	ff02f313          	andi	t1,t0,-16
1000029c:	00a363b3          	or	t2,t1,a0
100002a0:	440305b7          	lui	a1,0x44030
100002a4:	00772e23          	sw	t2,28(a4)
100002a8:	4190                	c.lw	a2,0(a1)
100002aa:	fe064fe3          	bltz	a2,100002a8 <flash_set_clk+0x1e>
100002ae:	8082                	c.jr	ra

100002b0 <flash_erase_sector>:
100002b0:	8401a783          	lw	a5,-1984(gp) # 30000138 <flash_size>
100002b4:	02f57863          	bgeu	a0,a5,100002e4 <flash_erase_sector+0x34>
100002b8:	8020                	exec.it	#16     !lui	t0,0x44030
100002ba:	8630                	exec.it	#83     !lw	a4,0(t0) # 44030000 <_stack+0x13fd0000>
100002bc:	fe074fe3          	bltz	a4,100002ba <flash_erase_sector+0xa>
100002c0:	8c20                	exec.it	#22     !lw	t1,0(t0)
100002c2:	8224                	exec.it	#88     !lui	a3,0x40000
100002c4:	5c05255b          	bfoz	a0,a0,23,0
100002c8:	8e54                	exec.it	#111     !and	t2,t1,a3
100002ca:	007565b3          	or	a1,a0,t2
100002ce:	2d000637          	lui	a2,0x2d000
100002d2:	00c5e833          	or	a6,a1,a2
100002d6:	440308b7          	lui	a7,0x44030
100002da:	0102a023          	sw	a6,0(t0)
100002de:	8424                	exec.it	#26     !lw	t3,0(a7) # 44030000 <_stack+0x13fd0000>
100002e0:	fe0e4fe3          	bltz	t3,100002de <flash_erase_sector+0x2e>
100002e4:	8082                	c.jr	ra

100002e6 <flash_erase_block>:
100002e6:	8401a783          	lw	a5,-1984(gp) # 30000138 <flash_size>
100002ea:	02f57863          	bgeu	a0,a5,1000031a <flash_erase_block+0x34>
100002ee:	8020                	exec.it	#16     !lui	t0,0x44030
100002f0:	8630                	exec.it	#83     !lw	a4,0(t0) # 44030000 <_stack+0x13fd0000>
100002f2:	fe074fe3          	bltz	a4,100002f0 <flash_erase_block+0xa>
100002f6:	8c20                	exec.it	#22     !lw	t1,0(t0)
100002f8:	8224                	exec.it	#88     !lui	a3,0x40000
100002fa:	5c05255b          	bfoz	a0,a0,23,0
100002fe:	8e54                	exec.it	#111     !and	t2,t1,a3
10000300:	007565b3          	or	a1,a0,t2
10000304:	2f000637          	lui	a2,0x2f000
10000308:	00c5e833          	or	a6,a1,a2
1000030c:	440308b7          	lui	a7,0x44030
10000310:	0102a023          	sw	a6,0(t0)
10000314:	8424                	exec.it	#26     !lw	t3,0(a7) # 44030000 <_stack+0x13fd0000>
10000316:	fe0e4fe3          	bltz	t3,10000314 <flash_erase_block+0x2e>
1000031a:	8082                	c.jr	ra

1000031c <flash_erase>:
1000031c:	f0006317          	auipc	t1,0xf0006
10000320:	a1e302e7          	jalr	t0,-1506(t1) # 5d3a <__riscv_save_4>
10000324:	00fff4b7          	lui	s1,0xfff
10000328:	67bd                	c.lui	a5,0xf
1000032a:	8ce9                	c.and	s1,a0
1000032c:	8d7d                	c.and	a0,a5
1000032e:	892e                	c.mv	s2,a1
10000330:	ed09                	c.bnez	a0,1000034a <flash_erase+0x2e>
10000332:	842e                	c.mv	s0,a1
10000334:	69c1                	c.lui	s3,0x10
10000336:	0489e563          	bltu	s3,s0,10000380 <flash_erase+0x64>
1000033a:	94a2                	c.add	s1,s0
1000033c:	6985                	c.lui	s3,0x1
1000033e:	e439                	c.bnez	s0,1000038c <flash_erase+0x70>
10000340:	854a                	c.mv	a0,s2
10000342:	f0006317          	auipc	t1,0xf0006
10000346:	a2c30067          	jr	-1492(t1) # 5d6e <__riscv_restore_4>
1000034a:	62c1                	c.lui	t0,0x10
1000034c:	fff28313          	addi	t1,t0,-1 # ffff <_data_lmastart+0x66b9>
10000350:	006483b3          	add	t2,s1,t1
10000354:	00ff0737          	lui	a4,0xff0
10000358:	00e3f433          	and	s0,t2,a4
1000035c:	00b489b3          	add	s3,s1,a1
10000360:	01347363          	bgeu	s0,s3,10000366 <flash_erase+0x4a>
10000364:	89a2                	c.mv	s3,s0
10000366:	844a                	c.mv	s0,s2
10000368:	6a05                	c.lui	s4,0x1
1000036a:	fd34f5e3          	bgeu	s1,s3,10000334 <flash_erase+0x18>
1000036e:	8526                	c.mv	a0,s1
10000370:	3781                	c.jal	100002b0 <flash_erase_sector>
10000372:	94d2                	c.add	s1,s4
10000374:	01446463          	bltu	s0,s4,1000037c <flash_erase+0x60>
10000378:	8824                	exec.it	#28     !sub	s0,s0,s4
1000037a:	bfc5                	c.j	1000036a <flash_erase+0x4e>
1000037c:	4401                	c.li	s0,0
1000037e:	b7f5                	c.j	1000036a <flash_erase+0x4e>
10000380:	8526                	c.mv	a0,s1
10000382:	41340433          	sub	s0,s0,s3
10000386:	94ce                	c.add	s1,s3
10000388:	3fb9                	c.jal	100002e6 <flash_erase_block>
1000038a:	b775                	c.j	10000336 <flash_erase+0x1a>
1000038c:	40848533          	sub	a0,s1,s0
10000390:	3705                	c.jal	100002b0 <flash_erase_sector>
10000392:	fb3467e3          	bltu	s0,s3,10000340 <flash_erase+0x24>
10000396:	41340433          	sub	s0,s0,s3
1000039a:	b755                	c.j	1000033e <flash_erase+0x22>

1000039c <flash_read_data>:
1000039c:	c651                	c.beqz	a2,10000428 <flash_read_data+0x8c>
1000039e:	1101                	c.addi	sp,-32
100003a0:	8c04                	exec.it	#14     !lui	a4,0x44030
100003a2:	431c                	c.lw	a5,0(a4)
100003a4:	fe07cfe3          	bltz	a5,100003a2 <flash_read_data+0x6>
100003a8:	01000337          	lui	t1,0x1000
100003ac:	fe05f893          	andi	a7,a1,-32
100003b0:	8020                	exec.it	#16     !lui	t0,0x44030
100003b2:	fff30393          	addi	t2,t1,-1 # ffffff <SRAM2_SIZE+0xfe58d7>
100003b6:	40000e37          	lui	t3,0x40000
100003ba:	25000eb7          	lui	t4,0x25000
100003be:	0002a683          	lw	a3,0(t0) # 44030000 <_stack+0x13fd0000>
100003c2:	0078f833          	and	a6,a7,t2
100003c6:	01c6ff33          	and	t5,a3,t3
100003ca:	01e86fb3          	or	t6,a6,t5
100003ce:	01dfe733          	or	a4,t6,t4
100003d2:	00e2a023          	sw	a4,0(t0)
100003d6:	0002a783          	lw	a5,0(t0)
100003da:	fe07cee3          	bltz	a5,100003d6 <flash_read_data+0x3a>
100003de:	880a                	c.mv	a6,sp
100003e0:	02088893          	addi	a7,a7,32
100003e4:	8342                	c.mv	t1,a6
100003e6:	0082a683          	lw	a3,8(t0)
100003ea:	0811                	c.addi	a6,4
100003ec:	fed82e23          	sw	a3,-4(a6)
100003f0:	02010f13          	addi	t5,sp,32
100003f4:	ff0f19e3          	bne	t5,a6,100003e6 <flash_read_data+0x4a>
100003f8:	01f5f693          	andi	a3,a1,31
100003fc:	8faa                	c.mv	t6,a0
100003fe:	00d30733          	add	a4,t1,a3
10000402:	00070783          	lb	a5,0(a4) # 44030000 <_stack+0x13fd0000>
10000406:	0f85                	c.addi	t6,1
10000408:	00bf8833          	add	a6,t6,a1
1000040c:	167d                	c.addi	a2,-1
1000040e:	feff8fa3          	sb	a5,-1(t6)
10000412:	40a80f33          	sub	t5,a6,a0
10000416:	c619                	c.beqz	a2,10000424 <flash_read_data+0x88>
10000418:	0685                	c.addi	a3,1
1000041a:	be06e2db          	bnec	a3,32,100003fe <flash_read_data+0x62>
1000041e:	85fa                	c.mv	a1,t5
10000420:	857e                	c.mv	a0,t6
10000422:	bf71                	c.j	100003be <flash_read_data+0x22>
10000424:	6105                	c.addi16sp	sp,32
10000426:	8082                	c.jr	ra
10000428:	8082                	c.jr	ra

1000042a <flash_write_data>:
1000042a:	f0006317          	auipc	t1,0xf0006
1000042e:	902302e7          	jalr	t0,-1790(t1) # 5d2c <__riscv_save_10>
10000432:	01f5f793          	andi	a5,a1,31
10000436:	1101                	c.addi	sp,-32
10000438:	892a                	c.mv	s2,a0
1000043a:	842e                	c.mv	s0,a1
1000043c:	84b2                	c.mv	s1,a2
1000043e:	fe05fa13          	andi	s4,a1,-32
10000442:	c789                	c.beqz	a5,1000044c <flash_write_data+0x22>
10000444:	8c24                	exec.it	#30     !li	a2,32
10000446:	85d2                	c.mv	a1,s4
10000448:	850a                	c.mv	a0,sp
1000044a:	3f89                	c.jal	1000039c <flash_read_data>
1000044c:	8c04                	exec.it	#14     !lui	a4,0x44030
1000044e:	00072283          	lw	t0,0(a4) # 44030000 <_stack+0x13fd0000>
10000452:	fe02cee3          	bltz	t0,1000044e <flash_write_data+0x24>
10000456:	44030ab7          	lui	s5,0x44030
1000045a:	40000bb7          	lui	s7,0x40000
1000045e:	2c000c37          	lui	s8,0x2c000
10000462:	e491                	c.bnez	s1,1000046e <flash_write_data+0x44>
10000464:	6105                	c.addi16sp	sp,32
10000466:	f0006317          	auipc	t1,0xf0006
1000046a:	8fe30067          	jr	-1794(t1) # 5d64 <__riscv_restore_10>
1000046e:	01f47313          	andi	t1,s0,31
10000472:	89ca                	c.mv	s3,s2
10000474:	00098683          	lb	a3,0(s3) # 1000 <__rtos_signature_freertos_v10_3+0x1000>
10000478:	0985                	c.addi	s3,1
1000047a:	006100b3          	add	ra,sp,t1
1000047e:	00898b33          	add	s6,s3,s0
10000482:	14fd                	c.addi	s1,-1
10000484:	00d08023          	sb	a3,0(ra)
10000488:	412b0b33          	sub	s6,s6,s2
1000048c:	c481                	c.beqz	s1,10000494 <flash_write_data+0x6a>
1000048e:	0305                	c.addi	t1,1
10000490:	be0362db          	bnec	t1,32,10000474 <flash_write_data+0x4a>
10000494:	840a                	c.mv	s0,sp
10000496:	00042383          	lw	t2,0(s0)
1000049a:	1008                	c.addi4spn	a0,sp,32
1000049c:	0411                	c.addi	s0,4
1000049e:	007aa223          	sw	t2,4(s5) # 44030004 <_stack+0x13fd0004>
100004a2:	fea41ae3          	bne	s0,a0,10000496 <flash_write_data+0x6c>
100004a6:	000aa583          	lw	a1,0(s5)
100004aa:	0175f633          	and	a2,a1,s7
100004ae:	01466833          	or	a6,a2,s4
100004b2:	018868b3          	or	a7,a6,s8
100004b6:	011aa023          	sw	a7,0(s5)
100004ba:	000aa903          	lw	s2,0(s5)
100004be:	fe094ee3          	bltz	s2,100004ba <flash_write_data+0x90>
100004c2:	8c24                	exec.it	#30     !li	a2,32
100004c4:	8040                	exec.it	#32     !li	a1,255
100004c6:	850a                	c.mv	a0,sp
100004c8:	020a0a13          	addi	s4,s4,32 # 1020 <__rtos_signature_freertos_v10_3+0x1020>
100004cc:	845a                	c.mv	s0,s6
100004ce:	894e                	c.mv	s2,s3
100004d0:	f0006097          	auipc	ra,0xf0006
100004d4:	a74080e7          	jalr	-1420(ra) # 5f44 <memset>
100004d8:	b769                	c.j	10000462 <flash_write_data+0x38>

100004da <uart_init>:
100004da:	f0006317          	auipc	t1,0xf0006
100004de:	87a302e7          	jalr	t0,-1926(t1) # 5d54 <__riscv_save_0>
100004e2:	6471                	c.lui	s0,0x1c
100004e4:	20040513          	addi	a0,s0,512 # 1c200 <SRAM2_SIZE+0x1ad8>
100004e8:	f0002097          	auipc	ra,0xf0002
100004ec:	bd6080e7          	jalr	-1066(ra) # 20be <uart0_init>
100004f0:	20040513          	addi	a0,s0,512
100004f4:	f0002097          	auipc	ra,0xf0002
100004f8:	d0e080e7          	jalr	-754(ra) # 2202 <uart1_init>
100004fc:	4501                	c.li	a0,0
100004fe:	f0006317          	auipc	t1,0xf0006
10000502:	87a30067          	jr	-1926(t1) # 5d78 <__riscv_restore_0>

10000506 <uart_deinit>:
10000506:	f0006317          	auipc	t1,0xf0006
1000050a:	84e302e7          	jalr	t0,-1970(t1) # 5d54 <__riscv_save_0>
1000050e:	f0002097          	auipc	ra,0xf0002
10000512:	c22080e7          	jalr	-990(ra) # 2130 <uart0_disable>
10000516:	f0002097          	auipc	ra,0xf0002
1000051a:	d60080e7          	jalr	-672(ra) # 2276 <uart1_disable>
1000051e:	f0006317          	auipc	t1,0xf0006
10000522:	85a30067          	jr	-1958(t1) # 5d78 <__riscv_restore_0>

10000526 <uart_putc>:
10000526:	f0006317          	auipc	t1,0xf0006
1000052a:	82e302e7          	jalr	t0,-2002(t1) # 5d54 <__riscv_save_0>
1000052e:	1141                	c.addi	sp,-16
10000530:	00a10623          	sb	a0,12(sp)
10000534:	4585                	c.li	a1,1
10000536:	0068                	c.addi4spn	a0,sp,12
10000538:	f0002097          	auipc	ra,0xf0002
1000053c:	e08080e7          	jalr	-504(ra) # 2340 <uart1_send>
10000540:	4505                	c.li	a0,1
10000542:	0141                	c.addi	sp,16
10000544:	f0006317          	auipc	t1,0xf0006
10000548:	83430067          	jr	-1996(t1) # 5d78 <__riscv_restore_0>

1000054c <uart_getc>:
1000054c:	557d                	c.li	a0,-1
1000054e:	8082                	c.jr	ra

10000550 <uart_console_output>:
10000550:	f0006317          	auipc	t1,0xf0006
10000554:	804302e7          	jalr	t0,-2044(t1) # 5d54 <__riscv_save_0>
10000558:	892a                	c.mv	s2,a0
1000055a:	84ae                	c.mv	s1,a1
1000055c:	4401                	c.li	s0,0
1000055e:	00944663          	blt	s0,s1,1000056a <uart_console_output+0x1a>
10000562:	f0006317          	auipc	t1,0xf0006
10000566:	81630067          	jr	-2026(t1) # 5d78 <__riscv_restore_0>
1000056a:	008907b3          	add	a5,s2,s0
1000056e:	0007c503          	lbu	a0,0(a5) # f000 <_data_lmastart+0x56ba>
10000572:	0405                	c.addi	s0,1
10000574:	3f4d                	c.jal	10000526 <uart_putc>
10000576:	b7e5                	c.j	1000055e <uart_console_output+0xe>

10000578 <bk_printf>:
10000578:	7179                	c.addi16sp	sp,-48
1000057a:	c606                	c.swsp	ra,12(sp)
1000057c:	ca2e                	c.swsp	a1,20(sp)
1000057e:	cc32                	c.swsp	a2,24(sp)
10000580:	ce36                	c.swsp	a3,28(sp)
10000582:	d03a                	c.swsp	a4,32(sp)
10000584:	d23e                	c.swsp	a5,36(sp)
10000586:	d442                	c.swsp	a6,40(sp)
10000588:	d646                	c.swsp	a7,44(sp)
1000058a:	f0002097          	auipc	ra,0xf0002
1000058e:	dce080e7          	jalr	-562(ra) # 2358 <uart1_send_string>
10000592:	40b2                	c.lwsp	ra,12(sp)
10000594:	6145                	c.addi16sp	sp,48
10000596:	8082                	c.jr	ra

10000598 <hex2Str>:
10000598:	f0005317          	auipc	t1,0xf0005
1000059c:	7bc302e7          	jalr	t0,1980(t1) # 5d54 <__riscv_save_0>
100005a0:	1101                	c.addi	sp,-32
100005a2:	842a                	c.mv	s0,a0
100005a4:	4645                	c.li	a2,17
100005a6:	00001597          	auipc	a1,0x1
100005aa:	94258593          	addi	a1,a1,-1726 # 10000ee8 <flash_config+0xcc>
100005ae:	0068                	c.addi4spn	a0,sp,12
100005b0:	f0006097          	auipc	ra,0xf0006
100005b4:	824080e7          	jalr	-2012(ra) # 5dd4 <memcpy>
100005b8:	1014                	c.addi4spn	a3,sp,32
100005ba:	00445793          	srli	a5,s0,0x4
100005be:	00f47393          	andi	t2,s0,15
100005c2:	00768433          	add	s0,a3,t2
100005c6:	00f682b3          	add	t0,a3,a5
100005ca:	fec28303          	lb	t1,-20(t0)
100005ce:	fec40603          	lb	a2,-20(s0)
100005d2:	83818513          	addi	a0,gp,-1992 # 30000130 <str.0>
100005d6:	00650023          	sb	t1,0(a0) # 36000000 <_stack+0x5fa0000>
100005da:	00c500a3          	sb	a2,1(a0)
100005de:	00050123          	sb	zero,2(a0)
100005e2:	6105                	c.addi16sp	sp,32
100005e4:	f0005317          	auipc	t1,0xf0005
100005e8:	79430067          	jr	1940(t1) # 5d78 <__riscv_restore_0>

100005ec <bk_print_hex>:
100005ec:	f0005317          	auipc	t1,0xf0005
100005f0:	768302e7          	jalr	t0,1896(t1) # 5d54 <__riscv_save_0>
100005f4:	84aa                	c.mv	s1,a0
100005f6:	4461                	c.li	s0,24
100005f8:	00001517          	auipc	a0,0x1
100005fc:	90450513          	addi	a0,a0,-1788 # 10000efc <flash_config+0xe0>
10000600:	5961                	c.li	s2,-8
10000602:	f0002097          	auipc	ra,0xf0002
10000606:	d56080e7          	jalr	-682(ra) # 2358 <uart1_send_string>
1000060a:	0084d533          	srl	a0,s1,s0
1000060e:	0ff57513          	andi	a0,a0,255
10000612:	1461                	c.addi	s0,-8
10000614:	3751                	c.jal	10000598 <hex2Str>
10000616:	f0002097          	auipc	ra,0xf0002
1000061a:	d42080e7          	jalr	-702(ra) # 2358 <uart1_send_string>
1000061e:	ff2416e3          	bne	s0,s2,1000060a <bk_print_hex+0x1e>
10000622:	f0005317          	auipc	t1,0xf0005
10000626:	75630067          	jr	1878(t1) # 5d78 <__riscv_restore_0>

1000062a <uart2_init>:
1000062a:	f0005317          	auipc	t1,0xf0005
1000062e:	72a302e7          	jalr	t0,1834(t1) # 5d54 <__riscv_save_0>
10000632:	8420                	exec.it	#18     !lui	a5,0x45840
10000634:	8470                	exec.it	#51     !sw	zero,0(a5) # 45840000 <_stack+0x157e0000>
10000636:	8464                	exec.it	#58     !li	s0,225
10000638:	cd19                	c.beqz	a0,10000656 <uart2_init+0x2c>
1000063a:	8624                	exec.it	#90     !lui	ra,0x18cc
1000063c:	8c70                	exec.it	#55     !srli	s0,a0,0x1
1000063e:	8220                	exec.it	#80     !addi	t0,ra,-1409 # 18cba7f <SRAM2_SIZE+0x18b1357>
10000640:	8a44                	exec.it	#108     !add	t1,s0,t0
10000642:	8a34                	exec.it	#93     !divu	t2,t1,a0
10000644:	4491                	c.li	s1,4
10000646:	8e20                	exec.it	#86     !addi	s0,t2,-1
10000648:	00947363          	bgeu	s0,s1,1000064e <uart2_init+0x24>
1000064c:	4411                	c.li	s0,4
1000064e:	6509                	c.lui	a0,0x2
10000650:	00a46363          	bltu	s0,a0,10000656 <uart2_init+0x2c>
10000654:	8a50                	exec.it	#101     !addi	s0,a0,-1 # 1fff <nmi_handler+0x9d>
10000656:	8620                	exec.it	#82     !lui	s1,0x44010
10000658:	588c                	c.lw	a1,48(s1)
1000065a:	6705                	c.lui	a4,0x1
1000065c:	80070613          	addi	a2,a4,-2048 # 800 <__rtos_signature_freertos_v10_3+0x800>
10000660:	00c5e6b3          	or	a3,a1,a2
10000664:	4509                	c.li	a0,2
10000666:	d894                	c.sw	a3,48(s1)
10000668:	f0002097          	auipc	ra,0xf0002
1000066c:	9c8080e7          	jalr	-1592(ra) # 2030 <GPIO_UART_function_enable>
10000670:	688d                	c.lui	a7,0x3
10000672:	45840837          	lui	a6,0x45840
10000676:	04088e13          	addi	t3,a7,64 # 3040 <bsdiff_flash_init+0xa>
1000067a:	01c82223          	sw	t3,4(a6) # 45840004 <_stack+0x157e0004>
1000067e:	04200e93          	li	t4,66
10000682:	01d82823          	sw	t4,16(a6)
10000686:	00082c23          	sw	zero,24(a6)
1000068a:	00841f13          	slli	t5,s0,0x8
1000068e:	00082e23          	sw	zero,28(a6)
10000692:	01bf6413          	ori	s0,t5,27
10000696:	00882023          	sw	s0,0(a6)
1000069a:	8201aa23          	sw	zero,-1996(gp) # 3000012c <uart2_rx_done>
1000069e:	8201a823          	sw	zero,-2000(gp) # 30000128 <uart2_rx_index>
100006a2:	8874                	exec.it	#61     !addi	s1,s1,128 # 44010080 <_stack+0x13fb0080>
100006a4:	0004af83          	lw	t6,0(s1)
100006a8:	67c1                	c.lui	a5,0x10
100006aa:	00ffe2b3          	or	t0,t6,a5
100006ae:	0054a023          	sw	t0,0(s1)
100006b2:	f0005317          	auipc	t1,0xf0005
100006b6:	6c630067          	jr	1734(t1) # 5d78 <__riscv_restore_0>

100006ba <uart2_disable>:
100006ba:	8420                	exec.it	#18     !lui	a5,0x45840
100006bc:	4398                	c.lw	a4,0(a5)
100006be:	76f5                	c.lui	a3,0xffffd
100006c0:	8a24                	exec.it	#92     !andi	t0,a4,-28
100006c2:	8830                	exec.it	#21     !sw	t0,0(a5) # 45840000 <_stack+0x157e0000>
100006c4:	8250                	exec.it	#97     !lw	t1,4(a5)
100006c6:	8064                	exec.it	#56     !addi	t2,a3,-65 # ffffcfbf <_stack+0xcff9cfbf>
100006c8:	8204                	exec.it	#72     !and	a0,t1,t2
100006ca:	c3c8                	c.sw	a0,4(a5)
100006cc:	4b8c                	c.lw	a1,16(a5)
100006ce:	8c64                	exec.it	#62     !lui	a6,0x44010
100006d0:	8270                	exec.it	#113     !addi	a7,a6,128 # 44010080 <_stack+0x13fb0080>
100006d2:	8a10                	exec.it	#69     !andi	a2,a1,-67
100006d4:	cb90                	c.sw	a2,16(a5)
100006d6:	7ec1                	c.lui	t4,0xffff0
100006d8:	8424                	exec.it	#26     !lw	t3,0(a7)
100006da:	fffe8f13          	addi	t5,t4,-1 # fffeffff <_stack+0xcff8ffff>
100006de:	01ee7fb3          	and	t6,t3,t5
100006e2:	01f8a023          	sw	t6,0(a7)
100006e6:	777d                	c.lui	a4,0xfffff
100006e8:	03082783          	lw	a5,48(a6)
100006ec:	7ff70293          	addi	t0,a4,2047 # fffff7ff <_stack+0xcff9f7ff>
100006f0:	0057f333          	and	t1,a5,t0
100006f4:	02682823          	sw	t1,48(a6)
100006f8:	8082                	c.jr	ra

100006fa <UART2_InterruptHandler>:
100006fa:	f0005317          	auipc	t1,0xf0005
100006fe:	640302e7          	jalr	t0,1600(t1) # 5d3a <__riscv_save_4>
10000702:	8420                	exec.it	#18     !lui	a5,0x45840
10000704:	4bc0                	c.lw	s0,20(a5)
10000706:	8e04                	exec.it	#78     !andi	t0,s0,66
10000708:	02028f63          	beqz	t0,10000746 <UART2_InterruptHandler+0x4c>
1000070c:	8e00                	exec.it	#70     !lw	t1,-2036(gp) # 30000104 <boot_downloading>
1000070e:	06136c5b          	bnec	t1,1,10000786 <UART2_InterruptHandler+0x8c>
10000712:	8e44                	exec.it	#110     !addi	s1,gp,-2040 # 30000100 <uart_dl_port>
10000714:	8604                	exec.it	#74     !lw	t2,0(s1)
10000716:	00038463          	beqz	t2,1000071e <UART2_InterruptHandler+0x24>
1000071a:	0663e65b          	bnec	t2,6,10000786 <UART2_InterruptHandler+0x8c>
1000071e:	45840937          	lui	s2,0x45840
10000722:	4999                	c.li	s3,6
10000724:	a829                	c.j	1000073e <UART2_InterruptHandler+0x44>
10000726:	8c44                	exec.it	#46     !lw	ra,0(s1)
10000728:	00009363          	bnez	ra,1000072e <UART2_InterruptHandler+0x34>
1000072c:	8a60                	exec.it	#116     !sw	s3,0(s1)
1000072e:	00c92283          	lw	t0,12(s2) # 4584000c <_stack+0x157e000c>
10000732:	0ff2f513          	andi	a0,t0,255
10000736:	f0002097          	auipc	ra,0xf0002
1000073a:	dd8080e7          	jalr	-552(ra) # 250e <uart_download_rx>
1000073e:	00892f83          	lw	t6,8(s2)
10000742:	ff5ff25b          	bbs	t6,21,10000726 <UART2_InterruptHandler+0x2c>
10000746:	8420                	exec.it	#18     !lui	a5,0x45840
10000748:	cbc0                	c.sw	s0,20(a5)
1000074a:	f0005317          	auipc	t1,0xf0005
1000074e:	62430067          	jr	1572(t1) # 5d6e <__riscv_restore_4>
10000752:	00c6a883          	lw	a7,12(a3)
10000756:	8a00                	exec.it	#68     !lw	t3,0(a4)
10000758:	001e0813          	addi	a6,t3,1 # 40000001 <_stack+0xffa0001>
1000075c:	01c60eb3          	add	t4,a2,t3
10000760:	01072023          	sw	a6,0(a4)
10000764:	00072f03          	lw	t5,0(a4)
10000768:	011e8023          	sb	a7,0(t4)
1000076c:	00bf1463          	bne	t5,a1,10000774 <UART2_InterruptHandler+0x7a>
10000770:	8201a823          	sw	zero,-2000(gp) # 30000128 <uart2_rx_index>
10000774:	4688                	c.lw	a0,8(a3)
10000776:	fd557e5b          	bbs	a0,21,10000752 <UART2_InterruptHandler+0x58>
1000077a:	bc64765b          	bbc	s0,6,10000746 <UART2_InterruptHandler+0x4c>
1000077e:	4485                	c.li	s1,1
10000780:	8291aa23          	sw	s1,-1996(gp) # 3000012c <uart2_rx_done>
10000784:	b7c9                	c.j	10000746 <UART2_InterruptHandler+0x4c>
10000786:	458406b7          	lui	a3,0x45840
1000078a:	83018713          	addi	a4,gp,-2000 # 30000128 <uart2_rx_index>
1000078e:	9b018613          	addi	a2,gp,-1616 # 300002a8 <uart2_rx_buf>
10000792:	08000593          	li	a1,128
10000796:	bff9                	c.j	10000774 <UART2_InterruptHandler+0x7a>

10000798 <uart2_send>:
10000798:	95aa                	c.add	a1,a0
1000079a:	8420                	exec.it	#18     !lui	a5,0x45840
1000079c:	00b51363          	bne	a0,a1,100007a2 <uart2_send+0xa>
100007a0:	8082                	c.jr	ra
100007a2:	00054703          	lbu	a4,0(a0)
100007a6:	0505                	c.addi	a0,1
100007a8:	4794                	c.lw	a3,8(a5)
100007aa:	bf46ff5b          	bbc	a3,20,100007a8 <uart2_send+0x10>
100007ae:	c7d8                	c.sw	a4,12(a5)
100007b0:	b7f5                	c.j	1000079c <uart2_send+0x4>

100007b2 <uart2_send_string>:
100007b2:	8081a783          	lw	a5,-2040(gp) # 30000100 <uart_dl_port>
100007b6:	0267e25b          	bnec	a5,6,100007da <uart2_send_string+0x28>
100007ba:	80c1a283          	lw	t0,-2036(gp) # 30000104 <boot_downloading>
100007be:	0012ee5b          	bnec	t0,1,100007da <uart2_send_string+0x28>
100007c2:	8082                	c.jr	ra
100007c4:	0505                	c.addi	a0,1
100007c6:	4714                	c.lw	a3,8(a4)
100007c8:	bf46ff5b          	bbc	a3,20,100007c6 <uart2_send_string+0x14>
100007cc:	00672623          	sw	t1,12(a4)
100007d0:	00054303          	lbu	t1,0(a0)
100007d4:	fe0318e3          	bnez	t1,100007c4 <uart2_send_string+0x12>
100007d8:	8082                	c.jr	ra
100007da:	45840737          	lui	a4,0x45840
100007de:	bfcd                	c.j	100007d0 <uart2_send_string+0x1e>

100007e0 <uart2_wait_tx_finish>:
100007e0:	45840737          	lui	a4,0x45840
100007e4:	471c                	c.lw	a5,8(a4)
100007e6:	bf17ff5b          	bbc	a5,17,100007e4 <uart2_wait_tx_finish+0x4>
100007ea:	8082                	c.jr	ra

100007ec <fal_flash_init>:
100007ec:	f0005317          	auipc	t1,0xf0005
100007f0:	568302e7          	jalr	t0,1384(t1) # 5d54 <__riscv_save_0>
100007f4:	85918413          	addi	s0,gp,-1959 # 30000151 <init_ok>
100007f8:	00044783          	lbu	a5,0(s0)
100007fc:	eba1                	c.bnez	a5,1000084c <fal_flash_init+0x60>
100007fe:	f0006297          	auipc	t0,0xf0006
10000802:	7d228293          	addi	t0,t0,2002 # 6fd0 <beken_onchip_flash>
10000806:	0282a703          	lw	a4,40(t0)
1000080a:	c731                	c.beqz	a4,10000856 <fal_flash_init+0x6a>
1000080c:	02c2a303          	lw	t1,44(t0)
10000810:	06030163          	beqz	t1,10000872 <fal_flash_init+0x86>
10000814:	0302a383          	lw	t2,48(t0)
10000818:	06038b63          	beqz	t2,1000088e <fal_flash_init+0xa2>
1000081c:	0242a503          	lw	a0,36(t0)
10000820:	c111                	c.beqz	a0,10000824 <fal_flash_init+0x38>
10000822:	9502                	c.jalr	a0
10000824:	f0007597          	auipc	a1,0xf0007
10000828:	80458593          	addi	a1,a1,-2044 # 7028 <beken_onchip_flash_crc>
1000082c:	5590                	c.lw	a2,40(a1)
1000082e:	c605                	c.beqz	a2,10000856 <fal_flash_init+0x6a>
10000830:	55d4                	c.lw	a3,44(a1)
10000832:	c2a1                	c.beqz	a3,10000872 <fal_flash_init+0x86>
10000834:	0305a803          	lw	a6,48(a1)
10000838:	04080b63          	beqz	a6,1000088e <fal_flash_init+0xa2>
1000083c:	0245a883          	lw	a7,36(a1)
10000840:	00088363          	beqz	a7,10000846 <fal_flash_init+0x5a>
10000844:	9882                	c.jalr	a7
10000846:	4e05                	c.li	t3,1
10000848:	01c40023          	sb	t3,0(s0)
1000084c:	4501                	c.li	a0,0
1000084e:	f0005317          	auipc	t1,0xf0005
10000852:	52a30067          	jr	1322(t1) # 5d78 <__riscv_restore_0>
10000856:	00000617          	auipc	a2,0x0
1000085a:	74660613          	addi	a2,a2,1862 # 10000f9c <__FUNCTION__.1>
1000085e:	00000597          	auipc	a1,0x0
10000862:	6a258593          	addi	a1,a1,1698 # 10000f00 <flash_config+0xe4>
10000866:	00000517          	auipc	a0,0x0
1000086a:	6b650513          	addi	a0,a0,1718 # 10000f1c <flash_config+0x100>
1000086e:	3329                	c.jal	10000578 <bk_printf>
10000870:	a001                	c.j	10000870 <fal_flash_init+0x84>
10000872:	00000617          	auipc	a2,0x0
10000876:	72a60613          	addi	a2,a2,1834 # 10000f9c <__FUNCTION__.1>
1000087a:	00000597          	auipc	a1,0x0
1000087e:	6b658593          	addi	a1,a1,1718 # 10000f30 <flash_config+0x114>
10000882:	00000517          	auipc	a0,0x0
10000886:	69a50513          	addi	a0,a0,1690 # 10000f1c <flash_config+0x100>
1000088a:	31fd                	c.jal	10000578 <bk_printf>
1000088c:	a001                	c.j	1000088c <fal_flash_init+0xa0>
1000088e:	00000617          	auipc	a2,0x0
10000892:	70e60613          	addi	a2,a2,1806 # 10000f9c <__FUNCTION__.1>
10000896:	00000597          	auipc	a1,0x0
1000089a:	6b658593          	addi	a1,a1,1718 # 10000f4c <flash_config+0x130>
1000089e:	00000517          	auipc	a0,0x0
100008a2:	67e50513          	addi	a0,a0,1662 # 10000f1c <flash_config+0x100>
100008a6:	39c9                	c.jal	10000578 <bk_printf>
100008a8:	a001                	c.j	100008a8 <fal_flash_init+0xbc>

100008aa <fal_flash_device_find>:
100008aa:	f0005317          	auipc	t1,0xf0005
100008ae:	4aa302e7          	jalr	t0,1194(t1) # 5d54 <__riscv_save_0>
100008b2:	8591c783          	lbu	a5,-1959(gp) # 30000151 <init_ok>
100008b6:	ef99                	c.bnez	a5,100008d4 <fal_flash_device_find+0x2a>
100008b8:	00000617          	auipc	a2,0x0
100008bc:	6cc60613          	addi	a2,a2,1740 # 10000f84 <__FUNCTION__.0>
100008c0:	00000597          	auipc	a1,0x0
100008c4:	6a858593          	addi	a1,a1,1704 # 10000f68 <flash_config+0x14c>
100008c8:	00000517          	auipc	a0,0x0
100008cc:	65450513          	addi	a0,a0,1620 # 10000f1c <flash_config+0x100>
100008d0:	3165                	c.jal	10000578 <bk_printf>
100008d2:	a001                	c.j	100008d2 <fal_flash_device_find+0x28>
100008d4:	842a                	c.mv	s0,a0
100008d6:	c521                	c.beqz	a0,1000091e <fal_flash_device_find+0x74>
100008d8:	4661                	c.li	a2,24
100008da:	f0006597          	auipc	a1,0xf0006
100008de:	6f658593          	addi	a1,a1,1782 # 6fd0 <beken_onchip_flash>
100008e2:	f0006097          	auipc	ra,0xf0006
100008e6:	8da080e7          	jalr	-1830(ra) # 61bc <strncmp>
100008ea:	f0006297          	auipc	t0,0xf0006
100008ee:	6e628293          	addi	t0,t0,1766 # 6fd0 <beken_onchip_flash>
100008f2:	c10d                	c.beqz	a0,10000914 <fal_flash_device_find+0x6a>
100008f4:	4661                	c.li	a2,24
100008f6:	f0006597          	auipc	a1,0xf0006
100008fa:	73258593          	addi	a1,a1,1842 # 7028 <beken_onchip_flash_crc>
100008fe:	8522                	c.mv	a0,s0
10000900:	f0006097          	auipc	ra,0xf0006
10000904:	8bc080e7          	jalr	-1860(ra) # 61bc <strncmp>
10000908:	f0006297          	auipc	t0,0xf0006
1000090c:	72028293          	addi	t0,t0,1824 # 7028 <beken_onchip_flash_crc>
10000910:	c111                	c.beqz	a0,10000914 <fal_flash_device_find+0x6a>
10000912:	4281                	c.li	t0,0
10000914:	8516                	c.mv	a0,t0
10000916:	f0005317          	auipc	t1,0xf0005
1000091a:	46230067          	jr	1122(t1) # 5d78 <__riscv_restore_0>
1000091e:	00000617          	auipc	a2,0x0
10000922:	66660613          	addi	a2,a2,1638 # 10000f84 <__FUNCTION__.0>
10000926:	00000597          	auipc	a1,0x0
1000092a:	64a58593          	addi	a1,a1,1610 # 10000f70 <flash_config+0x154>
1000092e:	00000517          	auipc	a0,0x0
10000932:	5ee50513          	addi	a0,a0,1518 # 10000f1c <flash_config+0x100>
10000936:	3189                	c.jal	10000578 <bk_printf>
10000938:	a001                	c.j	10000938 <fal_flash_device_find+0x8e>

1000093a <fal_get_fw_hdr>:
1000093a:	f0005317          	auipc	t1,0xf0005
1000093e:	41a302e7          	jalr	t0,1050(t1) # 5d54 <__riscv_save_0>
10000942:	1141                	c.addi	sp,-16
10000944:	84aa                	c.mv	s1,a0
10000946:	c62e                	c.swsp	a1,12(sp)
10000948:	24a1                	c.jal	10000b90 <fal_partition_find>
1000094a:	00000597          	auipc	a1,0x0
1000094e:	62e58593          	addi	a1,a1,1582 # 10000f78 <flash_config+0x15c>
10000952:	842a                	c.mv	s0,a0
10000954:	8526                	c.mv	a0,s1
10000956:	f0005097          	auipc	ra,0xf0005
1000095a:	7c6080e7          	jalr	1990(ra) # 611c <strcmp>
1000095e:	4632                	c.lwsp	a2,12(sp)
10000960:	e511                	c.bnez	a0,1000096c <fal_get_fw_hdr+0x32>
10000962:	06000693          	li	a3,96
10000966:	4581                	c.li	a1,0
10000968:	8522                	c.mv	a0,s0
1000096a:	26a1                	c.jal	10000cb2 <fal_partition_read>
1000096c:	4501                	c.li	a0,0
1000096e:	0141                	c.addi	sp,16
10000970:	f0005317          	auipc	t1,0xf0005
10000974:	40830067          	jr	1032(t1) # 5d78 <__riscv_restore_0>

10000978 <fal_show_part_table>:
10000978:	f0005317          	auipc	t1,0xf0005
1000097c:	3c2302e7          	jalr	t0,962(t1) # 5d3a <__riscv_save_4>
10000980:	00000517          	auipc	a0,0x0
10000984:	5f050513          	addi	a0,a0,1520 # 10000f70 <flash_config+0x154>
10000988:	f0006097          	auipc	ra,0xf0006
1000098c:	804080e7          	jalr	-2044(ra) # 618c <strlen>
10000990:	84aa                	c.mv	s1,a0
10000992:	00000517          	auipc	a0,0x0
10000996:	61a50513          	addi	a0,a0,1562 # 10000fac <__FUNCTION__.1+0x10>
1000099a:	85c18a13          	addi	s4,gp,-1956 # 30000154 <partition_table_len>
1000099e:	f0005097          	auipc	ra,0xf0005
100009a2:	7ee080e7          	jalr	2030(ra) # 618c <strlen>
100009a6:	000a2783          	lw	a5,0(s4)
100009aa:	cfa1                	c.beqz	a5,10000a02 <fal_show_part_table+0x8a>
100009ac:	892a                	c.mv	s2,a0
100009ae:	4981                	c.li	s3,0
100009b0:	86418b13          	addi	s6,gp,-1948 # 3000015c <partition_table>
100009b4:	000b2083          	lw	ra,0(s6)
100009b8:	00699413          	slli	s0,s3,0x6
100009bc:	9406                	c.add	s0,ra
100009be:	00440a93          	addi	s5,s0,4
100009c2:	8556                	c.mv	a0,s5
100009c4:	f0005097          	auipc	ra,0xf0005
100009c8:	7c8080e7          	jalr	1992(ra) # 618c <strlen>
100009cc:	00a4f863          	bgeu	s1,a0,100009dc <fal_show_part_table+0x64>
100009d0:	8556                	c.mv	a0,s5
100009d2:	f0005097          	auipc	ra,0xf0005
100009d6:	7ba080e7          	jalr	1978(ra) # 618c <strlen>
100009da:	84aa                	c.mv	s1,a0
100009dc:	0471                	c.addi	s0,28
100009de:	8522                	c.mv	a0,s0
100009e0:	f0005097          	auipc	ra,0xf0005
100009e4:	7ac080e7          	jalr	1964(ra) # 618c <strlen>
100009e8:	00a97863          	bgeu	s2,a0,100009f8 <fal_show_part_table+0x80>
100009ec:	8522                	c.mv	a0,s0
100009ee:	f0005097          	auipc	ra,0xf0005
100009f2:	79e080e7          	jalr	1950(ra) # 618c <strlen>
100009f6:	892a                	c.mv	s2,a0
100009f8:	000a2283          	lw	t0,0(s4)
100009fc:	0985                	c.addi	s3,1
100009fe:	fa59ebe3          	bltu	s3,t0,100009b4 <fal_show_part_table+0x3c>
10000a02:	f0005317          	auipc	t1,0xf0005
10000a06:	36c30067          	jr	876(t1) # 5d6e <__riscv_restore_4>

10000a0a <fal_partition_init>:
10000a0a:	f0005317          	auipc	t1,0xf0005
10000a0e:	322302e7          	jalr	t0,802(t1) # 5d2c <__riscv_save_10>
10000a12:	86018a93          	addi	s5,gp,-1952 # 30000158 <init_ok>
10000a16:	000aca03          	lbu	s4,0(s5)
10000a1a:	7139                	c.addi16sp	sp,-64
10000a1c:	000a0963          	beqz	s4,10000a2e <fal_partition_init+0x24>
10000a20:	85c1a503          	lw	a0,-1956(gp) # 30000154 <partition_table_len>
10000a24:	6121                	c.addi16sp	sp,64
10000a26:	f0005317          	auipc	t1,0xf0005
10000a2a:	33e30067          	jr	830(t1) # 5d64 <__riscv_restore_10>
10000a2e:	00000517          	auipc	a0,0x0
10000a32:	58a50513          	addi	a0,a0,1418 # 10000fb8 <__FUNCTION__.1+0x1c>
10000a36:	3d95                	c.jal	100008aa <fal_flash_device_find>
10000a38:	89aa                	c.mv	s3,a0
10000a3a:	e119                	c.bnez	a0,10000a40 <fal_partition_init+0x36>
10000a3c:	3f35                	c.jal	10000978 <fal_show_part_table>
10000a3e:	b7cd                	c.j	10000a20 <fal_partition_init+0x16>
10000a40:	4d5c                	c.lw	a5,28(a0)
10000a42:	6441                	c.lui	s0,0x10
10000a44:	fef45ce3          	bge	s0,a5,10000a3c <fal_partition_init+0x32>
10000a48:	8474                	exec.it	#59     !li	a0,64
10000a4a:	f0005097          	auipc	ra,0xf0005
10000a4e:	33a080e7          	jalr	826(ra) # 5d84 <malloc>
10000a52:	fc040413          	addi	s0,s0,-64 # ffc0 <_data_lmastart+0x667a>
10000a56:	84aa                	c.mv	s1,a0
10000a58:	cd59                	c.beqz	a0,10000af6 <fal_partition_init+0xec>
10000a5a:	45503937          	lui	s2,0x45503
10000a5e:	13090913          	addi	s2,s2,304 # 45503130 <_stack+0x154a3130>
10000a62:	03f00b13          	li	s6,63
10000a66:	0289a303          	lw	t1,40(s3)
10000a6a:	8410                	exec.it	#3     !li	a2,64
10000a6c:	858a                	c.mv	a1,sp
10000a6e:	8522                	c.mv	a0,s0
10000a70:	9302                	c.jalr	t1
10000a72:	02a05e63          	blez	a0,10000aae <fal_partition_init+0xa4>
10000a76:	870a                	c.mv	a4,sp
10000a78:	4681                	c.li	a3,0
10000a7a:	00174083          	lbu	ra,1(a4) # 45840001 <_stack+0x157e0001>
10000a7e:	00074603          	lbu	a2,0(a4)
10000a82:	00274503          	lbu	a0,2(a4)
10000a86:	00374b83          	lbu	s7,3(a4)
10000a8a:	00809293          	slli	t0,ra,0x8
10000a8e:	00c283b3          	add	t2,t0,a2
10000a92:	01051813          	slli	a6,a0,0x10
10000a96:	010388b3          	add	a7,t2,a6
10000a9a:	018b9c93          	slli	s9,s7,0x18
10000a9e:	01988e33          	add	t3,a7,s9
10000aa2:	85b6                	c.mv	a1,a3
10000aa4:	0685                	c.addi	a3,1
10000aa6:	072e1063          	bne	t3,s2,10000b06 <fal_partition_init+0xfc>
10000aaa:	942e                	c.add	s0,a1
10000aac:	4a05                	c.li	s4,1
10000aae:	45503eb7          	lui	t4,0x45503
10000ab2:	4901                	c.li	s2,0
10000ab4:	fc000c93          	li	s9,-64
10000ab8:	130e8b13          	addi	s6,t4,304 # 45503130 <_stack+0x154a3130>
10000abc:	86418b93          	addi	s7,gp,-1948 # 3000015c <partition_table>
10000ac0:	020a0963          	beqz	s4,10000af2 <fal_partition_init+0xe8>
10000ac4:	864a                	c.mv	a2,s2
10000ac6:	4581                	c.li	a1,0
10000ac8:	8526                	c.mv	a0,s1
10000aca:	f0005097          	auipc	ra,0xf0005
10000ace:	47a080e7          	jalr	1146(ra) # 5f44 <memset>
10000ad2:	03990f33          	mul	t5,s2,s9
10000ad6:	0289af83          	lw	t6,40(s3)
10000ada:	8410                	exec.it	#3     !li	a2,64
10000adc:	85a6                	c.mv	a1,s1
10000ade:	008f0533          	add	a0,t5,s0
10000ae2:	00691c13          	slli	s8,s2,0x6
10000ae6:	9f82                	c.jalr	t6
10000ae8:	00054763          	bltz	a0,10000af6 <fal_partition_init+0xec>
10000aec:	409c                	c.lw	a5,0(s1)
10000aee:	03678763          	beq	a5,s6,10000b1c <fal_partition_init+0x112>
10000af2:	04091963          	bnez	s2,10000b44 <fal_partition_init+0x13a>
10000af6:	3549                	c.jal	10000978 <fal_show_part_table>
10000af8:	d485                	c.beqz	s1,10000a20 <fal_partition_init+0x16>
10000afa:	8526                	c.mv	a0,s1
10000afc:	f0005097          	auipc	ra,0xf0005
10000b00:	28e080e7          	jalr	654(ra) # 5d8a <free>
10000b04:	bf31                	c.j	10000a20 <fal_partition_init+0x16>
10000b06:	0705                	c.addi	a4,1
10000b08:	b7d6e9db          	bnec	a3,61,10000a7a <fal_partition_init+0x70>
10000b0c:	008b5563          	bge	s6,s0,10000b16 <fal_partition_init+0x10c>
10000b10:	fc340413          	addi	s0,s0,-61
10000b14:	bf89                	c.j	10000a66 <fal_partition_init+0x5c>
10000b16:	dc41                	c.beqz	s0,10000aae <fal_partition_init+0xa4>
10000b18:	4401                	c.li	s0,0
10000b1a:	b7b1                	c.j	10000a66 <fal_partition_init+0x5c>
10000b1c:	000ba503          	lw	a0,0(s7) # 40000000 <_stack+0xffa0000>
10000b20:	040c0593          	addi	a1,s8,64 # 2c000040 <_itcm_ema_end+0x1bffedaa>
10000b24:	f0005097          	auipc	ra,0xf0005
10000b28:	5ee080e7          	jalr	1518(ra) # 6112 <realloc>
10000b2c:	00aba023          	sw	a0,0(s7)
10000b30:	0905                	c.addi	s2,1
10000b32:	d171                	c.beqz	a0,10000af6 <fal_partition_init+0xec>
10000b34:	8410                	exec.it	#3     !li	a2,64
10000b36:	85a6                	c.mv	a1,s1
10000b38:	9562                	c.add	a0,s8
10000b3a:	f0005097          	auipc	ra,0xf0005
10000b3e:	29a080e7          	jalr	666(ra) # 5dd4 <memcpy>
10000b42:	bfbd                	c.j	10000ac0 <fal_partition_init+0xb6>
10000b44:	85c18993          	addi	s3,gp,-1956 # 30000154 <partition_table_len>
10000b48:	0129a023          	sw	s2,0(s3)
10000b4c:	4401                	c.li	s0,0
10000b4e:	86418913          	addi	s2,gp,-1948 # 3000015c <partition_table>
10000b52:	00092303          	lw	t1,0(s2)
10000b56:	00641a13          	slli	s4,s0,0x6
10000b5a:	014305b3          	add	a1,t1,s4
10000b5e:	01c58513          	addi	a0,a1,28
10000b62:	33a1                	c.jal	100008aa <fal_flash_device_find>
10000b64:	cd09                	c.beqz	a0,10000b7e <fal_partition_init+0x174>
10000b66:	00092683          	lw	a3,0(s2)
10000b6a:	4d50                	c.lw	a2,28(a0)
10000b6c:	014680b3          	add	ra,a3,s4
10000b70:	0340a283          	lw	t0,52(ra)
10000b74:	00c2c563          	blt	t0,a2,10000b7e <fal_partition_init+0x174>
10000b78:	8401ae23          	sw	zero,-1956(gp) # 30000154 <partition_table_len>
10000b7c:	bfad                	c.j	10000af6 <fal_partition_init+0xec>
10000b7e:	0009a383          	lw	t2,0(s3)
10000b82:	0405                	c.addi	s0,1
10000b84:	fc7467e3          	bltu	s0,t2,10000b52 <fal_partition_init+0x148>
10000b88:	4505                	c.li	a0,1
10000b8a:	00aa8023          	sb	a0,0(s5)
10000b8e:	b7a5                	c.j	10000af6 <fal_partition_init+0xec>

10000b90 <fal_partition_find>:
10000b90:	f0005317          	auipc	t1,0xf0005
10000b94:	1aa302e7          	jalr	t0,426(t1) # 5d3a <__riscv_save_4>
10000b98:	8a14                	exec.it	#77     !lbu	a5,-1952(gp) # 30000158 <init_ok>
10000b9a:	cf89                	c.beqz	a5,10000bb4 <fal_partition_find+0x24>
10000b9c:	84aa                	c.mv	s1,a0
10000b9e:	4401                	c.li	s0,0
10000ba0:	85c18a13          	addi	s4,gp,-1956 # 30000154 <partition_table_len>
10000ba4:	86418913          	addi	s2,gp,-1948 # 3000015c <partition_table>
10000ba8:	000a2283          	lw	t0,0(s4)
10000bac:	02546263          	bltu	s0,t0,10000bd0 <fal_partition_find+0x40>
10000bb0:	4501                	c.li	a0,0
10000bb2:	a83d                	c.j	10000bf0 <fal_partition_find+0x60>
10000bb4:	00000617          	auipc	a2,0x0
10000bb8:	4a460613          	addi	a2,a2,1188 # 10001058 <__FUNCTION__.5>
10000bbc:	00000597          	auipc	a1,0x0
10000bc0:	3ac58593          	addi	a1,a1,940 # 10000f68 <flash_config+0x14c>
10000bc4:	00000517          	auipc	a0,0x0
10000bc8:	35850513          	addi	a0,a0,856 # 10000f1c <flash_config+0x100>
10000bcc:	3275                	c.jal	10000578 <bk_printf>
10000bce:	a001                	c.j	10000bce <fal_partition_find+0x3e>
10000bd0:	8054                	exec.it	#41     !lw	a1,0(s2)
10000bd2:	00641993          	slli	s3,s0,0x6
10000bd6:	013580b3          	add	ra,a1,s3
10000bda:	00408593          	addi	a1,ra,4
10000bde:	8526                	c.mv	a0,s1
10000be0:	f0005097          	auipc	ra,0xf0005
10000be4:	53c080e7          	jalr	1340(ra) # 611c <strcmp>
10000be8:	e901                	c.bnez	a0,10000bf8 <fal_partition_find+0x68>
10000bea:	00092503          	lw	a0,0(s2)
10000bee:	954e                	c.add	a0,s3
10000bf0:	f0005317          	auipc	t1,0xf0005
10000bf4:	17e30067          	jr	382(t1) # 5d6e <__riscv_restore_4>
10000bf8:	0405                	c.addi	s0,1
10000bfa:	b77d                	c.j	10000ba8 <fal_partition_find+0x18>

10000bfc <fal_get_partition_table>:
10000bfc:	f0005317          	auipc	t1,0xf0005
10000c00:	158302e7          	jalr	t0,344(t1) # 5d54 <__riscv_save_0>
10000c04:	8a14                	exec.it	#77     !lbu	a5,-1952(gp) # 30000158 <init_ok>
10000c06:	ef99                	c.bnez	a5,10000c24 <fal_get_partition_table+0x28>
10000c08:	00000617          	auipc	a2,0x0
10000c0c:	43860613          	addi	a2,a2,1080 # 10001040 <__FUNCTION__.4>
10000c10:	00000597          	auipc	a1,0x0
10000c14:	35858593          	addi	a1,a1,856 # 10000f68 <flash_config+0x14c>
10000c18:	00000517          	auipc	a0,0x0
10000c1c:	30450513          	addi	a0,a0,772 # 10000f1c <flash_config+0x100>
10000c20:	3aa1                	c.jal	10000578 <bk_printf>
10000c22:	a001                	c.j	10000c22 <fal_get_partition_table+0x26>
10000c24:	e105                	c.bnez	a0,10000c44 <fal_get_partition_table+0x48>
10000c26:	00000617          	auipc	a2,0x0
10000c2a:	41a60613          	addi	a2,a2,1050 # 10001040 <__FUNCTION__.4>
10000c2e:	00000597          	auipc	a1,0x0
10000c32:	39e58593          	addi	a1,a1,926 # 10000fcc <__FUNCTION__.1+0x30>
10000c36:	00000517          	auipc	a0,0x0
10000c3a:	2e650513          	addi	a0,a0,742 # 10000f1c <flash_config+0x100>
10000c3e:	93bff0ef          	jal	ra,10000578 <bk_printf>
10000c42:	a001                	c.j	10000c42 <fal_get_partition_table+0x46>
10000c44:	85c1a283          	lw	t0,-1956(gp) # 30000154 <partition_table_len>
10000c48:	00552023          	sw	t0,0(a0)
10000c4c:	8641a503          	lw	a0,-1948(gp) # 3000015c <partition_table>
10000c50:	f0005317          	auipc	t1,0xf0005
10000c54:	12830067          	jr	296(t1) # 5d78 <__riscv_restore_0>

10000c58 <fal_set_partition_table_temp>:
10000c58:	f0005317          	auipc	t1,0xf0005
10000c5c:	0fc302e7          	jalr	t0,252(t1) # 5d54 <__riscv_save_0>
10000c60:	8a14                	exec.it	#77     !lbu	a5,-1952(gp) # 30000158 <init_ok>
10000c62:	e385                	c.bnez	a5,10000c82 <fal_set_partition_table_temp+0x2a>
10000c64:	00000617          	auipc	a2,0x0
10000c68:	3bc60613          	addi	a2,a2,956 # 10001020 <__FUNCTION__.3>
10000c6c:	00000597          	auipc	a1,0x0
10000c70:	2fc58593          	addi	a1,a1,764 # 10000f68 <flash_config+0x14c>
10000c74:	00000517          	auipc	a0,0x0
10000c78:	2a850513          	addi	a0,a0,680 # 10000f1c <flash_config+0x100>
10000c7c:	8fdff0ef          	jal	ra,10000578 <bk_printf>
10000c80:	a001                	c.j	10000c80 <fal_set_partition_table_temp+0x28>
10000c82:	e105                	c.bnez	a0,10000ca2 <fal_set_partition_table_temp+0x4a>
10000c84:	00000617          	auipc	a2,0x0
10000c88:	39c60613          	addi	a2,a2,924 # 10001020 <__FUNCTION__.3>
10000c8c:	00000597          	auipc	a1,0x0
10000c90:	34458593          	addi	a1,a1,836 # 10000fd0 <__FUNCTION__.1+0x34>
10000c94:	00000517          	auipc	a0,0x0
10000c98:	28850513          	addi	a0,a0,648 # 10000f1c <flash_config+0x100>
10000c9c:	8ddff0ef          	jal	ra,10000578 <bk_printf>
10000ca0:	a001                	c.j	10000ca0 <fal_set_partition_table_temp+0x48>
10000ca2:	84b1ae23          	sw	a1,-1956(gp) # 30000154 <partition_table_len>
10000ca6:	86a1a223          	sw	a0,-1948(gp) # 3000015c <partition_table>
10000caa:	f0005317          	auipc	t1,0xf0005
10000cae:	0ce30067          	jr	206(t1) # 5d78 <__riscv_restore_0>

10000cb2 <fal_partition_read>:
10000cb2:	f0005317          	auipc	t1,0xf0005
10000cb6:	0a2302e7          	jalr	t0,162(t1) # 5d54 <__riscv_save_0>
10000cba:	1141                	c.addi	sp,-16
10000cbc:	e105                	c.bnez	a0,10000cdc <fal_partition_read+0x2a>
10000cbe:	00000617          	auipc	a2,0x0
10000cc2:	34e60613          	addi	a2,a2,846 # 1000100c <__FUNCTION__.2>
10000cc6:	00000597          	auipc	a1,0x0
10000cca:	31258593          	addi	a1,a1,786 # 10000fd8 <__FUNCTION__.1+0x3c>
10000cce:	00000517          	auipc	a0,0x0
10000cd2:	24e50513          	addi	a0,a0,590 # 10000f1c <flash_config+0x100>
10000cd6:	8a3ff0ef          	jal	ra,10000578 <bk_printf>
10000cda:	a001                	c.j	10000cda <fal_partition_read+0x28>
10000cdc:	84ae                	c.mv	s1,a1
10000cde:	e205                	c.bnez	a2,10000cfe <fal_partition_read+0x4c>
10000ce0:	00000617          	auipc	a2,0x0
10000ce4:	32c60613          	addi	a2,a2,812 # 1000100c <__FUNCTION__.2>
10000ce8:	00000597          	auipc	a1,0x0
10000cec:	2f858593          	addi	a1,a1,760 # 10000fe0 <__FUNCTION__.1+0x44>
10000cf0:	00000517          	auipc	a0,0x0
10000cf4:	22c50513          	addi	a0,a0,556 # 10000f1c <flash_config+0x100>
10000cf8:	881ff0ef          	jal	ra,10000578 <bk_printf>
10000cfc:	a001                	c.j	10000cfc <fal_partition_read+0x4a>
10000cfe:	5d18                	c.lw	a4,56(a0)
10000d00:	00d487b3          	add	a5,s1,a3
10000d04:	842a                	c.mv	s0,a0
10000d06:	00f77863          	bgeu	a4,a5,10000d16 <fal_partition_read+0x64>
10000d0a:	557d                	c.li	a0,-1
10000d0c:	0141                	c.addi	sp,16
10000d0e:	f0005317          	auipc	t1,0xf0005
10000d12:	06a30067          	jr	106(t1) # 5d78 <__riscv_restore_0>
10000d16:	0571                	c.addi	a0,28
10000d18:	c636                	c.swsp	a3,12(sp)
10000d1a:	c432                	c.swsp	a2,8(sp)
10000d1c:	3679                	c.jal	100008aa <fal_flash_device_find>
10000d1e:	d575                	c.beqz	a0,10000d0a <fal_partition_read+0x58>
10000d20:	8610                	exec.it	#67     !lw	ra,52(s0)
10000d22:	02852303          	lw	t1,40(a0)
10000d26:	4632                	c.lwsp	a2,12(sp)
10000d28:	45a2                	c.lwsp	a1,8(sp)
10000d2a:	8244                	exec.it	#104     !add	a0,s1,ra
10000d2c:	9302                	c.jalr	t1
10000d2e:	bff9                	c.j	10000d0c <fal_partition_read+0x5a>

10000d30 <fal_partition_write>:
10000d30:	f0005317          	auipc	t1,0xf0005
10000d34:	024302e7          	jalr	t0,36(t1) # 5d54 <__riscv_save_0>
10000d38:	1141                	c.addi	sp,-16
10000d3a:	e105                	c.bnez	a0,10000d5a <fal_partition_write+0x2a>
10000d3c:	00000617          	auipc	a2,0x0
10000d40:	2bc60613          	addi	a2,a2,700 # 10000ff8 <__FUNCTION__.1>
10000d44:	00000597          	auipc	a1,0x0
10000d48:	29458593          	addi	a1,a1,660 # 10000fd8 <__FUNCTION__.1+0x3c>
10000d4c:	00000517          	auipc	a0,0x0
10000d50:	1d050513          	addi	a0,a0,464 # 10000f1c <flash_config+0x100>
10000d54:	825ff0ef          	jal	ra,10000578 <bk_printf>
10000d58:	a001                	c.j	10000d58 <fal_partition_write+0x28>
10000d5a:	84ae                	c.mv	s1,a1
10000d5c:	e205                	c.bnez	a2,10000d7c <fal_partition_write+0x4c>
10000d5e:	00000617          	auipc	a2,0x0
10000d62:	29a60613          	addi	a2,a2,666 # 10000ff8 <__FUNCTION__.1>
10000d66:	00000597          	auipc	a1,0x0
10000d6a:	27a58593          	addi	a1,a1,634 # 10000fe0 <__FUNCTION__.1+0x44>
10000d6e:	00000517          	auipc	a0,0x0
10000d72:	1ae50513          	addi	a0,a0,430 # 10000f1c <flash_config+0x100>
10000d76:	803ff0ef          	jal	ra,10000578 <bk_printf>
10000d7a:	a001                	c.j	10000d7a <fal_partition_write+0x4a>
10000d7c:	5d18                	c.lw	a4,56(a0)
10000d7e:	00d487b3          	add	a5,s1,a3
10000d82:	842a                	c.mv	s0,a0
10000d84:	00f77863          	bgeu	a4,a5,10000d94 <fal_partition_write+0x64>
10000d88:	557d                	c.li	a0,-1
10000d8a:	0141                	c.addi	sp,16
10000d8c:	f0005317          	auipc	t1,0xf0005
10000d90:	fec30067          	jr	-20(t1) # 5d78 <__riscv_restore_0>
10000d94:	0571                	c.addi	a0,28
10000d96:	c636                	c.swsp	a3,12(sp)
10000d98:	c432                	c.swsp	a2,8(sp)
10000d9a:	3e01                	c.jal	100008aa <fal_flash_device_find>
10000d9c:	d575                	c.beqz	a0,10000d88 <fal_partition_write+0x58>
10000d9e:	8610                	exec.it	#67     !lw	ra,52(s0)
10000da0:	02c52303          	lw	t1,44(a0)
10000da4:	4632                	c.lwsp	a2,12(sp)
10000da6:	45a2                	c.lwsp	a1,8(sp)
10000da8:	8244                	exec.it	#104     !add	a0,s1,ra
10000daa:	9302                	c.jalr	t1
10000dac:	bff9                	c.j	10000d8a <fal_partition_write+0x5a>

10000dae <fal_partition_erase>:
10000dae:	f0005317          	auipc	t1,0xf0005
10000db2:	fa6302e7          	jalr	t0,-90(t1) # 5d54 <__riscv_save_0>
10000db6:	1141                	c.addi	sp,-16
10000db8:	e105                	c.bnez	a0,10000dd8 <fal_partition_erase+0x2a>
10000dba:	00000617          	auipc	a2,0x0
10000dbe:	22a60613          	addi	a2,a2,554 # 10000fe4 <__FUNCTION__.0>
10000dc2:	00000597          	auipc	a1,0x0
10000dc6:	21658593          	addi	a1,a1,534 # 10000fd8 <__FUNCTION__.1+0x3c>
10000dca:	00000517          	auipc	a0,0x0
10000dce:	15250513          	addi	a0,a0,338 # 10000f1c <flash_config+0x100>
10000dd2:	fa6ff0ef          	jal	ra,10000578 <bk_printf>
10000dd6:	a001                	c.j	10000dd6 <fal_partition_erase+0x28>
10000dd8:	5d18                	c.lw	a4,56(a0)
10000dda:	00c587b3          	add	a5,a1,a2
10000dde:	842a                	c.mv	s0,a0
10000de0:	84ae                	c.mv	s1,a1
10000de2:	00f77863          	bgeu	a4,a5,10000df2 <fal_partition_erase+0x44>
10000de6:	557d                	c.li	a0,-1
10000de8:	0141                	c.addi	sp,16
10000dea:	f0005317          	auipc	t1,0xf0005
10000dee:	f8e30067          	jr	-114(t1) # 5d78 <__riscv_restore_0>
10000df2:	0571                	c.addi	a0,28
10000df4:	c632                	c.swsp	a2,12(sp)
10000df6:	3c55                	c.jal	100008aa <fal_flash_device_find>
10000df8:	d57d                	c.beqz	a0,10000de6 <fal_partition_erase+0x38>
10000dfa:	8610                	exec.it	#67     !lw	ra,52(s0)
10000dfc:	03052303          	lw	t1,48(a0)
10000e00:	45b2                	c.lwsp	a1,12(sp)
10000e02:	8244                	exec.it	#104     !add	a0,s1,ra
10000e04:	9302                	c.jalr	t1
10000e06:	b7cd                	c.j	10000de8 <fal_partition_erase+0x3a>

10000e08 <fal_partition_erase_all>:
10000e08:	5d10                	c.lw	a2,56(a0)
10000e0a:	4581                	c.li	a1,0
10000e0c:	b74d                	c.j	10000dae <fal_partition_erase>
10000e0e:	0001                	c.nop

Disassembly of section .itcm_write_flash:

10001070 <bl_hw_board_deinit>:
10001070:	f0005317          	auipc	t1,0xf0005
10001074:	ce4302e7          	jalr	t0,-796(t1) # 5d54 <__riscv_save_0>
10001078:	fffff097          	auipc	ra,0xfffff
1000107c:	48e080e7          	jalr	1166(ra) # 10000506 <uart_deinit>
10001080:	4501                	c.li	a0,0
10001082:	f0002097          	auipc	ra,0xf0002
10001086:	a8a080e7          	jalr	-1398(ra) # 2b0c <arch_interrupt_ctrl>
1000108a:	f0005317          	auipc	t1,0xf0005
1000108e:	cee30067          	jr	-786(t1) # 5d78 <__riscv_restore_0>

10001092 <bl_hw_board_init>:
10001092:	f0005317          	auipc	t1,0xf0005
10001096:	cc2302e7          	jalr	t0,-830(t1) # 5d54 <__riscv_save_0>
1000109a:	fffff097          	auipc	ra,0xfffff
1000109e:	440080e7          	jalr	1088(ra) # 100004da <uart_init>
100010a2:	4509                	c.li	a0,2
100010a4:	fffff097          	auipc	ra,0xfffff
100010a8:	1a8080e7          	jalr	424(ra) # 1000024c <flash_set_line_mode>
100010ac:	4515                	c.li	a0,5
100010ae:	fffff097          	auipc	ra,0xfffff
100010b2:	1dc080e7          	jalr	476(ra) # 1000028a <flash_set_clk>
100010b6:	fffff097          	auipc	ra,0xfffff
100010ba:	f9a080e7          	jalr	-102(ra) # 10000050 <get_flash_ID>
100010be:	fffff097          	auipc	ra,0xfffff
100010c2:	f46080e7          	jalr	-186(ra) # 10000004 <flash_get_current_flash_config>
100010c6:	4505                	c.li	a0,1
100010c8:	f0002097          	auipc	ra,0xf0002
100010cc:	a44080e7          	jalr	-1468(ra) # 2b0c <arch_interrupt_ctrl>
100010d0:	f0005317          	auipc	t1,0xf0005
100010d4:	ca830067          	jr	-856(t1) # 5d78 <__riscv_restore_0>

100010d8 <boot_main>:
100010d8:	f0005317          	auipc	t1,0xf0005
100010dc:	c7c302e7          	jalr	t0,-900(t1) # 5d54 <__riscv_save_0>
100010e0:	3f4d                	c.jal	10001092 <bl_hw_board_init>
100010e2:	f0006517          	auipc	a0,0xf0006
100010e6:	dca50513          	addi	a0,a0,-566 # 6eac <_ITB_BASE_+0x1ec>
100010ea:	fffff097          	auipc	ra,0xfffff
100010ee:	48e080e7          	jalr	1166(ra) # 10000578 <bk_printf>
100010f2:	2219                	c.jal	100011f8 <system_timeout_startup>
100010f4:	4501                	c.li	a0,0
100010f6:	f0005317          	auipc	t1,0xf0005
100010fa:	c8230067          	jr	-894(t1) # 5d78 <__riscv_restore_0>

100010fe <ota_main>:
100010fe:	f0005317          	auipc	t1,0xf0005
10001102:	c56302e7          	jalr	t0,-938(t1) # 5d54 <__riscv_save_0>
10001106:	222d                	c.jal	10001230 <fal_init>
10001108:	57fd                	c.li	a5,-1
1000110a:	842a                	c.mv	s0,a0
1000110c:	00f51f63          	bne	a0,a5,1000112a <ota_main+0x2c>
10001110:	f0006517          	auipc	a0,0xf0006
10001114:	db850513          	addi	a0,a0,-584 # 6ec8 <_ITB_BASE_+0x208>
10001118:	fffff097          	auipc	ra,0xfffff
1000111c:	460080e7          	jalr	1120(ra) # 10000578 <bk_printf>
10001120:	8522                	c.mv	a0,s0
10001122:	f0005317          	auipc	t1,0xf0005
10001126:	c5630067          	jr	-938(t1) # 5d78 <__riscv_restore_0>
1000112a:	50f9                	c.li	ra,-2
1000112c:	00151763          	bne	a0,ra,1000113a <ota_main+0x3c>
10001130:	f0006517          	auipc	a0,0xf0006
10001134:	db850513          	addi	a0,a0,-584 # 6ee8 <_ITB_BASE_+0x228>
10001138:	b7c5                	c.j	10001118 <ota_main+0x1a>
1000113a:	f0006517          	auipc	a0,0xf0006
1000113e:	dce50513          	addi	a0,a0,-562 # 6f08 <_ITB_BASE_+0x248>
10001142:	fffff097          	auipc	ra,0xfffff
10001146:	436080e7          	jalr	1078(ra) # 10000578 <bk_printf>
1000114a:	4501                	c.li	a0,0
1000114c:	fffff097          	auipc	ra,0xfffff
10001150:	04e080e7          	jalr	78(ra) # 1000019a <set_flash_protect>
10001154:	4501                	c.li	a0,0
10001156:	f0003097          	auipc	ra,0xf0003
1000115a:	cf0080e7          	jalr	-784(ra) # 3e46 <ty_bsdiff_entry>
1000115e:	c909                	c.beqz	a0,10001170 <ota_main+0x72>
10001160:	f0006517          	auipc	a0,0xf0006
10001164:	dbc50513          	addi	a0,a0,-580 # 6f1c <_ITB_BASE_+0x25c>
10001168:	fffff097          	auipc	ra,0xfffff
1000116c:	410080e7          	jalr	1040(ra) # 10000578 <bk_printf>
10001170:	f0007517          	auipc	a0,0xf0007
10001174:	99450513          	addi	a0,a0,-1644 # 7b04 <irq_handler+0xaa8>
10001178:	fffff097          	auipc	ra,0xfffff
1000117c:	400080e7          	jalr	1024(ra) # 10000578 <bk_printf>
10001180:	f0006517          	auipc	a0,0xf0006
10001184:	db450513          	addi	a0,a0,-588 # 6f34 <_ITB_BASE_+0x274>
10001188:	fffff097          	auipc	ra,0xfffff
1000118c:	3f0080e7          	jalr	1008(ra) # 10000578 <bk_printf>
10001190:	40800533          	neg	a0,s0
10001194:	fffff097          	auipc	ra,0xfffff
10001198:	458080e7          	jalr	1112(ra) # 100005ec <bk_print_hex>
1000119c:	f0007517          	auipc	a0,0xf0007
100011a0:	96850513          	addi	a0,a0,-1688 # 7b04 <irq_handler+0xaa8>
100011a4:	fffff097          	auipc	ra,0xfffff
100011a8:	3d4080e7          	jalr	980(ra) # 10000578 <bk_printf>
100011ac:	ec09                	c.bnez	s0,100011c6 <ota_main+0xc8>
100011ae:	f0006517          	auipc	a0,0xf0006
100011b2:	da250513          	addi	a0,a0,-606 # 6f50 <_ITB_BASE_+0x290>
100011b6:	fffff097          	auipc	ra,0xfffff
100011ba:	3c2080e7          	jalr	962(ra) # 10000578 <bk_printf>
100011be:	f0001097          	auipc	ra,0xf0001
100011c2:	29e080e7          	jalr	670(ra) # 245c <wdt_reboot>
100011c6:	4505                	c.li	a0,1
100011c8:	4401                	c.li	s0,0
100011ca:	fffff097          	auipc	ra,0xfffff
100011ce:	fd0080e7          	jalr	-48(ra) # 1000019a <set_flash_protect>
100011d2:	b7b9                	c.j	10001120 <ota_main+0x22>

100011d4 <system_startup>:
100011d4:	f0005317          	auipc	t1,0xf0005
100011d8:	b80302e7          	jalr	t0,-1152(t1) # 5d54 <__riscv_save_0>
100011dc:	370d                	c.jal	100010fe <ota_main>
100011de:	f0006517          	auipc	a0,0xf0006
100011e2:	d8e50513          	addi	a0,a0,-626 # 6f6c <_ITB_BASE_+0x2ac>
100011e6:	fffff097          	auipc	ra,0xfffff
100011ea:	392080e7          	jalr	914(ra) # 10000578 <bk_printf>
100011ee:	f0001097          	auipc	ra,0xf0001
100011f2:	2da080e7          	jalr	730(ra) # 24c8 <jump_to_app>
100011f6:	a001                	c.j	100011f6 <system_startup+0x22>

100011f8 <system_timeout_startup>:
100011f8:	8501a783          	lw	a5,-1968(gp) # 30000148 <uart_download_status>
100011fc:	0217d95b          	beqc	a5,1,1000122e <system_timeout_startup+0x36>
10001200:	f0005317          	auipc	t1,0xf0005
10001204:	b54302e7          	jalr	t0,-1196(t1) # 5d54 <__riscv_save_0>
10001208:	f0006517          	auipc	a0,0xf0006
1000120c:	d7850513          	addi	a0,a0,-648 # 6f80 <_ITB_BASE_+0x2c0>
10001210:	fffff097          	auipc	ra,0xfffff
10001214:	368080e7          	jalr	872(ra) # 10000578 <bk_printf>
10001218:	f0006517          	auipc	a0,0xf0006
1000121c:	d8c50513          	addi	a0,a0,-628 # 6fa4 <_ITB_BASE_+0x2e4>
10001220:	fffff097          	auipc	ra,0xfffff
10001224:	358080e7          	jalr	856(ra) # 10000578 <bk_printf>
10001228:	8001a623          	sw	zero,-2036(gp) # 30000104 <boot_downloading>
1000122c:	3765                	c.jal	100011d4 <system_startup>
1000122e:	8082                	c.jr	ra

10001230 <fal_init>:
10001230:	f0005317          	auipc	t1,0xf0005
10001234:	b24302e7          	jalr	t0,-1244(t1) # 5d54 <__riscv_save_0>
10001238:	fffff097          	auipc	ra,0xfffff
1000123c:	5b4080e7          	jalr	1460(ra) # 100007ec <fal_flash_init>
10001240:	842a                	c.mv	s0,a0
10001242:	02054963          	bltz	a0,10001274 <fal_init+0x44>
10001246:	fffff097          	auipc	ra,0xfffff
1000124a:	7c4080e7          	jalr	1988(ra) # 10000a0a <fal_partition_init>
1000124e:	842a                	c.mv	s0,a0
10001250:	f0006517          	auipc	a0,0xf0006
10001254:	eec50513          	addi	a0,a0,-276 # 713c <irq_handler+0xe0>
10001258:	00000097          	auipc	ra,0x0
1000125c:	938080e7          	jalr	-1736(ra) # 10000b90 <fal_partition_find>
10001260:	ed09                	c.bnez	a0,1000127a <fal_init+0x4a>
10001262:	f0006517          	auipc	a0,0xf0006
10001266:	ee650513          	addi	a0,a0,-282 # 7148 <irq_handler+0xec>
1000126a:	5479                	c.li	s0,-2
1000126c:	fffff097          	auipc	ra,0xfffff
10001270:	30c080e7          	jalr	780(ra) # 10000578 <bk_printf>
10001274:	84018c23          	sb	zero,-1960(gp) # 30000150 <init_ok>
10001278:	a811                	c.j	1000128c <fal_init+0x5c>
1000127a:	fe805de3          	blez	s0,10001274 <fal_init+0x44>
1000127e:	85818793          	addi	a5,gp,-1960 # 30000150 <init_ok>
10001282:	8854                	exec.it	#45     !lbu	a4,0(a5) # 45840000 <_stack+0x157e0000>
10001284:	e701                	c.bnez	a4,1000128c <fal_init+0x5c>
10001286:	4085                	c.li	ra,1
10001288:	00178023          	sb	ra,0(a5)
1000128c:	8522                	c.mv	a0,s0
1000128e:	f0005317          	auipc	t1,0xf0005
10001292:	aea30067          	jr	-1302(t1) # 5d78 <__riscv_restore_0>
