
./out/l_bootloader.elf:     file format elf32-littleriscv


Disassembly of section .nds_vector:

00000000 <reset_vector>:
   0:	342022f3          	csrr	t0,mcause
   4:	00028463          	beqz	t0,c <_start>
   8:	05a0006f          	j	62 <nmi_handler>

0000000c <_start>:
   c:	7ca027f3          	csrr	a5,mcache_ctl
  10:	0017e793          	ori	a5,a5,1
  14:	7ca79073          	csrw	mcache_ctl,a5
  18:	7ca027f3          	csrr	a5,mcache_ctl
  1c:	0027e793          	ori	a5,a5,2
  20:	7ca79073          	csrw	mcache_ctl,a5
  24:	30011197          	auipc	gp,0x30011
  28:	8d418193          	addi	gp,gp,-1836 # 300108f8 <__global_pointer$>
  2c:	30040297          	auipc	t0,0x30040
  30:	fd428293          	addi	t0,t0,-44 # 30040000 <_stack>
  34:	8116                	c.mv	sp,t0
  36:	00002297          	auipc	t0,0x2
  3a:	8be28293          	addi	t0,t0,-1858 # 18f4 <_ITB_BASE_>
  3e:	80029073          	csrw	uitb,t0
  42:	00001297          	auipc	t0,0x1
  46:	4ee28293          	addi	t0,t0,1262 # 1530 <trap_entry>
  4a:	30529073          	csrw	mtvec,t0
  4e:	00001097          	auipc	ra,0x1
  52:	3ec080e7          	jalr	1004(ra) # 143a <__platform_init>
  56:	00001097          	auipc	ra,0x1
  5a:	3d0080e7          	jalr	976(ra) # 1426 <reset_handler>
  5e:	a001                	c.j	5e <_start+0x52>
  60:	8082                	c.jr	ra

00000062 <nmi_handler>:
  62:	79f0f06f          	j	10000 <SRAM2_SIZE+0xd8f8>
  66:	a001                	c.j	66 <nmi_handler+0x4>
	...
 100:	4b42                	c.lwsp	s6,16(sp)
 102:	36353237          	lui	tp,0x36353
	...
 10e:	0000                	unimp
 110:	6568                	c.flw	fa0,76(a0)
 112:	6461                	c.lui	s0,0x18
	...

Disassembly of section .text:

00000130 <GPIO_UART_function_enable>:
     130:	ed0d                	c.bnez	a0,16a <GPIO_UART_function_enable+0x3a>
     132:	440107b7          	lui	a5,0x44010
     136:	44000eb7          	lui	t4,0x44000
     13a:	07c00f13          	li	t5,124
     13e:	43eea423          	sw	t5,1064(t4) # 44000428 <_stack+0x13fc0428>
     142:	08078293          	addi	t0,a5,128 # 44010080 <_stack+0x13fd0080>
     146:	07800f93          	li	t6,120
     14a:	43fea623          	sw	t6,1068(t4)
     14e:	7365                	c.lui	t1,0xffff9
     150:	0442a703          	lw	a4,68(t0)
     154:	8ff30393          	addi	t2,t1,-1793 # ffff88ff <_stack+0xcffb88ff>
     158:	00777533          	and	a0,a4,t2
     15c:	04a2a223          	sw	a0,68(t0)
     160:	0442a583          	lw	a1,68(t0)
     164:	04b2a223          	sw	a1,68(t0)
     168:	8082                	c.jr	ra
     16a:	02156b5b          	bnec	a0,1,1a0 <GPIO_UART_function_enable+0x70>
     16e:	440003b7          	lui	t2,0x44000
     172:	04800513          	li	a0,72
     176:	44010637          	lui	a2,0x44010
     17a:	40a3a023          	sw	a0,1024(t2) # 44000400 <_stack+0x13fc0400>
     17e:	08060693          	addi	a3,a2,128 # 44010080 <_stack+0x13fd0080>
     182:	07800593          	li	a1,120
     186:	40b3a223          	sw	a1,1028(t2)
     18a:	0406a803          	lw	a6,64(a3)
     18e:	f8887893          	andi	a7,a6,-120
     192:	0516a023          	sw	a7,64(a3)
     196:	0406ae03          	lw	t3,64(a3)
     19a:	05c6a023          	sw	t3,64(a3)
     19e:	8082                	c.jr	ra
     1a0:	00256e5b          	bnec	a0,2,1bc <GPIO_UART_function_enable+0x8c>
     1a4:	440107b7          	lui	a5,0x44010
     1a8:	0d47a703          	lw	a4,212(a5) # 440100d4 <_stack+0x13fd00d4>
     1ac:	08078293          	addi	t0,a5,128
     1b0:	04e2aa23          	sw	a4,84(t0)
     1b4:	0542a303          	lw	t1,84(t0)
     1b8:	0462aa23          	sw	t1,84(t0)
     1bc:	8082                	c.jr	ra

000001be <uart0_init>:
     1be:	8810                	exec.it	#5     !jal	t0,173e <__riscv_save_0>
     1c0:	8c00                	exec.it	#6     !lui	a5,0x44820
     1c2:	0007a023          	sw	zero,0(a5) # 44820000 <_stack+0x147e0000>
     1c6:	0e100413          	li	s0,225
     1ca:	c515                	c.beqz	a0,1f6 <uart0_init+0x38>
     1cc:	018cc0b7          	lui	ra,0x18cc
     1d0:	00155413          	srli	s0,a0,0x1
     1d4:	a7f08293          	addi	t0,ra,-1409 # 18cba7f <SRAM2_SIZE+0x18c9377>
     1d8:	00540333          	add	t1,s0,t0
     1dc:	02a353b3          	divu	t2,t1,a0
     1e0:	4491                	c.li	s1,4
     1e2:	fff38413          	addi	s0,t2,-1
     1e6:	00947363          	bgeu	s0,s1,1ec <uart0_init+0x2e>
     1ea:	4411                	c.li	s0,4
     1ec:	6509                	c.lui	a0,0x2
     1ee:	00a46463          	bltu	s0,a0,1f6 <uart0_init+0x38>
     1f2:	fff50413          	addi	s0,a0,-1 # 1fff <_data_lmastart+0x47f>
     1f6:	440104b7          	lui	s1,0x44010
     1fa:	588c                	c.lw	a1,48(s1)
     1fc:	4501                	c.li	a0,0
     1fe:	0045e613          	ori	a2,a1,4
     202:	d890                	c.sw	a2,48(s1)
     204:	3735                	c.jal	130 <GPIO_UART_function_enable>
     206:	670d                	c.lui	a4,0x3
     208:	448206b7          	lui	a3,0x44820
     20c:	04070813          	addi	a6,a4,64 # 3040 <SRAM2_SIZE+0x938>
     210:	0106a223          	sw	a6,4(a3) # 44820004 <_stack+0x147e0004>
     214:	04200893          	li	a7,66
     218:	0116a823          	sw	a7,16(a3)
     21c:	0006ac23          	sw	zero,24(a3)
     220:	00841e13          	slli	t3,s0,0x8
     224:	0006ae23          	sw	zero,28(a3)
     228:	01be6413          	ori	s0,t3,27
     22c:	c280                	c.sw	s0,0(a3)
     22e:	8001a623          	sw	zero,-2036(gp) # 30010104 <uart0_rx_done>
     232:	30010617          	auipc	a2,0x30010
     236:	ec062723          	sw	zero,-306(a2) # 30010100 <uart0_rx_index>
     23a:	08048493          	addi	s1,s1,128 # 44010080 <_stack+0x13fd0080>
     23e:	0004ae83          	lw	t4,0(s1)
     242:	010eef13          	ori	t5,t4,16
     246:	01e4a023          	sw	t5,0(s1)
     24a:	8c10                	exec.it	#7     !j	1762 <__riscv_restore_0>

0000024c <uart0_disable>:
     24c:	8c00                	exec.it	#6     !lui	a5,0x44820
     24e:	4398                	c.lw	a4,0(a5)
     250:	76f5                	c.lui	a3,0xffffd
     252:	fe477293          	andi	t0,a4,-28
     256:	8800                	exec.it	#4     !sw	t0,0(a5) # 44820000 <_stack+0x147e0000>
     258:	0047a303          	lw	t1,4(a5)
     25c:	fbf68393          	addi	t2,a3,-65 # ffffcfbf <_stack+0xcffbcfbf>
     260:	00737533          	and	a0,t1,t2
     264:	c3c8                	c.sw	a0,4(a5)
     266:	4b8c                	c.lw	a1,16(a5)
     268:	44010837          	lui	a6,0x44010
     26c:	08080893          	addi	a7,a6,128 # 44010080 <_stack+0x13fd0080>
     270:	fbd5f613          	andi	a2,a1,-67
     274:	cb90                	c.sw	a2,16(a5)
     276:	8410                	exec.it	#3     !lw	t3,0(a7)
     278:	fefe7e93          	andi	t4,t3,-17
     27c:	01d8a023          	sw	t4,0(a7)
     280:	03082f03          	lw	t5,48(a6)
     284:	ffbf7f93          	andi	t6,t5,-5
     288:	03f82823          	sw	t6,48(a6)
     28c:	8082                	c.jr	ra

0000028e <UART0_InterruptHandler>:
     28e:	8004                	exec.it	#8     !jal	t0,1724 <__riscv_save_4>
     290:	8c00                	exec.it	#6     !lui	a5,0x44820
     292:	4bc0                	c.lw	s0,20(a5)
     294:	8830                	exec.it	#21     !andi	t0,s0,66
     296:	02028663          	beqz	t0,2c2 <UART0_InterruptHandler+0x34>
     29a:	30010317          	auipc	t1,0x30010
     29e:	e6232303          	lw	t1,-414(t1) # 300100fc <boot_downloading>
     2a2:	06136e5b          	bnec	t1,1,31e <UART0_InterruptHandler+0x90>
     2a6:	30010497          	auipc	s1,0x30010
     2aa:	e5248493          	addi	s1,s1,-430 # 300100f8 <__fini_array_end>
     2ae:	4098                	c.lw	a4,0(s1)
     2b0:	06e36763          	bltu	t1,a4,31e <UART0_InterruptHandler+0x90>
     2b4:	44820937          	lui	s2,0x44820
     2b8:	4985                	c.li	s3,1
     2ba:	00892783          	lw	a5,8(s2) # 44820008 <_stack+0x147e0008>
     2be:	4157f75b          	bbs	a5,21,2cc <UART0_InterruptHandler+0x3e>
     2c2:	448202b7          	lui	t0,0x44820
     2c6:	0082aa23          	sw	s0,20(t0) # 44820014 <_stack+0x147e0014>
     2ca:	8400                	exec.it	#2     !j	1758 <__riscv_restore_4>
     2cc:	8014                	exec.it	#9     !lw	ra,0(s1)
     2ce:	00009363          	bnez	ra,2d4 <UART0_InterruptHandler+0x46>
     2d2:	8804                	exec.it	#12     !sw	s3,0(s1)
     2d4:	00c92303          	lw	t1,12(s2)
     2d8:	3c83255b          	bfoz	a0,t1,15,8
     2dc:	8420                	exec.it	#18     !jal	ra,d4a <uart_download_rx>
     2de:	bff1                	c.j	2ba <UART0_InterruptHandler+0x2c>
     2e0:	00c62883          	lw	a7,12(a2)
     2e4:	0003ae03          	lw	t3,0(t2)
     2e8:	0088df13          	srli	t5,a7,0x8
     2ec:	001e0813          	addi	a6,t3,1
     2f0:	01c58eb3          	add	t4,a1,t3
     2f4:	0103a023          	sw	a6,0(t2)
     2f8:	0003af83          	lw	t6,0(t2)
     2fc:	01ee8023          	sb	t5,0(t4)
     300:	00af9663          	bne	t6,a0,30c <UART0_InterruptHandler+0x7e>
     304:	30010697          	auipc	a3,0x30010
     308:	de06ae23          	sw	zero,-516(a3) # 30010100 <uart0_rx_index>
     30c:	4614                	c.lw	a3,8(a2)
     30e:	fd56f95b          	bbs	a3,21,2e0 <UART0_InterruptHandler+0x52>
     312:	ba64785b          	bbc	s0,6,2c2 <UART0_InterruptHandler+0x34>
     316:	4485                	c.li	s1,1
     318:	8091a623          	sw	s1,-2036(gp) # 30010104 <uart0_rx_done>
     31c:	b75d                	c.j	2c2 <UART0_InterruptHandler+0x34>
     31e:	44820637          	lui	a2,0x44820
     322:	30010397          	auipc	t2,0x30010
     326:	dde38393          	addi	t2,t2,-546 # 30010100 <uart0_rx_index>
     32a:	30010597          	auipc	a1,0x30010
     32e:	e3658593          	addi	a1,a1,-458 # 30010160 <uart0_rx_buf>
     332:	08000513          	li	a0,128
     336:	bfd9                	c.j	30c <UART0_InterruptHandler+0x7e>

00000338 <uart0_send>:
     338:	95aa                	c.add	a1,a0
     33a:	8c00                	exec.it	#6     !lui	a5,0x44820
     33c:	00b51363          	bne	a0,a1,342 <uart0_send+0xa>
     340:	8082                	c.jr	ra
     342:	00054703          	lbu	a4,0(a0)
     346:	0505                	c.addi	a0,1
     348:	4794                	c.lw	a3,8(a5)
     34a:	bf46ff5b          	bbc	a3,20,348 <uart0_send+0x10>
     34e:	c7d8                	c.sw	a4,12(a5)
     350:	b7f5                	c.j	33c <uart0_send+0x4>

00000352 <uart1_send_byte>:
     352:	8010                	exec.it	#1     !lui	a5,0x45830
     354:	4798                	c.lw	a4,8(a5)
     356:	bf477f5b          	bbc	a4,20,354 <uart1_send_byte+0x2>
     35a:	c7c8                	c.sw	a0,12(a5)
     35c:	8082                	c.jr	ra

0000035e <uart1_init>:
     35e:	8810                	exec.it	#5     !jal	t0,173e <__riscv_save_0>
     360:	8010                	exec.it	#1     !lui	a5,0x45830
     362:	0007a023          	sw	zero,0(a5) # 45830000 <_stack+0x157f0000>
     366:	0e100413          	li	s0,225
     36a:	c515                	c.beqz	a0,396 <uart1_init+0x38>
     36c:	018cc0b7          	lui	ra,0x18cc
     370:	00155413          	srli	s0,a0,0x1
     374:	a7f08293          	addi	t0,ra,-1409 # 18cba7f <SRAM2_SIZE+0x18c9377>
     378:	00540333          	add	t1,s0,t0
     37c:	02a353b3          	divu	t2,t1,a0
     380:	4491                	c.li	s1,4
     382:	fff38413          	addi	s0,t2,-1
     386:	00947363          	bgeu	s0,s1,38c <uart1_init+0x2e>
     38a:	4411                	c.li	s0,4
     38c:	6509                	c.lui	a0,0x2
     38e:	00a46463          	bltu	s0,a0,396 <uart1_init+0x38>
     392:	fff50413          	addi	s0,a0,-1 # 1fff <_data_lmastart+0x47f>
     396:	440104b7          	lui	s1,0x44010
     39a:	588c                	c.lw	a1,48(s1)
     39c:	4505                	c.li	a0,1
     39e:	4005e613          	ori	a2,a1,1024
     3a2:	d890                	c.sw	a2,48(s1)
     3a4:	3371                	c.jal	130 <GPIO_UART_function_enable>
     3a6:	670d                	c.lui	a4,0x3
     3a8:	458306b7          	lui	a3,0x45830
     3ac:	04070813          	addi	a6,a4,64 # 3040 <SRAM2_SIZE+0x938>
     3b0:	0106a223          	sw	a6,4(a3) # 45830004 <_stack+0x157f0004>
     3b4:	04200893          	li	a7,66
     3b8:	0116a823          	sw	a7,16(a3)
     3bc:	0006ac23          	sw	zero,24(a3)
     3c0:	00841e13          	slli	t3,s0,0x8
     3c4:	0006ae23          	sw	zero,28(a3)
     3c8:	01be6413          	ori	s0,t3,27
     3cc:	c280                	c.sw	s0,0(a3)
     3ce:	8001aa23          	sw	zero,-2028(gp) # 3001010c <uart1_rx_done>
     3d2:	8001a823          	sw	zero,-2032(gp) # 30010108 <uart1_rx_index>
     3d6:	08048493          	addi	s1,s1,128 # 44010080 <_stack+0x13fd0080>
     3da:	0004ae83          	lw	t4,0(s1)
     3de:	6f21                	c.lui	t5,0x8
     3e0:	01eeefb3          	or	t6,t4,t5
     3e4:	01f4a023          	sw	t6,0(s1)
     3e8:	8c10                	exec.it	#7     !j	1762 <__riscv_restore_0>

000003ea <uart1_disable>:
     3ea:	8010                	exec.it	#1     !lui	a5,0x45830
     3ec:	4398                	c.lw	a4,0(a5)
     3ee:	76f5                	c.lui	a3,0xffffd
     3f0:	fe477293          	andi	t0,a4,-28
     3f4:	8800                	exec.it	#4     !sw	t0,0(a5) # 45830000 <_stack+0x157f0000>
     3f6:	0047a303          	lw	t1,4(a5)
     3fa:	fbf68393          	addi	t2,a3,-65 # ffffcfbf <_stack+0xcffbcfbf>
     3fe:	00737533          	and	a0,t1,t2
     402:	c3c8                	c.sw	a0,4(a5)
     404:	4b8c                	c.lw	a1,16(a5)
     406:	44010837          	lui	a6,0x44010
     40a:	08080893          	addi	a7,a6,128 # 44010080 <_stack+0x13fd0080>
     40e:	fbd5f613          	andi	a2,a1,-67
     412:	cb90                	c.sw	a2,16(a5)
     414:	7ee1                	c.lui	t4,0xffff8
     416:	8410                	exec.it	#3     !lw	t3,0(a7)
     418:	fffe8f13          	addi	t5,t4,-1 # ffff7fff <_stack+0xcffb7fff>
     41c:	01ee7fb3          	and	t6,t3,t5
     420:	01f8a023          	sw	t6,0(a7)
     424:	03082783          	lw	a5,48(a6)
     428:	bff7f713          	andi	a4,a5,-1025
     42c:	02e82823          	sw	a4,48(a6)
     430:	8082                	c.jr	ra

00000432 <UART1_InterruptHandler>:
     432:	8004                	exec.it	#8     !jal	t0,1724 <__riscv_save_4>
     434:	8010                	exec.it	#1     !lui	a5,0x45830
     436:	4bc0                	c.lw	s0,20(a5)
     438:	8830                	exec.it	#21     !andi	t0,s0,66
     43a:	02028863          	beqz	t0,46a <UART1_InterruptHandler+0x38>
     43e:	30010317          	auipc	t1,0x30010
     442:	cbe32303          	lw	t1,-834(t1) # 300100fc <boot_downloading>
     446:	06136d5b          	bnec	t1,1,4c0 <UART1_InterruptHandler+0x8e>
     44a:	30010497          	auipc	s1,0x30010
     44e:	cae48493          	addi	s1,s1,-850 # 300100f8 <__fini_array_end>
     452:	0004a383          	lw	t2,0(s1)
     456:	ffd3f513          	andi	a0,t2,-3
     45a:	e13d                	c.bnez	a0,4c0 <UART1_InterruptHandler+0x8e>
     45c:	45830937          	lui	s2,0x45830
     460:	4989                	c.li	s3,2
     462:	00892783          	lw	a5,8(s2) # 45830008 <_stack+0x157f0008>
     466:	4157f75b          	bbs	a5,21,474 <UART1_InterruptHandler+0x42>
     46a:	458302b7          	lui	t0,0x45830
     46e:	0082aa23          	sw	s0,20(t0) # 45830014 <_stack+0x157f0014>
     472:	8400                	exec.it	#2     !j	1758 <__riscv_restore_4>
     474:	8014                	exec.it	#9     !lw	ra,0(s1)
     476:	00009363          	bnez	ra,47c <UART1_InterruptHandler+0x4a>
     47a:	8804                	exec.it	#12     !sw	s3,0(s1)
     47c:	00c92303          	lw	t1,12(s2)
     480:	0ff37513          	andi	a0,t1,255
     484:	8420                	exec.it	#18     !jal	ra,d4a <uart_download_rx>
     486:	bff1                	c.j	462 <UART1_InterruptHandler+0x30>
     488:	00c6a883          	lw	a7,12(a3)
     48c:	00072e03          	lw	t3,0(a4)
     490:	001e0e93          	addi	t4,t3,1
     494:	01c60f33          	add	t5,a2,t3
     498:	01d72023          	sw	t4,0(a4)
     49c:	00072f83          	lw	t6,0(a4)
     4a0:	011f0023          	sb	a7,0(t5) # 8000 <SRAM2_SIZE+0x58f8>
     4a4:	00bf9463          	bne	t6,a1,4ac <UART1_InterruptHandler+0x7a>
     4a8:	8001a823          	sw	zero,-2032(gp) # 30010108 <uart1_rx_index>
     4ac:	0086a803          	lw	a6,8(a3)
     4b0:	fd587c5b          	bbs	a6,21,488 <UART1_InterruptHandler+0x56>
     4b4:	ba647b5b          	bbc	s0,6,46a <UART1_InterruptHandler+0x38>
     4b8:	4485                	c.li	s1,1
     4ba:	8091aa23          	sw	s1,-2028(gp) # 3001010c <uart1_rx_done>
     4be:	b775                	c.j	46a <UART1_InterruptHandler+0x38>
     4c0:	458306b7          	lui	a3,0x45830
     4c4:	81018713          	addi	a4,gp,-2032 # 30010108 <uart1_rx_index>
     4c8:	8e818613          	addi	a2,gp,-1816 # 300101e0 <uart1_rx_buf>
     4cc:	08000593          	li	a1,128
     4d0:	bff1                	c.j	4ac <UART1_InterruptHandler+0x7a>

000004d2 <uart1_send>:
     4d2:	8810                	exec.it	#5     !jal	t0,173e <__riscv_save_0>
     4d4:	842a                	c.mv	s0,a0
     4d6:	00b504b3          	add	s1,a0,a1
     4da:	00941363          	bne	s0,s1,4e0 <uart1_send+0xe>
     4de:	8c10                	exec.it	#7     !j	1762 <__riscv_restore_0>
     4e0:	8414                	exec.it	#11     !lbu	a0,0(s0) # 18000 <SRAM2_SIZE+0x158f8>
     4e2:	0405                	c.addi	s0,1
     4e4:	35bd                	c.jal	352 <uart1_send_byte>
     4e6:	bfd5                	c.j	4da <uart1_send+0x8>

000004e8 <uart1_send_string>:
     4e8:	256012ef          	jal	t0,173e <__riscv_save_0>
     4ec:	30010797          	auipc	a5,0x30010
     4f0:	c0c7a783          	lw	a5,-1012(a5) # 300100f8 <__fini_array_end>
     4f4:	842a                	c.mv	s0,a0
     4f6:	0027ec5b          	bnec	a5,2,50e <uart1_send_string+0x26>
     4fa:	30010297          	auipc	t0,0x30010
     4fe:	c022a283          	lw	t0,-1022(t0) # 300100fc <boot_downloading>
     502:	0012e65b          	bnec	t0,1,50e <uart1_send_string+0x26>
     506:	25c0106f          	j	1762 <__riscv_restore_0>
     50a:	0405                	c.addi	s0,1
     50c:	3599                	c.jal	352 <uart1_send_byte>
     50e:	8414                	exec.it	#11     !lbu	a0,0(s0)
     510:	fd6d                	c.bnez	a0,50a <uart1_send_string+0x22>
     512:	bfd5                	c.j	506 <uart1_send_string+0x1e>

00000514 <uart1_wait_tx_finish>:
     514:	45830737          	lui	a4,0x45830
     518:	471c                	c.lw	a5,8(a4)
     51a:	bf17ff5b          	bbc	a5,17,518 <uart1_wait_tx_finish+0x4>
     51e:	8082                	c.jr	ra

00000520 <UART2_InterruptHandler>:
     520:	8004                	exec.it	#8     !jal	t0,1724 <__riscv_save_4>
     522:	458407b7          	lui	a5,0x45840
     526:	4bc0                	c.lw	s0,20(a5)
     528:	8830                	exec.it	#21     !andi	t0,s0,66
     52a:	04028363          	beqz	t0,570 <UART2_InterruptHandler+0x50>
     52e:	30010317          	auipc	t1,0x30010
     532:	bce32303          	lw	t1,-1074(t1) # 300100fc <boot_downloading>
     536:	06136c5b          	bnec	t1,1,5ae <UART2_InterruptHandler+0x8e>
     53a:	30010497          	auipc	s1,0x30010
     53e:	bbe48493          	addi	s1,s1,-1090 # 300100f8 <__fini_array_end>
     542:	0004a383          	lw	t2,0(s1)
     546:	00038463          	beqz	t2,54e <UART2_InterruptHandler+0x2e>
     54a:	0663e25b          	bnec	t2,6,5ae <UART2_InterruptHandler+0x8e>
     54e:	45840937          	lui	s2,0x45840
     552:	4999                	c.li	s3,6
     554:	a811                	c.j	568 <UART2_InterruptHandler+0x48>
     556:	8014                	exec.it	#9     !lw	ra,0(s1)
     558:	00009363          	bnez	ra,55e <UART2_InterruptHandler+0x3e>
     55c:	8804                	exec.it	#12     !sw	s3,0(s1)
     55e:	00c92283          	lw	t0,12(s2) # 4584000c <_stack+0x1580000c>
     562:	0ff2f513          	andi	a0,t0,255
     566:	8420                	exec.it	#18     !jal	ra,d4a <uart_download_rx>
     568:	00892f83          	lw	t6,8(s2)
     56c:	ff5ff55b          	bbs	t6,21,556 <UART2_InterruptHandler+0x36>
     570:	458407b7          	lui	a5,0x45840
     574:	cbc0                	c.sw	s0,20(a5)
     576:	8400                	exec.it	#2     !j	1758 <__riscv_restore_4>
     578:	00c6a883          	lw	a7,12(a3) # 4583000c <_stack+0x157f000c>
     57c:	00072e03          	lw	t3,0(a4) # 45830000 <_stack+0x157f0000>
     580:	001e0813          	addi	a6,t3,1
     584:	01c60eb3          	add	t4,a2,t3
     588:	01072023          	sw	a6,0(a4)
     58c:	00072f03          	lw	t5,0(a4)
     590:	011e8023          	sb	a7,0(t4)
     594:	00bf1463          	bne	t5,a1,59c <UART2_InterruptHandler+0x7c>
     598:	8001ac23          	sw	zero,-2024(gp) # 30010110 <uart2_rx_index>
     59c:	4688                	c.lw	a0,8(a3)
     59e:	fd557d5b          	bbs	a0,21,578 <UART2_InterruptHandler+0x58>
     5a2:	bc64775b          	bbc	s0,6,570 <UART2_InterruptHandler+0x50>
     5a6:	4485                	c.li	s1,1
     5a8:	8091ae23          	sw	s1,-2020(gp) # 30010114 <uart2_rx_done>
     5ac:	b7d1                	c.j	570 <UART2_InterruptHandler+0x50>
     5ae:	458406b7          	lui	a3,0x45840
     5b2:	81818713          	addi	a4,gp,-2024 # 30010110 <uart2_rx_index>
     5b6:	96818613          	addi	a2,gp,-1688 # 30010260 <uart2_rx_buf>
     5ba:	08000593          	li	a1,128
     5be:	bff9                	c.j	59c <UART2_InterruptHandler+0x7c>

000005c0 <uart_init>:
     5c0:	17e012ef          	jal	t0,173e <__riscv_save_0>
     5c4:	6471                	c.lui	s0,0x1c
     5c6:	20040513          	addi	a0,s0,512 # 1c200 <SRAM2_SIZE+0x19af8>
     5ca:	3ed5                	c.jal	1be <uart0_init>
     5cc:	20040513          	addi	a0,s0,512
     5d0:	3379                	c.jal	35e <uart1_init>
     5d2:	4501                	c.li	a0,0
     5d4:	18e0106f          	j	1762 <__riscv_restore_0>

000005d8 <uart_deinit>:
     5d8:	166012ef          	jal	t0,173e <__riscv_save_0>
     5dc:	3985                	c.jal	24c <uart0_disable>
     5de:	3531                	c.jal	3ea <uart1_disable>
     5e0:	1820106f          	j	1762 <__riscv_restore_0>

000005e4 <bk_printf>:
     5e4:	7179                	c.addi16sp	sp,-48
     5e6:	c606                	c.swsp	ra,12(sp)
     5e8:	ca2e                	c.swsp	a1,20(sp)
     5ea:	cc32                	c.swsp	a2,24(sp)
     5ec:	ce36                	c.swsp	a3,28(sp)
     5ee:	d03a                	c.swsp	a4,32(sp)
     5f0:	d23e                	c.swsp	a5,36(sp)
     5f2:	d442                	c.swsp	a6,40(sp)
     5f4:	d646                	c.swsp	a7,44(sp)
     5f6:	3dcd                	c.jal	4e8 <uart1_send_string>
     5f8:	40b2                	c.lwsp	ra,12(sp)
     5fa:	6145                	c.addi16sp	sp,48
     5fc:	8082                	c.jr	ra

000005fe <hex2Str>:
     5fe:	140012ef          	jal	t0,173e <__riscv_save_0>
     602:	1101                	c.addi	sp,-32
     604:	842a                	c.mv	s0,a0
     606:	4645                	c.li	a2,17
     608:	00001597          	auipc	a1,0x1
     60c:	34858593          	addi	a1,a1,840 # 1950 <_ITB_BASE_+0x5c>
     610:	0068                	c.addi4spn	a0,sp,12
     612:	15e010ef          	jal	ra,1770 <memcpy>
     616:	1014                	c.addi4spn	a3,sp,32
     618:	00445793          	srli	a5,s0,0x4
     61c:	00f47393          	andi	t2,s0,15
     620:	00768433          	add	s0,a3,t2
     624:	00f682b3          	add	t0,a3,a5
     628:	fec28303          	lb	t1,-20(t0)
     62c:	fec40603          	lb	a2,-20(s0)
     630:	82018513          	addi	a0,gp,-2016 # 30010118 <str.0>
     634:	00650023          	sb	t1,0(a0)
     638:	00c500a3          	sb	a2,1(a0)
     63c:	00050123          	sb	zero,2(a0)
     640:	6105                	c.addi16sp	sp,32
     642:	1200106f          	j	1762 <__riscv_restore_0>

00000646 <bk_print_hex>:
     646:	0f8012ef          	jal	t0,173e <__riscv_save_0>
     64a:	84aa                	c.mv	s1,a0
     64c:	4461                	c.li	s0,24
     64e:	00001517          	auipc	a0,0x1
     652:	31650513          	addi	a0,a0,790 # 1964 <_ITB_BASE_+0x70>
     656:	5961                	c.li	s2,-8
     658:	3d41                	c.jal	4e8 <uart1_send_string>
     65a:	0084d533          	srl	a0,s1,s0
     65e:	0ff57513          	andi	a0,a0,255
     662:	1461                	c.addi	s0,-8
     664:	3f69                	c.jal	5fe <hex2Str>
     666:	3549                	c.jal	4e8 <uart1_send_string>
     668:	ff2419e3          	bne	s0,s2,65a <bk_print_hex+0x14>
     66c:	0f60106f          	j	1762 <__riscv_restore_0>

00000670 <flash_get_current_flash_config>:
     670:	00001717          	auipc	a4,0x1
     674:	2f870713          	addi	a4,a4,760 # 1968 <flash_config>
     678:	82c1a603          	lw	a2,-2004(gp) # 30010124 <flash_id>
     67c:	4781                	c.li	a5,0
     67e:	86ba                	c.mv	a3,a4
     680:	430c                	c.lw	a1,0(a4)
     682:	00c59a63          	bne	a1,a2,696 <flash_get_current_flash_config+0x26>
     686:	4331                	c.li	t1,12
     688:	026783b3          	mul	t2,a5,t1
     68c:	007682b3          	add	t0,a3,t2
     690:	8251a223          	sw	t0,-2012(gp) # 3001011c <flash_current_config>
     694:	8082                	c.jr	ra
     696:	0785                	c.addi	a5,1
     698:	0731                	c.addi	a4,12
     69a:	bf07e35b          	bnec	a5,16,680 <flash_get_current_flash_config+0x10>
     69e:	00001297          	auipc	t0,0x1
     6a2:	38a28293          	addi	t0,t0,906 # 1a28 <flash_config+0xc0>
     6a6:	b7ed                	c.j	690 <flash_get_current_flash_config+0x20>

000006a8 <get_flash_ID>:
     6a8:	8820                	exec.it	#20     !lui	a5,0x44030
     6aa:	4398                	c.lw	a4,0(a5)
     6ac:	fe074fe3          	bltz	a4,6aa <get_flash_ID+0x2>
     6b0:	740002b7          	lui	t0,0x74000
     6b4:	44030337          	lui	t1,0x44030
     6b8:	8800                	exec.it	#4     !sw	t0,0(a5) # 44030000 <_stack+0x13ff0000>
     6ba:	8030                	exec.it	#17     !lw	t2,0(t1) # 44030000 <_stack+0x13ff0000>
     6bc:	fe03cfe3          	bltz	t2,6ba <get_flash_ID+0x12>
     6c0:	01032503          	lw	a0,16(t1)
     6c4:	82a1a623          	sw	a0,-2004(gp) # 30010124 <flash_id>
     6c8:	0ff57593          	andi	a1,a0,255
     6cc:	fff58613          	addi	a2,a1,-1
     6d0:	4689                	c.li	a3,2
     6d2:	00c69833          	sll	a6,a3,a2
     6d6:	8301a423          	sw	a6,-2008(gp) # 30010120 <flash_size>
     6da:	8082                	c.jr	ra

000006dc <flash_read_sr>:
     6dc:	86aa                	c.mv	a3,a0
     6de:	8820                	exec.it	#20     !lui	a5,0x44030
     6e0:	4398                	c.lw	a4,0(a5)
     6e2:	fe074fe3          	bltz	a4,6e0 <flash_read_sr+0x4>
     6e6:	630002b7          	lui	t0,0x63000
     6ea:	44030337          	lui	t1,0x44030
     6ee:	8800                	exec.it	#4     !sw	t0,0(a5) # 44030000 <_stack+0x13ff0000>
     6f0:	8030                	exec.it	#17     !lw	t2,0(t1) # 44030000 <_stack+0x13ff0000>
     6f2:	fe03cfe3          	bltz	t2,6f0 <flash_read_sr+0x14>
     6f6:	01432503          	lw	a0,20(t1)
     6fa:	0ff57513          	andi	a0,a0,255
     6fe:	0226e25b          	bnec	a3,2,722 <flash_read_sr+0x46>
     702:	660005b7          	lui	a1,0x66000
     706:	44030637          	lui	a2,0x44030
     70a:	00b32023          	sw	a1,0(t1)
     70e:	00062803          	lw	a6,0(a2) # 44030000 <_stack+0x13ff0000>
     712:	fe084ee3          	bltz	a6,70e <flash_read_sr+0x32>
     716:	01462883          	lw	a7,20(a2)
     71a:	20f8ae5b          	bfoz	t3,a7,8,15
     71e:	00ae6533          	or	a0,t3,a0
     722:	8082                	c.jr	ra

00000724 <flash_write_sr>:
     724:	8820                	exec.it	#20     !lui	a5,0x44030
     726:	4398                	c.lw	a4,0(a5)
     728:	fe074fe3          	bltz	a4,726 <flash_write_sr+0x2>
     72c:	01c7a283          	lw	t0,28(a5) # 4403001c <_stack+0x13ff001c>
     730:	fc0006b7          	lui	a3,0xfc000
     734:	3ff68313          	addi	t1,a3,1023 # fc0003ff <_stack+0xcbfc03ff>
     738:	0062f3b3          	and	t2,t0,t1
     73c:	05aa                	c.slli	a1,0xa
     73e:	0075e633          	or	a2,a1,t2
     742:	44030837          	lui	a6,0x44030
     746:	cfd0                	c.sw	a2,28(a5)
     748:	00082883          	lw	a7,0(a6) # 44030000 <_stack+0x13ff0000>
     74c:	fe08cee3          	bltz	a7,748 <flash_write_sr+0x24>
     750:	64000e37          	lui	t3,0x64000
     754:	0015565b          	beqc	a0,1,760 <flash_write_sr+0x3c>
     758:	0025665b          	bnec	a0,2,764 <flash_write_sr+0x40>
     75c:	67000e37          	lui	t3,0x67000
     760:	01c82023          	sw	t3,0(a6)
     764:	44030537          	lui	a0,0x44030
     768:	00052e83          	lw	t4,0(a0) # 44030000 <_stack+0x13ff0000>
     76c:	fe0ecee3          	bltz	t4,768 <flash_write_sr+0x44>
     770:	8082                	c.jr	ra

00000772 <clr_flash_protect>:
     772:	7cd002ef          	jal	t0,173e <__riscv_save_0>
     776:	4509                	c.li	a0,2
     778:	3795                	c.jal	6dc <flash_read_sr>
     77a:	c519                	c.beqz	a0,788 <clr_flash_protect+0x16>
     77c:	8241a783          	lw	a5,-2012(gp) # 3001011c <flash_current_config>
     780:	0047c503          	lbu	a0,4(a5)
     784:	4581                	c.li	a1,0
     786:	3f79                	c.jal	724 <flash_write_sr>
     788:	7db0006f          	j	1762 <__riscv_restore_0>

0000078c <set_flash_protect>:
     78c:	8810                	exec.it	#5     !jal	t0,173e <__riscv_save_0>
     78e:	82418493          	addi	s1,gp,-2012 # 3001011c <flash_current_config>
     792:	409c                	c.lw	a5,0(s1)
     794:	e105                	c.bnez	a0,7b4 <set_flash_protect+0x28>
     796:	0087d403          	lhu	s0,8(a5)
     79a:	0047c503          	lbu	a0,4(a5)
     79e:	040a                	c.slli	s0,0x2
     7a0:	3f35                	c.jal	6dc <flash_read_sr>
     7a2:	00850863          	beq	a0,s0,7b2 <set_flash_protect+0x26>
     7a6:	8014                	exec.it	#9     !lw	ra,0(s1)
     7a8:	3c0425db          	bfoz	a1,s0,15,0
     7ac:	0040c503          	lbu	a0,4(ra)
     7b0:	3f95                	c.jal	724 <flash_write_sr>
     7b2:	8c10                	exec.it	#7     !j	1762 <__riscv_restore_0>
     7b4:	0067d403          	lhu	s0,6(a5)
     7b8:	b7cd                	c.j	79a <set_flash_protect+0xe>

000007ba <clr_flash_qwfr>:
     7ba:	8000                	exec.it	#0     !lui	a4,0x44030
     7bc:	4f5c                	c.lw	a5,28(a4)
     7be:	8c14                	exec.it	#15     !lui	a3,0x40000
     7c0:	e0f7f293          	andi	t0,a5,-497
     7c4:	00572e23          	sw	t0,28(a4) # 4403001c <_stack+0x13ff001c>
     7c8:	00072303          	lw	t1,0(a4)
     7cc:	36000537          	lui	a0,0x36000
     7d0:	8c04                	exec.it	#14     !and	t2,t1,a3
     7d2:	00a3e5b3          	or	a1,t2,a0
     7d6:	c30c                	c.sw	a1,0(a4)
     7d8:	4310                	c.lw	a2,0(a4)
     7da:	fe064fe3          	bltz	a2,7d8 <clr_flash_qwfr+0x1e>
     7de:	8082                	c.jr	ra

000007e0 <flash_set_line_mode>:
     7e0:	0015635b          	bnec	a0,1,7e6 <flash_set_line_mode+0x6>
     7e4:	bfd9                	c.j	7ba <clr_flash_qwfr>
     7e6:	0225615b          	bnec	a0,2,808 <flash_set_line_mode+0x28>
     7ea:	440303b7          	lui	t2,0x44030
     7ee:	01c3a503          	lw	a0,28(t2) # 4403001c <_stack+0x13ff001c>
     7f2:	e0f57593          	andi	a1,a0,-497
     7f6:	0105e613          	ori	a2,a1,16
     7fa:	00c3ae23          	sw	a2,28(t2)
     7fe:	0003a683          	lw	a3,0(t2)
     802:	fe06cee3          	bltz	a3,7fe <flash_set_line_mode+0x1e>
     806:	8082                	c.jr	ra
     808:	00456b5b          	bnec	a0,4,81e <flash_set_line_mode+0x3e>
     80c:	8000                	exec.it	#0     !lui	a4,0x44030
     80e:	4f5c                	c.lw	a5,28(a4)
     810:	e0f7f293          	andi	t0,a5,-497
     814:	0a02e313          	ori	t1,t0,160
     818:	00672e23          	sw	t1,28(a4) # 4403001c <_stack+0x13ff001c>
     81c:	8082                	c.jr	ra
     81e:	8082                	c.jr	ra

00000820 <flash_set_clk>:
     820:	8000                	exec.it	#0     !lui	a4,0x44030
     822:	431c                	c.lw	a5,0(a4)
     824:	fe07cfe3          	bltz	a5,822 <flash_set_clk+0x2>
     828:	01c72283          	lw	t0,28(a4) # 4403001c <_stack+0x13ff001c>
     82c:	893d                	c.andi	a0,15
     82e:	ff02f313          	andi	t1,t0,-16
     832:	00a363b3          	or	t2,t1,a0
     836:	440305b7          	lui	a1,0x44030
     83a:	00772e23          	sw	t2,28(a4)
     83e:	4190                	c.lw	a2,0(a1)
     840:	fe064fe3          	bltz	a2,83e <flash_set_clk+0x1e>
     844:	8082                	c.jr	ra

00000846 <flash_erase_sector>:
     846:	8281a783          	lw	a5,-2008(gp) # 30010120 <flash_size>
     84a:	02f57963          	bgeu	a0,a5,87c <flash_erase_sector+0x36>
     84e:	8814                	exec.it	#13     !lui	t0,0x44030
     850:	0002a703          	lw	a4,0(t0) # 44030000 <_stack+0x13ff0000>
     854:	fe074ee3          	bltz	a4,850 <flash_erase_sector+0xa>
     858:	8404                	exec.it	#10     !lw	t1,0(t0)
     85a:	8c14                	exec.it	#15     !lui	a3,0x40000
     85c:	5c05255b          	bfoz	a0,a0,23,0
     860:	8c04                	exec.it	#14     !and	t2,t1,a3
     862:	007565b3          	or	a1,a0,t2
     866:	2d000637          	lui	a2,0x2d000
     86a:	00c5e833          	or	a6,a1,a2
     86e:	440308b7          	lui	a7,0x44030
     872:	0102a023          	sw	a6,0(t0)
     876:	8410                	exec.it	#3     !lw	t3,0(a7) # 44030000 <_stack+0x13ff0000>
     878:	fe0e4fe3          	bltz	t3,876 <flash_erase_sector+0x30>
     87c:	8082                	c.jr	ra

0000087e <flash_erase_block>:
     87e:	8281a783          	lw	a5,-2008(gp) # 30010120 <flash_size>
     882:	02f57963          	bgeu	a0,a5,8b4 <flash_erase_block+0x36>
     886:	8814                	exec.it	#13     !lui	t0,0x44030
     888:	0002a703          	lw	a4,0(t0) # 44030000 <_stack+0x13ff0000>
     88c:	fe074ee3          	bltz	a4,888 <flash_erase_block+0xa>
     890:	8404                	exec.it	#10     !lw	t1,0(t0)
     892:	8c14                	exec.it	#15     !lui	a3,0x40000
     894:	5c05255b          	bfoz	a0,a0,23,0
     898:	8c04                	exec.it	#14     !and	t2,t1,a3
     89a:	007565b3          	or	a1,a0,t2
     89e:	2f000637          	lui	a2,0x2f000
     8a2:	00c5e833          	or	a6,a1,a2
     8a6:	440308b7          	lui	a7,0x44030
     8aa:	0102a023          	sw	a6,0(t0)
     8ae:	8410                	exec.it	#3     !lw	t3,0(a7) # 44030000 <_stack+0x13ff0000>
     8b0:	fe0e4fe3          	bltz	t3,8ae <flash_erase_block+0x30>
     8b4:	8082                	c.jr	ra

000008b6 <flash_erase>:
     8b6:	66f002ef          	jal	t0,1724 <__riscv_save_4>
     8ba:	00fff4b7          	lui	s1,0xfff
     8be:	67bd                	c.lui	a5,0xf
     8c0:	8ce9                	c.and	s1,a0
     8c2:	8d7d                	c.and	a0,a5
     8c4:	892e                	c.mv	s2,a1
     8c6:	e919                	c.bnez	a0,8dc <flash_erase+0x26>
     8c8:	842e                	c.mv	s0,a1
     8ca:	69c1                	c.lui	s3,0x10
     8cc:	0489e463          	bltu	s3,s0,914 <flash_erase+0x5e>
     8d0:	94a2                	c.add	s1,s0
     8d2:	6985                	c.lui	s3,0x1
     8d4:	e431                	c.bnez	s0,920 <flash_erase+0x6a>
     8d6:	854a                	c.mv	a0,s2
     8d8:	6810006f          	j	1758 <__riscv_restore_4>
     8dc:	62c1                	c.lui	t0,0x10
     8de:	fff28313          	addi	t1,t0,-1 # ffff <SRAM2_SIZE+0xd8f7>
     8e2:	006483b3          	add	t2,s1,t1
     8e6:	00ff0737          	lui	a4,0xff0
     8ea:	00e3f433          	and	s0,t2,a4
     8ee:	00b489b3          	add	s3,s1,a1
     8f2:	01347363          	bgeu	s0,s3,8f8 <flash_erase+0x42>
     8f6:	89a2                	c.mv	s3,s0
     8f8:	844a                	c.mv	s0,s2
     8fa:	6a05                	c.lui	s4,0x1
     8fc:	fd34f7e3          	bgeu	s1,s3,8ca <flash_erase+0x14>
     900:	8526                	c.mv	a0,s1
     902:	3791                	c.jal	846 <flash_erase_sector>
     904:	94d2                	c.add	s1,s4
     906:	01446563          	bltu	s0,s4,910 <flash_erase+0x5a>
     90a:	41440433          	sub	s0,s0,s4
     90e:	b7fd                	c.j	8fc <flash_erase+0x46>
     910:	4401                	c.li	s0,0
     912:	b7ed                	c.j	8fc <flash_erase+0x46>
     914:	8526                	c.mv	a0,s1
     916:	41340433          	sub	s0,s0,s3
     91a:	94ce                	c.add	s1,s3
     91c:	378d                	c.jal	87e <flash_erase_block>
     91e:	b77d                	c.j	8cc <flash_erase+0x16>
     920:	40848533          	sub	a0,s1,s0
     924:	370d                	c.jal	846 <flash_erase_sector>
     926:	fb3468e3          	bltu	s0,s3,8d6 <flash_erase+0x20>
     92a:	41340433          	sub	s0,s0,s3
     92e:	b75d                	c.j	8d4 <flash_erase+0x1e>

00000930 <flash_read_data>:
     930:	c651                	c.beqz	a2,9bc <flash_read_data+0x8c>
     932:	1101                	c.addi	sp,-32
     934:	8000                	exec.it	#0     !lui	a4,0x44030
     936:	431c                	c.lw	a5,0(a4)
     938:	fe07cfe3          	bltz	a5,936 <flash_read_data+0x6>
     93c:	01000337          	lui	t1,0x1000
     940:	fe05f893          	andi	a7,a1,-32
     944:	8814                	exec.it	#13     !lui	t0,0x44030
     946:	fff30393          	addi	t2,t1,-1 # ffffff <SRAM2_SIZE+0xffd8f7>
     94a:	40000e37          	lui	t3,0x40000
     94e:	25000eb7          	lui	t4,0x25000
     952:	0002a683          	lw	a3,0(t0) # 44030000 <_stack+0x13ff0000>
     956:	0078f833          	and	a6,a7,t2
     95a:	01c6ff33          	and	t5,a3,t3
     95e:	01e86fb3          	or	t6,a6,t5
     962:	01dfe733          	or	a4,t6,t4
     966:	00e2a023          	sw	a4,0(t0)
     96a:	0002a783          	lw	a5,0(t0)
     96e:	fe07cee3          	bltz	a5,96a <flash_read_data+0x3a>
     972:	880a                	c.mv	a6,sp
     974:	02088893          	addi	a7,a7,32
     978:	8342                	c.mv	t1,a6
     97a:	0082a683          	lw	a3,8(t0)
     97e:	0811                	c.addi	a6,4
     980:	fed82e23          	sw	a3,-4(a6)
     984:	02010f13          	addi	t5,sp,32
     988:	ff0f19e3          	bne	t5,a6,97a <flash_read_data+0x4a>
     98c:	01f5f693          	andi	a3,a1,31
     990:	8faa                	c.mv	t6,a0
     992:	00d30733          	add	a4,t1,a3
     996:	00070783          	lb	a5,0(a4) # 44030000 <_stack+0x13ff0000>
     99a:	0f85                	c.addi	t6,1
     99c:	00bf8833          	add	a6,t6,a1
     9a0:	167d                	c.addi	a2,-1
     9a2:	feff8fa3          	sb	a5,-1(t6)
     9a6:	40a80f33          	sub	t5,a6,a0
     9aa:	c619                	c.beqz	a2,9b8 <flash_read_data+0x88>
     9ac:	0685                	c.addi	a3,1
     9ae:	be06e2db          	bnec	a3,32,992 <flash_read_data+0x62>
     9b2:	85fa                	c.mv	a1,t5
     9b4:	857e                	c.mv	a0,t6
     9b6:	bf71                	c.j	952 <flash_read_data+0x22>
     9b8:	6105                	c.addi16sp	sp,32
     9ba:	8082                	c.jr	ra
     9bc:	8082                	c.jr	ra

000009be <flash_write_data>:
     9be:	559002ef          	jal	t0,1716 <__riscv_save_10>
     9c2:	01f5f793          	andi	a5,a1,31
     9c6:	1101                	c.addi	sp,-32
     9c8:	892a                	c.mv	s2,a0
     9ca:	842e                	c.mv	s0,a1
     9cc:	84b2                	c.mv	s1,a2
     9ce:	fe05fa13          	andi	s4,a1,-32
     9d2:	c791                	c.beqz	a5,9de <flash_write_data+0x20>
     9d4:	02000613          	li	a2,32
     9d8:	85d2                	c.mv	a1,s4
     9da:	850a                	c.mv	a0,sp
     9dc:	3f91                	c.jal	930 <flash_read_data>
     9de:	8000                	exec.it	#0     !lui	a4,0x44030
     9e0:	00072283          	lw	t0,0(a4) # 44030000 <_stack+0x13ff0000>
     9e4:	fe02cee3          	bltz	t0,9e0 <flash_write_data+0x22>
     9e8:	44030ab7          	lui	s5,0x44030
     9ec:	40000bb7          	lui	s7,0x40000
     9f0:	2c000c37          	lui	s8,0x2c000
     9f4:	e481                	c.bnez	s1,9fc <flash_write_data+0x3e>
     9f6:	6105                	c.addi16sp	sp,32
     9f8:	5570006f          	j	174e <__riscv_restore_10>
     9fc:	01f47313          	andi	t1,s0,31
     a00:	89ca                	c.mv	s3,s2
     a02:	00098683          	lb	a3,0(s3) # 1000 <boot_uart_data_callback+0x42>
     a06:	0985                	c.addi	s3,1
     a08:	006100b3          	add	ra,sp,t1
     a0c:	00898b33          	add	s6,s3,s0
     a10:	14fd                	c.addi	s1,-1
     a12:	00d08023          	sb	a3,0(ra)
     a16:	412b0b33          	sub	s6,s6,s2
     a1a:	c481                	c.beqz	s1,a22 <flash_write_data+0x64>
     a1c:	0305                	c.addi	t1,1
     a1e:	be0362db          	bnec	t1,32,a02 <flash_write_data+0x44>
     a22:	840a                	c.mv	s0,sp
     a24:	00042383          	lw	t2,0(s0)
     a28:	1008                	c.addi4spn	a0,sp,32
     a2a:	0411                	c.addi	s0,4
     a2c:	007aa223          	sw	t2,4(s5) # 44030004 <_stack+0x13ff0004>
     a30:	fea41ae3          	bne	s0,a0,a24 <flash_write_data+0x66>
     a34:	000aa583          	lw	a1,0(s5)
     a38:	0175f633          	and	a2,a1,s7
     a3c:	01466833          	or	a6,a2,s4
     a40:	018868b3          	or	a7,a6,s8
     a44:	011aa023          	sw	a7,0(s5)
     a48:	000aa903          	lw	s2,0(s5)
     a4c:	fe094ee3          	bltz	s2,a48 <flash_write_data+0x8a>
     a50:	02000613          	li	a2,32
     a54:	0ff00593          	li	a1,255
     a58:	850a                	c.mv	a0,sp
     a5a:	020a0a13          	addi	s4,s4,32 # 1020 <boot_uart_data_callback+0x62>
     a5e:	845a                	c.mv	s0,s6
     a60:	894e                	c.mv	s2,s3
     a62:	5a3000ef          	jal	ra,1804 <memset>
     a66:	b779                	c.j	9f4 <flash_write_data+0x36>

00000a68 <wdt_ctrl>:
     a68:	4d7002ef          	jal	t0,173e <__riscv_save_0>
     a6c:	0e3307b7          	lui	a5,0xe330
     a70:	00378713          	addi	a4,a5,3 # e330003 <SRAM2_SIZE+0xe32d8fb>
     a74:	04e50763          	beq	a0,a4,ac2 <wdt_ctrl+0x5a>
     a78:	00a76d63          	bltu	a4,a0,a92 <wdt_ctrl+0x2a>
     a7c:	00178f13          	addi	t5,a5,1
     a80:	03e50a63          	beq	a0,t5,ab4 <wdt_ctrl+0x4c>
     a84:	00278293          	addi	t0,a5,2
     a88:	06550d63          	beq	a0,t0,b02 <wdt_ctrl+0x9a>
     a8c:	4501                	c.li	a0,0
     a8e:	4d50006f          	j	1762 <__riscv_restore_0>
     a92:	00478893          	addi	a7,a5,4
     a96:	ff151be3          	bne	a0,a7,a8c <wdt_ctrl+0x24>
     a9a:	8201a823          	sw	zero,-2000(gp) # 30010128 <g_wdt_period>
     a9e:	44010fb7          	lui	t6,0x44010
     aa2:	030fae03          	lw	t3,48(t6) # 44010030 <_stack+0x13fd0030>
     aa6:	80000eb7          	lui	t4,0x80000
     aaa:	01de6733          	or	a4,t3,t4
     aae:	02efa823          	sw	a4,48(t6)
     ab2:	bfe9                	c.j	a8c <wdt_ctrl+0x24>
     ab4:	44010fb7          	lui	t6,0x44010
     ab8:	030fa783          	lw	a5,48(t6) # 44010030 <_stack+0x13fd0030>
     abc:	7807a75b          	bfoz	a4,a5,30,0
     ac0:	b7fd                	c.j	aae <wdt_ctrl+0x46>
     ac2:	83018913          	addi	s2,gp,-2000 # 30010128 <g_wdt_period>
     ac6:	00092083          	lw	ra,0(s2)
     aca:	62c1                	c.lui	t0,0x10
     acc:	fff28413          	addi	s0,t0,-1 # ffff <SRAM2_SIZE+0xd8f7>
     ad0:	0080f333          	and	t1,ra,s0
     ad4:	005a03b7          	lui	t2,0x5a0
     ad8:	00736533          	or	a0,t1,t2
     adc:	440004b7          	lui	s1,0x44000
     ae0:	60a4a023          	sw	a0,1536(s1) # 44000600 <_stack+0x13fc0600>
     ae4:	8c20                	exec.it	#22     !li	a0,300
     ae6:	2879                	c.jal	b84 <DelayUS>
     ae8:	00092583          	lw	a1,0(s2)
     aec:	00a506b7          	lui	a3,0xa50
     af0:	0085f633          	and	a2,a1,s0
     af4:	00d66833          	or	a6,a2,a3
     af8:	6104a023          	sw	a6,1536(s1)
     afc:	8c20                	exec.it	#22     !li	a0,300
     afe:	2059                	c.jal	b84 <DelayUS>
     b00:	b771                	c.j	a8c <wdt_ctrl+0x24>
     b02:	0005a903          	lw	s2,0(a1) # 44030000 <_stack+0x13ff0000>
     b06:	6441                	c.lui	s0,0x10
     b08:	147d                	c.addi	s0,-1
     b0a:	008970b3          	and	ra,s2,s0
     b0e:	005a0337          	lui	t1,0x5a0
     b12:	8321a823          	sw	s2,-2000(gp) # 30010128 <g_wdt_period>
     b16:	0060e3b3          	or	t2,ra,t1
     b1a:	44000937          	lui	s2,0x44000
     b1e:	8c20                	exec.it	#22     !li	a0,300
     b20:	84ae                	c.mv	s1,a1
     b22:	60792023          	sw	t2,1536(s2) # 44000600 <_stack+0x13fc0600>
     b26:	28b9                	c.jal	b84 <DelayUS>
     b28:	4088                	c.lw	a0,0(s1)
     b2a:	00a50637          	lui	a2,0xa50
     b2e:	008575b3          	and	a1,a0,s0
     b32:	00c5e6b3          	or	a3,a1,a2
     b36:	60d92023          	sw	a3,1536(s2)
     b3a:	b7c9                	c.j	afc <wdt_ctrl+0x94>

00000b3c <wdt_reboot>:
     b3c:	403002ef          	jal	t0,173e <__riscv_save_0>
     b40:	1141                	c.addi	sp,-16
     b42:	0e330537          	lui	a0,0xe330
     b46:	4799                	c.li	a5,6
     b48:	006c                	c.addi4spn	a1,sp,12
     b4a:	0509                	c.addi	a0,2
     b4c:	c63e                	c.swsp	a5,12(sp)
     b4e:	3f29                	c.jal	a68 <wdt_ctrl>
     b50:	44010737          	lui	a4,0x44010
     b54:	03072283          	lw	t0,48(a4) # 44010030 <_stack+0x13fd0030>
     b58:	7802a35b          	bfoz	t1,t0,30,0
     b5c:	02672823          	sw	t1,48(a4)
     b60:	a001                	c.j	b60 <wdt_reboot+0x24>

00000b62 <wdt_close>:
     b62:	440007b7          	lui	a5,0x44000
     b66:	005a06b7          	lui	a3,0x5a0
     b6a:	60d7a023          	sw	a3,1536(a5) # 44000600 <_stack+0x13fc0600>
     b6e:	00a50737          	lui	a4,0xa50
     b72:	448002b7          	lui	t0,0x44800
     b76:	60e7a023          	sw	a4,1536(a5)
     b7a:	00d2a023          	sw	a3,0(t0) # 44800000 <_stack+0x147c0000>
     b7e:	00e2a023          	sw	a4,0(t0)
     b82:	8082                	c.jr	ra

00000b84 <DelayUS>:
     b84:	1101                	c.addi	sp,-32
     b86:	4689                	c.li	a3,2
     b88:	c62a                	c.swsp	a0,12(sp)
     b8a:	47b2                	c.lwsp	a5,12(sp)
     b8c:	fff78713          	addi	a4,a5,-1
     b90:	c63a                	c.swsp	a4,12(sp)
     b92:	e399                	c.bnez	a5,b98 <DelayUS+0x14>
     b94:	6105                	c.addi16sp	sp,32
     b96:	8082                	c.jr	ra
     b98:	ce02                	c.swsp	zero,28(sp)
     b9a:	42f2                	c.lwsp	t0,28(sp)
     b9c:	fe56e7e3          	bltu	a3,t0,b8a <DelayUS+0x6>
     ba0:	4372                	c.lwsp	t1,28(sp)
     ba2:	8430                	exec.it	#19     !addi	t2,t1,1 # 5a0001 <SRAM2_SIZE+0x59d8f9>
     ba4:	ce1e                	c.swsp	t2,28(sp)
     ba6:	bfd5                	c.j	b9a <DelayUS+0x16>

00000ba8 <DelayMS>:
     ba8:	6785                	c.lui	a5,0x1
     baa:	1101                	c.addi	sp,-32
     bac:	a8b78313          	addi	t1,a5,-1397 # a8b <wdt_ctrl+0x23>
     bb0:	c62a                	c.swsp	a0,12(sp)
     bb2:	4732                	c.lwsp	a4,12(sp)
     bb4:	fff70693          	addi	a3,a4,-1 # a4ffff <SRAM2_SIZE+0xa4d8f7>
     bb8:	c636                	c.swsp	a3,12(sp)
     bba:	e319                	c.bnez	a4,bc0 <DelayMS+0x18>
     bbc:	6105                	c.addi16sp	sp,32
     bbe:	8082                	c.jr	ra
     bc0:	ce02                	c.swsp	zero,28(sp)
     bc2:	42f2                	c.lwsp	t0,28(sp)
     bc4:	fe5367e3          	bltu	t1,t0,bb2 <DelayMS+0xa>
     bc8:	43f2                	c.lwsp	t2,28(sp)
     bca:	00138513          	addi	a0,t2,1 # 5a0001 <SRAM2_SIZE+0x59d8f9>
     bce:	ce2a                	c.swsp	a0,28(sp)
     bd0:	bfcd                	c.j	bc2 <DelayMS+0x1a>

00000bd2 <jump_to_app>:
     bd2:	36d002ef          	jal	t0,173e <__riscv_save_0>
     bd6:	6409                	c.lui	s0,0x2
     bd8:	f0040513          	addi	a0,s0,-256 # 1f00 <_data_lmastart+0x380>
     bdc:	f0040413          	addi	s0,s0,-256
     be0:	349d                	c.jal	646 <bk_print_hex>
     be2:	933ff0ef          	jal	ra,514 <uart1_wait_tx_finish>
     be6:	00001097          	auipc	ra,0x1
     bea:	ca6080e7          	jalr	-858(ra) # 188c <bl_hw_board_deinit>
     bee:	9402                	c.jalr	s0
     bf0:	4501                	c.li	a0,0
     bf2:	3710006f          	j	1762 <__riscv_restore_0>

00000bf6 <system_startup>:
     bf6:	349002ef          	jal	t0,173e <__riscv_save_0>
     bfa:	00001517          	auipc	a0,0x1
     bfe:	e3a50513          	addi	a0,a0,-454 # 1a34 <flash_config+0xcc>
     c02:	32cd                	c.jal	5e4 <bk_printf>
     c04:	37f9                	c.jal	bd2 <jump_to_app>
     c06:	35d0006f          	j	1762 <__riscv_restore_0>

00000c0a <boot_main>:
     c0a:	31b002ef          	jal	t0,1724 <__riscv_save_4>
     c0e:	00001097          	auipc	ra,0x1
     c12:	ca0080e7          	jalr	-864(ra) # 18ae <bl_hw_board_init>
     c16:	000629b7          	lui	s3,0x62
     c1a:	6a0d                	c.lui	s4,0x3
     c1c:	00001517          	auipc	a0,0x1
     c20:	e2850513          	addi	a0,a0,-472 # 1a44 <flash_config+0xdc>
     c24:	85818a93          	addi	s5,gp,-1960 # 30010150 <uart_buff_write>
     c28:	83e18413          	addi	s0,gp,-1986 # 30010136 <bim_uart_temp>
     c2c:	83c18493          	addi	s1,gp,-1988 # 30010134 <uart_buff_read>
     c30:	83818913          	addi	s2,gp,-1992 # 30010130 <check_cnt>
     c34:	a8098993          	addi	s3,s3,-1408 # 61a80 <SRAM2_SIZE+0x5f378>
     c38:	2c8a0a13          	addi	s4,s4,712 # 32c8 <SRAM2_SIZE+0xbc0>
     c3c:	9a9ff0ef          	jal	ra,5e4 <bk_printf>
     c40:	000ad583          	lhu	a1,0(s5)
     c44:	0004d503          	lhu	a0,0(s1)
     c48:	00b41023          	sh	a1,0(s0)
     c4c:	02b57363          	bgeu	a0,a1,c72 <boot_main+0x68>
     c50:	40a588b3          	sub	a7,a1,a0
     c54:	30010e17          	auipc	t3,0x30010
     c58:	a9ce0e13          	addi	t3,t3,-1380 # 300106f0 <bim_uart_rx_buf>
     c5c:	3c08a5db          	bfoz	a1,a7,15,0
     c60:	9572                	c.add	a0,t3
     c62:	2eb1                	c.jal	fbe <boot_uart_data_callback>
     c64:	00041e83          	lh	t4,0(s0)
     c68:	8201ac23          	sw	zero,-1992(gp) # 30010130 <check_cnt>
     c6c:	01d49023          	sh	t4,0(s1)
     c70:	bfc1                	c.j	c40 <boot_main+0x36>
     c72:	02a5f163          	bgeu	a1,a0,c94 <boot_main+0x8a>
     c76:	6605                	c.lui	a2,0x1
     c78:	40a60833          	sub	a6,a2,a0
     c7c:	30010b17          	auipc	s6,0x30010
     c80:	a74b0b13          	addi	s6,s6,-1420 # 300106f0 <bim_uart_rx_buf>
     c84:	3c0825db          	bfoz	a1,a6,15,0
     c88:	955a                	c.add	a0,s6
     c8a:	2e15                	c.jal	fbe <boot_uart_data_callback>
     c8c:	00045583          	lhu	a1,0(s0)
     c90:	855a                	c.mv	a0,s6
     c92:	bfc1                	c.j	c62 <boot_main+0x58>
     c94:	00092783          	lw	a5,0(s2)
     c98:	85c1a683          	lw	a3,-1956(gp) # 30010154 <uart_download_status>
     c9c:	00178713          	addi	a4,a5,1
     ca0:	00e92023          	sw	a4,0(s2)
     ca4:	ee91                	c.bnez	a3,cc0 <boot_main+0xb6>
     ca6:	f8fa5de3          	bge	s4,a5,c40 <boot_main+0x36>
     caa:	4085                	c.li	ra,1
     cac:	3000f697          	auipc	a3,0x3000f
     cb0:	4406a823          	sw	zero,1104(a3) # 300100fc <boot_downloading>
     cb4:	8211aa23          	sw	ra,-1996(gp) # 3001012c <startup>
     cb8:	3f3d                	c.jal	bf6 <system_startup>
     cba:	4501                	c.li	a0,0
     cbc:	29d0006f          	j	1758 <__riscv_restore_4>
     cc0:	f8f9d0e3          	bge	s3,a5,c40 <boot_main+0x36>
     cc4:	00001517          	auipc	a0,0x1
     cc8:	d9450513          	addi	a0,a0,-620 # 1a58 <flash_config+0xf0>
     ccc:	919ff0ef          	jal	ra,5e4 <bk_printf>
     cd0:	440102b7          	lui	t0,0x44010
     cd4:	03c2a303          	lw	t1,60(t0) # 4401003c <_stack+0x13fd003c>
     cd8:	ffd37393          	andi	t2,t1,-3
     cdc:	0272ae23          	sw	t2,60(t0)
     ce0:	3db1                	c.jal	b3c <wdt_reboot>
     ce2:	bfb9                	c.j	c40 <boot_main+0x36>

00000ce4 <make_crc32_table>:
     ce4:	edb88637          	lui	a2,0xedb88
     ce8:	3000f697          	auipc	a3,0x3000f
     cec:	60868693          	addi	a3,a3,1544 # 300102f0 <crc32_table>
     cf0:	4701                	c.li	a4,0
     cf2:	32060293          	addi	t0,a2,800 # edb88320 <_stack+0xbdb48320>
     cf6:	10000513          	li	a0,256
     cfa:	87ba                	c.mv	a5,a4
     cfc:	45a1                	c.li	a1,8
     cfe:	0017f813          	andi	a6,a5,1
     d02:	8385                	c.srli	a5,0x1
     d04:	00080463          	beqz	a6,d0c <make_crc32_table+0x28>
     d08:	0057c7b3          	xor	a5,a5,t0
     d0c:	15fd                	c.addi	a1,-1
     d0e:	f9e5                	c.bnez	a1,cfe <make_crc32_table+0x1a>
     d10:	c29c                	c.sw	a5,0(a3)
     d12:	0705                	c.addi	a4,1
     d14:	0691                	c.addi	a3,4
     d16:	fea712e3          	bne	a4,a0,cfa <make_crc32_table+0x16>
     d1a:	4501                	c.li	a0,0
     d1c:	8082                	c.jr	ra

00000d1e <make_crc32>:
     d1e:	962e                	c.add	a2,a1
     d20:	3000f717          	auipc	a4,0x3000f
     d24:	5d070713          	addi	a4,a4,1488 # 300102f0 <crc32_table>
     d28:	00c59363          	bne	a1,a2,d2e <make_crc32+0x10>
     d2c:	8082                	c.jr	ra
     d2e:	0005c783          	lbu	a5,0(a1)
     d32:	00855693          	srli	a3,a0,0x8
     d36:	8d3d                	c.xor	a0,a5
     d38:	089522db          	bfoz	t0,a0,2,9
     d3c:	00570333          	add	t1,a4,t0
     d40:	8030                	exec.it	#17     !lw	t2,0(t1)
     d42:	0585                	c.addi	a1,1
     d44:	0076c533          	xor	a0,a3,t2
     d48:	b7c5                	c.j	d28 <make_crc32+0xa>

00000d4a <uart_download_rx>:
     d4a:	85818793          	addi	a5,gp,-1960 # 30010150 <uart_buff_write>
     d4e:	4394                	c.lw	a3,0(a5)
     d50:	30010297          	auipc	t0,0x30010
     d54:	9a028293          	addi	t0,t0,-1632 # 300106f0 <bim_uart_rx_buf>
     d58:	00168713          	addi	a4,a3,1
     d5c:	00d28333          	add	t1,t0,a3
     d60:	6385                	c.lui	t2,0x1
     d62:	c398                	c.sw	a4,0(a5)
     d64:	00a30023          	sb	a0,0(t1)
     d68:	00771463          	bne	a4,t2,d70 <uart_download_rx+0x26>
     d6c:	8401ac23          	sw	zero,-1960(gp) # 30010150 <uart_buff_write>
     d70:	8082                	c.jr	ra

00000d72 <dl_uart_init>:
     d72:	1cd002ef          	jal	t0,173e <__riscv_save_0>
     d76:	3000f797          	auipc	a5,0x3000f
     d7a:	3827a783          	lw	a5,898(a5) # 300100f8 <__fini_array_end>
     d7e:	0017e65b          	bnec	a5,1,d8a <dl_uart_init+0x18>
     d82:	c3cff0ef          	jal	ra,1be <uart0_init>
     d86:	1dd0006f          	j	1762 <__riscv_restore_0>
     d8a:	be27ee5b          	bnec	a5,2,d86 <dl_uart_init+0x14>
     d8e:	dd0ff0ef          	jal	ra,35e <uart1_init>
     d92:	bfd5                	c.j	d86 <dl_uart_init+0x14>

00000d94 <dl_uart_send>:
     d94:	1ab002ef          	jal	t0,173e <__riscv_save_0>
     d98:	3000f797          	auipc	a5,0x3000f
     d9c:	3607a783          	lw	a5,864(a5) # 300100f8 <__fini_array_end>
     da0:	0017e65b          	bnec	a5,1,dac <dl_uart_send+0x18>
     da4:	d94ff0ef          	jal	ra,338 <uart0_send>
     da8:	1bb0006f          	j	1762 <__riscv_restore_0>
     dac:	be27ee5b          	bnec	a5,2,da8 <dl_uart_send+0x14>
     db0:	f22ff0ef          	jal	ra,4d2 <uart1_send>
     db4:	bfd5                	c.j	da8 <dl_uart_send+0x14>

00000db6 <cmd_response>:
     db6:	189002ef          	jal	t0,173e <__riscv_save_0>
     dba:	6785                	c.lui	a5,0x1
     dbc:	9e818713          	addi	a4,gp,-1560 # 300102e0 <cmd_res_buff>
     dc0:	e0478093          	addi	ra,a5,-508 # e04 <cmd_response+0x4e>
     dc4:	4285                	c.li	t0,1
     dc6:	ce000313          	li	t1,-800
     dca:	4e01                	c.li	t3,0
     dcc:	ffc58693          	addi	a3,a1,-4
     dd0:	00171023          	sh	ra,0(a4)
     dd4:	00b70123          	sb	a1,2(a4)
     dd8:	005701a3          	sb	t0,3(a4)
     ddc:	00671223          	sh	t1,4(a4)
     de0:	00a70323          	sb	a0,6(a4)
     de4:	00de4863          	blt	t3,a3,df4 <cmd_response+0x3e>
     de8:	058d                	c.addi	a1,3
     dea:	9e818513          	addi	a0,gp,-1560 # 300102e0 <cmd_res_buff>
     dee:	375d                	c.jal	d94 <dl_uart_send>
     df0:	1730006f          	j	1762 <__riscv_restore_0>
     df4:	01c60533          	add	a0,a2,t3
     df8:	00054803          	lbu	a6,0(a0)
     dfc:	01c703b3          	add	t2,a4,t3
     e00:	001e0893          	addi	a7,t3,1
     e04:	0ff8fe13          	andi	t3,a7,255
     e08:	010383a3          	sb	a6,7(t2) # 1007 <boot_uart_data_callback+0x49>
     e0c:	bfe1                	c.j	de4 <cmd_response+0x2e>

00000e0e <operate_flash_cmd_response>:
     e0e:	131002ef          	jal	t0,173e <__riscv_save_0>
     e12:	84c19823          	sh	a2,-1968(gp) # 30010148 <cmd_res_length>
     e16:	01ff17b7          	lui	a5,0x1ff1
     e1a:	c0010113          	addi	sp,sp,-1024
     e1e:	e0478093          	addi	ra,a5,-508 # 1ff0e04 <SRAM2_SIZE+0x1fee6fc>
     e22:	ce000293          	li	t0,-800
     e26:	5351                	c.li	t1,-12
     e28:	4e01                	c.li	t3,0
     e2a:	ffe60713          	addi	a4,a2,-2
     e2e:	c006                	c.swsp	ra,0(sp)
     e30:	00511223          	sh	t0,4(sp)
     e34:	00610323          	sb	t1,6(sp)
     e38:	00c103a3          	sb	a2,7(sp)
     e3c:	00010423          	sb	zero,8(sp)
     e40:	00a104a3          	sb	a0,9(sp)
     e44:	00b10523          	sb	a1,10(sp)
     e48:	00ee4a63          	blt	t3,a4,e5c <operate_flash_cmd_response+0x4e>
     e4c:	850a                	c.mv	a0,sp
     e4e:	00960593          	addi	a1,a2,9
     e52:	3789                	c.jal	d94 <dl_uart_send>
     e54:	40010113          	addi	sp,sp,1024
     e58:	10b0006f          	j	1762 <__riscv_restore_0>
     e5c:	01c68533          	add	a0,a3,t3
     e60:	00050803          	lb	a6,0(a0)
     e64:	40010593          	addi	a1,sp,1024
     e68:	01c583b3          	add	t2,a1,t3
     e6c:	001e0893          	addi	a7,t3,1
     e70:	3c08ae5b          	bfoz	t3,a7,15,0
     e74:	c10385a3          	sb	a6,-1013(t2)
     e78:	bfc1                	c.j	e48 <operate_flash_cmd_response+0x3a>

00000e7a <uart_cmd_dispath>:
     e7a:	8004                	exec.it	#8     !jal	t0,1724 <__riscv_save_4>
     e7c:	00054783          	lbu	a5,0(a0)
     e80:	716d                	c.addi16sp	sp,-272
     e82:	842a                	c.mv	s0,a0
     e84:	0507d55b          	beqc	a5,16,ece <uart_cmd_dispath+0x54>
     e88:	4341                	c.li	t1,16
     e8a:	00f36863          	bltu	t1,a5,e9a <uart_cmd_dispath+0x20>
     e8e:	10e7d65b          	beqc	a5,14,f9a <uart_cmd_dispath+0x120>
     e92:	0cf7d85b          	beqc	a5,15,f62 <uart_cmd_dispath+0xe8>
     e96:	6151                	c.addi16sp	sp,272
     e98:	8400                	exec.it	#2     !j	1758 <__riscv_restore_4>
     e9a:	0aa00393          	li	t2,170
     e9e:	fe779ce3          	bne	a5,t2,e96 <uart_cmd_dispath+0x1c>
     ea2:	86018513          	addi	a0,gp,-1952 # 30010158 <flag_boot>
     ea6:	00054583          	lbu	a1,0(a0)
     eaa:	f5f5                	c.bnez	a1,e96 <uart_cmd_dispath+0x1c>
     eac:	4485                	c.li	s1,1
     eae:	8491ae23          	sw	s1,-1956(gp) # 30010154 <uart_download_status>
     eb2:	00140083          	lb	ra,1(s0)
     eb6:	00950023          	sb	s1,0(a0)
     eba:	860a                	c.mv	a2,sp
     ebc:	4595                	c.li	a1,5
     ebe:	0aa00513          	li	a0,170
     ec2:	00110023          	sb	ra,0(sp)
     ec6:	3dc5                	c.jal	db6 <cmd_response>
     ec8:	8491aa23          	sw	s1,-1964(gp) # 3001014c <erase_fenable>
     ecc:	b7e9                	c.j	e96 <uart_cmd_dispath+0x1c>
     ece:	00254483          	lbu	s1,2(a0)
     ed2:	00644703          	lbu	a4,6(s0)
     ed6:	00744983          	lbu	s3,7(s0)
     eda:	00154283          	lbu	t0,1(a0)
     ede:	00544883          	lbu	a7,5(s0)
     ee2:	00354383          	lbu	t2,3(a0)
     ee6:	00444603          	lbu	a2,4(s0)
     eea:	00849093          	slli	ra,s1,0x8
     eee:	00844403          	lbu	s0,8(s0)
     ef2:	00871813          	slli	a6,a4,0x8
     ef6:	01180933          	add	s2,a6,a7
     efa:	01099e13          	slli	t3,s3,0x10
     efe:	00508333          	add	t1,ra,t0
     f02:	01039513          	slli	a0,t2,0x10
     f06:	01841f13          	slli	t5,s0,0x18
     f0a:	00a305b3          	add	a1,t1,a0
     f0e:	01861693          	slli	a3,a2,0x18
     f12:	01c90eb3          	add	t4,s2,t3
     f16:	01ee8933          	add	s2,t4,t5
     f1a:	00d584b3          	add	s1,a1,a3
     f1e:	33d9                	c.jal	ce4 <make_crc32_table>
     f20:	00190f93          	addi	t6,s2,1
     f24:	409f87b3          	sub	a5,t6,s1
     f28:	0087d413          	srli	s0,a5,0x8
     f2c:	4981                	c.li	s3,0
     f2e:	597d                	c.li	s2,-1
     f30:	0089e863          	bltu	s3,s0,f40 <uart_cmd_dispath+0xc6>
     f34:	860a                	c.mv	a2,sp
     f36:	45a1                	c.li	a1,8
     f38:	4541                	c.li	a0,16
     f3a:	c04a                	c.swsp	s2,0(sp)
     f3c:	3dad                	c.jal	db6 <cmd_response>
     f3e:	bfa1                	c.j	e96 <uart_cmd_dispath+0x1c>
     f40:	85a6                	c.mv	a1,s1
     f42:	8020                	exec.it	#16     !li	a2,256
     f44:	0808                	c.addi4spn	a0,sp,16
     f46:	9ebff0ef          	jal	ra,930 <flash_read_data>
     f4a:	854a                	c.mv	a0,s2
     f4c:	8020                	exec.it	#16     !li	a2,256
     f4e:	080c                	c.addi4spn	a1,sp,16
     f50:	33f9                	c.jal	d1e <make_crc32>
     f52:	00198293          	addi	t0,s3,1
     f56:	892a                	c.mv	s2,a0
     f58:	10048493          	addi	s1,s1,256
     f5c:	3c02a9db          	bfoz	s3,t0,15,0
     f60:	bfc1                	c.j	f30 <uart_cmd_dispath+0xb6>
     f62:	00254e03          	lbu	t3,2(a0)
     f66:	00154f03          	lbu	t5,1(a0)
     f6a:	00354783          	lbu	a5,3(a0)
     f6e:	00454383          	lbu	t2,4(a0)
     f72:	008e1e93          	slli	t4,t3,0x8
     f76:	01ee8fb3          	add	t6,t4,t5
     f7a:	01079293          	slli	t0,a5,0x10
     f7e:	005f8333          	add	t1,t6,t0
     f82:	01839513          	slli	a0,t2,0x18
     f86:	951a                	c.add	a0,t1
     f88:	33ed                	c.jal	d72 <dl_uart_init>
     f8a:	00544503          	lbu	a0,5(s0)
     f8e:	3929                	c.jal	ba8 <DelayMS>
     f90:	00140613          	addi	a2,s0,1
     f94:	45a5                	c.li	a1,9
     f96:	453d                	c.li	a0,15
     f98:	b755                	c.j	f3c <uart_cmd_dispath+0xc2>
     f9a:	00154603          	lbu	a2,1(a0)
     f9e:	0a500693          	li	a3,165
     fa2:	eed61ae3          	bne	a2,a3,e96 <uart_cmd_dispath+0x1c>
     fa6:	4505                	c.li	a0,1
     fa8:	3101                	c.jal	ba8 <DelayMS>
     faa:	44010737          	lui	a4,0x44010
     fae:	03c72803          	lw	a6,60(a4) # 4401003c <_stack+0x13fd003c>
     fb2:	ffd87893          	andi	a7,a6,-3
     fb6:	03172e23          	sw	a7,60(a4)
     fba:	3649                	c.jal	b3c <wdt_reboot>
     fbc:	bde9                	c.j	e96 <uart_cmd_dispath+0x1c>

00000fbe <boot_uart_data_callback>:
     fbe:	758002ef          	jal	t0,1716 <__riscv_save_10>
     fc2:	84aa                	c.mv	s1,a0
     fc4:	89ae                	c.mv	s3,a1
     fc6:	84e18a93          	addi	s5,gp,-1970 # 30010146 <cmd_status.6>
     fca:	00001c17          	auipc	s8,0x1
     fce:	a9ec0c13          	addi	s8,s8,-1378 # 1a68 <flash_config+0x100>
     fd2:	84818b13          	addi	s6,gp,-1976 # 30010140 <index.4>
     fd6:	30010417          	auipc	s0,0x30010
     fda:	71a40413          	addi	s0,s0,1818 # 300116f0 <bim_uart_data>
     fde:	84418a13          	addi	s4,gp,-1980 # 3001013c <scmd_length.3>
     fe2:	82c18c93          	addi	s9,gp,-2004 # 30010124 <flash_id>
     fe6:	84018b93          	addi	s7,gp,-1984 # 30010138 <index_cnt.2>
     fea:	85418d13          	addi	s10,gp,-1964 # 3001014c <erase_fenable>
     fee:	00099463          	bnez	s3,ff6 <boot_uart_data_callback+0x38>
     ff2:	75c0006f          	j	174e <__riscv_restore_10>
     ff6:	000ac783          	lbu	a5,0(s5)
     ffa:	4721                	c.li	a4,8
     ffc:	00f76e63          	bltu	a4,a5,1018 <boot_uart_data_callback+0x5a>
    1000:	0cfc02db          	lea.w	t0,s8,a5
    1004:	8404                	exec.it	#10     !lw	t1,0(t0)
    1006:	018303b3          	add	t2,t1,s8
    100a:	8382                	c.jr	t2
    100c:	0004c703          	lbu	a4,0(s1)
    1010:	0017645b          	bnec	a4,1,1018 <boot_uart_data_callback+0x5a>
    1014:	00ea8023          	sb	a4,0(s5)
    1018:	19fd                	c.addi	s3,-1
    101a:	3c09a9db          	bfoz	s3,s3,15,0
    101e:	0485                	c.addi	s1,1
    1020:	b7f9                	c.j	fee <boot_uart_data_callback+0x30>
    1022:	0004c283          	lbu	t0,0(s1)
    1026:	0e000313          	li	t1,224
    102a:	04629a63          	bne	t0,t1,107e <boot_uart_data_callback+0xc0>
    102e:	4709                	c.li	a4,2
    1030:	b7d5                	c.j	1014 <boot_uart_data_callback+0x56>
    1032:	0004c703          	lbu	a4,0(s1)
    1036:	0fc00793          	li	a5,252
    103a:	04f71263          	bne	a4,a5,107e <boot_uart_data_callback+0xc0>
    103e:	470d                	c.li	a4,3
    1040:	bfd1                	c.j	1014 <boot_uart_data_callback+0x56>
    1042:	0004c503          	lbu	a0,0(s1)
    1046:	84a19623          	sh	a0,-1972(gp) # 30010144 <length.5>
    104a:	0ff00693          	li	a3,255
    104e:	3c05285b          	bfoz	a6,a0,15,0
    1052:	00d51463          	bne	a0,a3,105a <boot_uart_data_callback+0x9c>
    1056:	4715                	c.li	a4,5
    1058:	bf75                	c.j	1014 <boot_uart_data_callback+0x56>
    105a:	fff50893          	addi	a7,a0,-1
    105e:	0ff8fe13          	andi	t3,a7,255
    1062:	0fd00e93          	li	t4,253
    1066:	01ceec63          	bltu	t4,t3,107e <boot_uart_data_callback+0xc0>
    106a:	00286d5b          	bnec	a6,2,1084 <boot_uart_data_callback+0xc6>
    106e:	8601cf03          	lbu	t5,-1952(gp) # 30010158 <flag_boot>
    1072:	001f695b          	bnec	t5,1,1084 <boot_uart_data_callback+0xc6>
    1076:	19fd                	c.addi	s3,-1
    1078:	3c09a9db          	bfoz	s3,s3,15,0
    107c:	0485                	c.addi	s1,1
    107e:	84018723          	sb	zero,-1970(gp) # 30010146 <cmd_status.6>
    1082:	bf59                	c.j	1018 <boot_uart_data_callback+0x5a>
    1084:	4f91                	c.li	t6,4
    1086:	8401a423          	sw	zero,-1976(gp) # 30010140 <index.4>
    108a:	01fa8023          	sb	t6,0(s5)
    108e:	b769                	c.j	1018 <boot_uart_data_callback+0x5a>
    1090:	000b2283          	lw	t0,0(s6)
    1094:	0004c383          	lbu	t2,0(s1)
    1098:	30011517          	auipc	a0,0x30011
    109c:	66050513          	addi	a0,a0,1632 # 300126f8 <bim_uart_cmd>
    10a0:	00128313          	addi	t1,t0,1
    10a4:	005505b3          	add	a1,a0,t0
    10a8:	84c1d603          	lhu	a2,-1972(gp) # 30010144 <length.5>
    10ac:	006b2023          	sw	t1,0(s6)
    10b0:	00758023          	sb	t2,0(a1)
    10b4:	f6c312e3          	bne	t1,a2,1018 <boot_uart_data_callback+0x5a>
    10b8:	0ff37593          	andi	a1,t1,255
    10bc:	3b7d                	c.jal	e7a <uart_cmd_dispath>
    10be:	84018723          	sb	zero,-1970(gp) # 30010146 <cmd_status.6>
    10c2:	bf99                	c.j	1018 <boot_uart_data_callback+0x5a>
    10c4:	0004cf83          	lbu	t6,0(s1)
    10c8:	0f400793          	li	a5,244
    10cc:	faff99e3          	bne	t6,a5,107e <boot_uart_data_callback+0xc0>
    10d0:	4719                	c.li	a4,6
    10d2:	b789                	c.j	1014 <boot_uart_data_callback+0x56>
    10d4:	0004cf03          	lbu	t5,0(s1)
    10d8:	4e9d                	c.li	t4,7
    10da:	01da8023          	sb	t4,0(s5)
    10de:	01ea1023          	sh	t5,0(s4)
    10e2:	bf1d                	c.j	1018 <boot_uart_data_callback+0x5a>
    10e4:	0004c503          	lbu	a0,0(s1)
    10e8:	000a1603          	lh	a2,0(s4)
    10ec:	00851593          	slli	a1,a0,0x8
    10f0:	00c586b3          	add	a3,a1,a2
    10f4:	3c06a85b          	bfoz	a6,a3,15,0
    10f8:	010038b3          	snez	a7,a6
    10fc:	00389e13          	slli	t3,a7,0x3
    1100:	8401a423          	sw	zero,-1976(gp) # 30010140 <index.4>
    1104:	010a1023          	sh	a6,0(s4)
    1108:	01ca8023          	sb	t3,0(s5)
    110c:	8401a023          	sw	zero,-1984(gp) # 30010138 <index_cnt.2>
    1110:	b721                	c.j	1018 <boot_uart_data_callback+0x5a>
    1112:	000b2383          	lw	t2,0(s6)
    1116:	0004c583          	lbu	a1,0(s1)
    111a:	00138813          	addi	a6,t2,1
    111e:	00740633          	add	a2,s0,t2
    1122:	00b60023          	sb	a1,0(a2)
    1126:	010b2023          	sw	a6,0(s6)
    112a:	8414                	exec.it	#11     !lbu	a0,0(s0)
    112c:	06956a5b          	bnec	a0,9,11a0 <boot_uart_data_callback+0x1e2>
    1130:	000a5503          	lhu	a0,0(s4)
    1134:	eea812e3          	bne	a6,a0,1018 <boot_uart_data_callback+0x5a>
    1138:	00244083          	lbu	ra,2(s0)
    113c:	00144603          	lbu	a2,1(s0)
    1140:	00344883          	lbu	a7,3(s0)
    1144:	00809593          	slli	a1,ra,0x8
    1148:	00444f03          	lbu	t5,4(s0)
    114c:	00c586b3          	add	a3,a1,a2
    1150:	01089e13          	slli	t3,a7,0x10
    1154:	01c68eb3          	add	t4,a3,t3
    1158:	018f1f93          	slli	t6,t5,0x18
    115c:	01fe85b3          	add	a1,t4,t6
    1160:	6709                	c.lui	a4,0x2
    1162:	00e5ff63          	bgeu	a1,a4,1180 <boot_uart_data_callback+0x1c2>
    1166:	ffc38793          	addi	a5,t2,-4
    116a:	30010697          	auipc	a3,0x30010
    116e:	58768693          	addi	a3,a3,1415 # 300116f1 <bim_uart_data+0x1>
    1172:	461d                	c.li	a2,7
    1174:	4599                	c.li	a1,6
    1176:	00f402a3          	sb	a5,5(s0)
    117a:	4525                	c.li	a0,9
    117c:	3949                	c.jal	e0e <operate_flash_cmd_response>
    117e:	b781                	c.j	10be <boot_uart_data_callback+0x100>
    1180:	6605                	c.lui	a2,0x1
    1182:	30010517          	auipc	a0,0x30010
    1186:	57350513          	addi	a0,a0,1395 # 300116f5 <bim_uart_data+0x5>
    118a:	fa6ff0ef          	jal	ra,930 <flash_read_data>
    118e:	6385                	c.lui	t2,0x1
    1190:	30010697          	auipc	a3,0x30010
    1194:	56168693          	addi	a3,a3,1377 # 300116f1 <bim_uart_data+0x1>
    1198:	00638613          	addi	a2,t2,6 # 1006 <boot_uart_data_callback+0x48>
    119c:	4581                	c.li	a1,0
    119e:	bff1                	c.j	117a <boot_uart_data_callback+0x1bc>
    11a0:	08f56b5b          	bnec	a0,15,1236 <boot_uart_data_callback+0x278>
    11a4:	000a5583          	lhu	a1,0(s4)
    11a8:	e6b818e3          	bne	a6,a1,1018 <boot_uart_data_callback+0x5a>
    11ac:	00344903          	lbu	s2,3(s0)
    11b0:	00244683          	lbu	a3,2(s0)
    11b4:	00444e03          	lbu	t3,4(s0)
    11b8:	00891613          	slli	a2,s2,0x8
    11bc:	00544f83          	lbu	t6,5(s0)
    11c0:	00d608b3          	add	a7,a2,a3
    11c4:	010e1e93          	slli	t4,t3,0x10
    11c8:	01d88f33          	add	t5,a7,t4
    11cc:	018f9713          	slli	a4,t6,0x18
    11d0:	00ef0933          	add	s2,t5,a4
    11d4:	6789                	c.lui	a5,0x2
    11d6:	00f95a63          	bge	s2,a5,11ea <boot_uart_data_callback+0x22c>
    11da:	30010697          	auipc	a3,0x30010
    11de:	51768693          	addi	a3,a3,1303 # 300116f1 <bim_uart_data+0x1>
    11e2:	461d                	c.li	a2,7
    11e4:	4599                	c.li	a1,6
    11e6:	453d                	c.li	a0,15
    11e8:	bf51                	c.j	117c <boot_uart_data_callback+0x1be>
    11ea:	000d2283          	lw	t0,0(s10)
    11ee:	0012e75b          	bnec	t0,1,11fc <boot_uart_data_callback+0x23e>
    11f2:	4501                	c.li	a0,0
    11f4:	d98ff0ef          	jal	ra,78c <set_flash_protect>
    11f8:	8401aa23          	sw	zero,-1964(gp) # 3001014c <erase_fenable>
    11fc:	00144303          	lbu	t1,1(s0)
    1200:	00036cdb          	bnec	t1,32,1218 <boot_uart_data_callback+0x25a>
    1204:	854a                	c.mv	a0,s2
    1206:	e40ff0ef          	jal	ra,846 <flash_erase_sector>
    120a:	30010697          	auipc	a3,0x30010
    120e:	4e768693          	addi	a3,a3,1255 # 300116f1 <bim_uart_data+0x1>
    1212:	461d                	c.li	a2,7
    1214:	4581                	c.li	a1,0
    1216:	bfc1                	c.j	11e6 <boot_uart_data_callback+0x228>
    1218:	0d800813          	li	a6,216
    121c:	01031663          	bne	t1,a6,1228 <boot_uart_data_callback+0x26a>
    1220:	854a                	c.mv	a0,s2
    1222:	e5cff0ef          	jal	ra,87e <flash_erase_block>
    1226:	b7d5                	c.j	120a <boot_uart_data_callback+0x24c>
    1228:	ff23615b          	bnec	t1,82,120a <boot_uart_data_callback+0x24c>
    122c:	65a1                	c.lui	a1,0x8
    122e:	854a                	c.mv	a0,s2
    1230:	e86ff0ef          	jal	ra,8b6 <flash_erase>
    1234:	bfd9                	c.j	120a <boot_uart_data_callback+0x24c>
    1236:	06656f5b          	bnec	a0,6,12b4 <boot_uart_data_callback+0x2f6>
    123a:	000a5883          	lhu	a7,0(s4)
    123e:	dd181de3          	bne	a6,a7,1018 <boot_uart_data_callback+0x5a>
    1242:	00244e03          	lbu	t3,2(s0)
    1246:	00144f03          	lbu	t5,1(s0)
    124a:	00344703          	lbu	a4,3(s0)
    124e:	00444303          	lbu	t1,4(s0)
    1252:	008e1e93          	slli	t4,t3,0x8
    1256:	01ee8fb3          	add	t6,t4,t5
    125a:	01071793          	slli	a5,a4,0x10
    125e:	00ff82b3          	add	t0,t6,a5
    1262:	01831813          	slli	a6,t1,0x18
    1266:	010285b3          	add	a1,t0,a6
    126a:	6089                	c.lui	ra,0x2
    126c:	ffc38613          	addi	a2,t2,-4
    1270:	0015ff63          	bgeu	a1,ra,128e <boot_uart_data_callback+0x2d0>
    1274:	00c402a3          	sb	a2,5(s0)
    1278:	30010697          	auipc	a3,0x30010
    127c:	47968693          	addi	a3,a3,1145 # 300116f1 <bim_uart_data+0x1>
    1280:	461d                	c.li	a2,7
    1282:	4599                	c.li	a1,6
    1284:	4519                	c.li	a0,6
    1286:	3661                	c.jal	e0e <operate_flash_cmd_response>
    1288:	84018723          	sb	zero,-1970(gp) # 30010146 <cmd_status.6>
    128c:	b371                	c.j	1018 <boot_uart_data_callback+0x5a>
    128e:	30010517          	auipc	a0,0x30010
    1292:	46750513          	addi	a0,a0,1127 # 300116f5 <bim_uart_data+0x5>
    1296:	f28ff0ef          	jal	ra,9be <flash_write_data>
    129a:	000a1383          	lh	t2,0(s4)
    129e:	30010697          	auipc	a3,0x30010
    12a2:	45368693          	addi	a3,a3,1107 # 300116f1 <bim_uart_data+0x1>
    12a6:	ffb38513          	addi	a0,t2,-5
    12aa:	461d                	c.li	a2,7
    12ac:	4581                	c.li	a1,0
    12ae:	00a402a3          	sb	a0,5(s0)
    12b2:	bfc9                	c.j	1284 <boot_uart_data_callback+0x2c6>
    12b4:	0a75695b          	bnec	a0,7,1366 <boot_uart_data_callback+0x3a8>
    12b8:	000ba303          	lw	t1,0(s7) # 40000000 <_stack+0xffc0000>
    12bc:	8430                	exec.it	#19     !addi	t2,t1,1
    12be:	00839513          	slli	a0,t2,0x8
    12c2:	00550593          	addi	a1,a0,5
    12c6:	d4b869e3          	bltu	a6,a1,1018 <boot_uart_data_callback+0x5a>
    12ca:	00244603          	lbu	a2,2(s0)
    12ce:	00144883          	lbu	a7,1(s0)
    12d2:	00344e83          	lbu	t4,3(s0)
    12d6:	00444703          	lbu	a4,4(s0)
    12da:	00861693          	slli	a3,a2,0x8
    12de:	01168e33          	add	t3,a3,a7
    12e2:	010e9f13          	slli	t5,t4,0x10
    12e6:	01ee0fb3          	add	t6,t3,t5
    12ea:	01871793          	slli	a5,a4,0x18
    12ee:	00ff82b3          	add	t0,t6,a5
    12f2:	6309                	c.lui	t1,0x2
    12f4:	0262f763          	bgeu	t0,t1,1322 <boot_uart_data_callback+0x364>
    12f8:	000a5683          	lhu	a3,0(s4)
    12fc:	007ba023          	sw	t2,0(s7)
    1300:	d0d81ce3          	bne	a6,a3,1018 <boot_uart_data_callback+0x5a>
    1304:	30010697          	auipc	a3,0x30010
    1308:	3ed68693          	addi	a3,a3,1005 # 300116f1 <bim_uart_data+0x1>
    130c:	4619                	c.li	a2,6
    130e:	4599                	c.li	a1,6
    1310:	451d                	c.li	a0,7
    1312:	006410a3          	sh	t1,1(s0)
    1316:	3ce5                	c.jal	e0e <operate_flash_cmd_response>
    1318:	84018723          	sb	zero,-1970(gp) # 30010146 <cmd_status.6>
    131c:	8401a023          	sw	zero,-1984(gp) # 30010138 <index_cnt.2>
    1320:	b9e5                	c.j	1018 <boot_uart_data_callback+0x5a>
    1322:	f0050813          	addi	a6,a0,-256
    1326:	f0550093          	addi	ra,a0,-251
    132a:	8020                	exec.it	#16     !li	a2,256
    132c:	005805b3          	add	a1,a6,t0
    1330:	00140533          	add	a0,s0,ra
    1334:	e8aff0ef          	jal	ra,9be <flash_write_data>
    1338:	000ba383          	lw	t2,0(s7)
    133c:	000a5583          	lhu	a1,0(s4)
    1340:	00138513          	addi	a0,t2,1
    1344:	000b2603          	lw	a2,0(s6)
    1348:	00aba023          	sw	a0,0(s7)
    134c:	ccc596e3          	bne	a1,a2,1018 <boot_uart_data_callback+0x5a>
    1350:	30010697          	auipc	a3,0x30010
    1354:	3a168693          	addi	a3,a3,929 # 300116f1 <bim_uart_data+0x1>
    1358:	4619                	c.li	a2,6
    135a:	4581                	c.li	a1,0
    135c:	451d                	c.li	a0,7
    135e:	3c45                	c.jal	e0e <operate_flash_cmd_response>
    1360:	84018723          	sb	zero,-1970(gp) # 30010146 <cmd_status.6>
    1364:	b365                	c.j	110c <boot_uart_data_callback+0x14e>
    1366:	02c5695b          	bnec	a0,12,1398 <boot_uart_data_callback+0x3da>
    136a:	0014c903          	lbu	s2,1(s1)
    136e:	854a                	c.mv	a0,s2
    1370:	b6cff0ef          	jal	ra,6dc <flash_read_sr>
    1374:	4611                	c.li	a2,4
    1376:	00a40123          	sb	a0,2(s0)
    137a:	30010697          	auipc	a3,0x30010
    137e:	37768693          	addi	a3,a3,887 # 300116f1 <bim_uart_data+0x1>
    1382:	4581                	c.li	a1,0
    1384:	4531                	c.li	a0,12
    1386:	012400a3          	sb	s2,1(s0)
    138a:	3451                	c.jal	e0e <operate_flash_cmd_response>
    138c:	4505                	c.li	a0,1
    138e:	84018723          	sb	zero,-1970(gp) # 30010146 <cmd_status.6>
    1392:	bfaff0ef          	jal	ra,78c <set_flash_protect>
    1396:	b149                	c.j	1018 <boot_uart_data_callback+0x5a>
    1398:	04e5605b          	bnec	a0,14,13d8 <boot_uart_data_callback+0x41a>
    139c:	bd6ff0ef          	jal	ra,772 <clr_flash_protect>
    13a0:	000c9e03          	lh	t3,0(s9)
    13a4:	000ca703          	lw	a4,0(s9)
    13a8:	3c8e2edb          	bfoz	t4,t3,15,8
    13ac:	20fe2f5b          	bfoz	t5,t3,8,15
    13b0:	01df6fb3          	or	t6,t5,t4
    13b4:	01875793          	srli	a5,a4,0x18
    13b8:	01075293          	srli	t0,a4,0x10
    13bc:	30010697          	auipc	a3,0x30010
    13c0:	33868693          	addi	a3,a3,824 # 300116f4 <bim_uart_data+0x4>
    13c4:	4619                	c.li	a2,6
    13c6:	4581                	c.li	a1,0
    13c8:	4539                	c.li	a0,14
    13ca:	00f40223          	sb	a5,4(s0)
    13ce:	005402a3          	sb	t0,5(s0)
    13d2:	01f41323          	sh	t6,6(s0)
    13d6:	bd45                	c.j	1286 <boot_uart_data_callback+0x2c8>
    13d8:	04d5625b          	bnec	a0,13,141c <boot_uart_data_callback+0x45e>
    13dc:	0014c083          	lbu	ra,1(s1)
    13e0:	0024c883          	lbu	a7,2(s1)
    13e4:	001400a3          	sb	ra,1(s0)
    13e8:	01140123          	sb	a7,2(s0)
    13ec:	b86ff0ef          	jal	ra,772 <clr_flash_protect>
    13f0:	00244583          	lbu	a1,2(s0)
    13f4:	00144503          	lbu	a0,1(s0)
    13f8:	b2cff0ef          	jal	ra,724 <flash_write_sr>
    13fc:	00144503          	lbu	a0,1(s0)
    1400:	adcff0ef          	jal	ra,6dc <flash_read_sr>
    1404:	30010697          	auipc	a3,0x30010
    1408:	2ed68693          	addi	a3,a3,749 # 300116f1 <bim_uart_data+0x1>
    140c:	00a40123          	sb	a0,2(s0)
    1410:	4611                	c.li	a2,4
    1412:	4581                	c.li	a1,0
    1414:	4535                	c.li	a0,13
    1416:	9f9ff0ef          	jal	ra,e0e <operate_flash_cmd_response>
    141a:	b195                	c.j	107e <boot_uart_data_callback+0xc0>
    141c:	000a5683          	lhu	a3,0(s4)
    1420:	c8d80fe3          	beq	a6,a3,10be <boot_uart_data_callback+0x100>
    1424:	bed5                	c.j	1018 <boot_uart_data_callback+0x5a>

00001426 <reset_handler>:
    1426:	318002ef          	jal	t0,173e <__riscv_save_0>
    142a:	f38ff0ef          	jal	ra,b62 <wdt_close>
    142e:	2039                	c.jal	143c <c_startup>
    1430:	208d                	c.jal	1492 <system_init>
    1432:	2849                	c.jal	14c4 <__libc_init_array>
    1434:	fd6ff0ef          	jal	ra,c0a <boot_main>
    1438:	a62d                	c.j	1762 <__riscv_restore_0>

0000143a <__platform_init>:
    143a:	8082                	c.jr	ra

0000143c <c_startup>:
    143c:	302002ef          	jal	t0,173e <__riscv_save_0>
    1440:	00000797          	auipc	a5,0x0
    1444:	73078793          	addi	a5,a5,1840 # 1b70 <_itcm_lma_end>
    1448:	00000597          	auipc	a1,0x0
    144c:	72858593          	addi	a1,a1,1832 # 1b70 <_itcm_lma_end>
    1450:	40b78633          	sub	a2,a5,a1
    1454:	00b78763          	beq	a5,a1,1462 <c_startup+0x26>
    1458:	0ffff517          	auipc	a0,0xffff
    145c:	ba850513          	addi	a0,a0,-1112 # 10000000 <ITCM_BEGIN>
    1460:	2e01                	c.jal	1770 <memcpy>
    1462:	3000f417          	auipc	s0,0x3000f
    1466:	c9e40413          	addi	s0,s0,-866 # 30010100 <uart0_rx_index>
    146a:	3000f517          	auipc	a0,0x3000f
    146e:	b9650513          	addi	a0,a0,-1130 # 30010000 <SRAM2_BEGIN>
    1472:	40a40633          	sub	a2,s0,a0
    1476:	00000597          	auipc	a1,0x0
    147a:	70a58593          	addi	a1,a1,1802 # 1b80 <_data_lmastart>
    147e:	2ccd                	c.jal	1770 <memcpy>
    1480:	30011617          	auipc	a2,0x30011
    1484:	28860613          	addi	a2,a2,648 # 30012708 <_end>
    1488:	8e01                	c.sub	a2,s0
    148a:	4581                	c.li	a1,0
    148c:	8522                	c.mv	a0,s0
    148e:	2e9d                	c.jal	1804 <memset>
    1490:	acc9                	c.j	1762 <__riscv_restore_0>

00001492 <system_init>:
    1492:	fffff717          	auipc	a4,0xfffff
    1496:	b6e70713          	addi	a4,a4,-1170 # 0 <NDS_SAG_LMA_bootloader>
    149a:	f01007b7          	lui	a5,0xf0100
    149e:	cbb8                	c.sw	a4,80(a5)
    14a0:	7d0022f3          	csrr	t0,mmisc_ctl
    14a4:	0012fc5b          	bbc	t0,1,14bc <system_init+0x2a>
    14a8:	e4000337          	lui	t1,0xe4000
    14ac:	438d                	c.li	t2,3
    14ae:	14000513          	li	a0,320
    14b2:	00732023          	sw	t2,0(t1) # e4000000 <_stack+0xb3fc0000>
    14b6:	7d0527f3          	csrrs	a5,mmisc_ctl,a0
    14ba:	8082                	c.jr	ra
    14bc:	e4000337          	lui	t1,0xe4000
    14c0:	4385                	c.li	t2,1
    14c2:	b7f5                	c.j	14ae <system_init+0x1c>

000014c4 <__libc_init_array>:
    14c4:	27a002ef          	jal	t0,173e <__riscv_save_0>
    14c8:	3000f497          	auipc	s1,0x3000f
    14cc:	c3048493          	addi	s1,s1,-976 # 300100f8 <__fini_array_end>
    14d0:	3000f417          	auipc	s0,0x3000f
    14d4:	c2840413          	addi	s0,s0,-984 # 300100f8 <__fini_array_end>
    14d8:	408482b3          	sub	t0,s1,s0
    14dc:	4022d493          	srai	s1,t0,0x2
    14e0:	4901                	c.li	s2,0
    14e2:	02991263          	bne	s2,s1,1506 <__libc_init_array+0x42>
    14e6:	3000f417          	auipc	s0,0x3000f
    14ea:	c1240413          	addi	s0,s0,-1006 # 300100f8 <__fini_array_end>
    14ee:	3000f317          	auipc	t1,0x3000f
    14f2:	c0a30313          	addi	t1,t1,-1014 # 300100f8 <__fini_array_end>
    14f6:	408303b3          	sub	t2,t1,s0
    14fa:	4023d493          	srai	s1,t2,0x2
    14fe:	4901                	c.li	s2,0
    1500:	00991863          	bne	s2,s1,1510 <__libc_init_array+0x4c>
    1504:	acb9                	c.j	1762 <__riscv_restore_0>
    1506:	4008                	c.lw	a0,0(s0)
    1508:	0905                	c.addi	s2,1
    150a:	0411                	c.addi	s0,4
    150c:	9502                	c.jalr	a0
    150e:	bfd1                	c.j	14e2 <__libc_init_array+0x1e>
    1510:	401c                	c.lw	a5,0(s0)
    1512:	0905                	c.addi	s2,1
    1514:	0411                	c.addi	s0,4
    1516:	9782                	c.jalr	a5
    1518:	b7e5                	c.j	1500 <__libc_init_array+0x3c>

0000151a <mtime_handler>:
    151a:	08000793          	li	a5,128
    151e:	3047b7f3          	csrrc	a5,mie,a5
    1522:	8082                	c.jr	ra

00001524 <mswi_handler>:
    1524:	304477f3          	csrrci	a5,mie,8
    1528:	8082                	c.jr	ra

0000152a <syscall_handler>:
    152a:	8082                	c.jr	ra

0000152c <except_handler>:
    152c:	852e                	c.mv	a0,a1
    152e:	8082                	c.jr	ra

00001530 <trap_entry>:
    1530:	715d                	c.addi16sp	sp,-80
    1532:	c686                	c.swsp	ra,76(sp)
    1534:	c496                	c.swsp	t0,72(sp)
    1536:	c29a                	c.swsp	t1,68(sp)
    1538:	c09e                	c.swsp	t2,64(sp)
    153a:	de22                	c.swsp	s0,60(sp)
    153c:	dc26                	c.swsp	s1,56(sp)
    153e:	da2a                	c.swsp	a0,52(sp)
    1540:	d82e                	c.swsp	a1,48(sp)
    1542:	d632                	c.swsp	a2,44(sp)
    1544:	d436                	c.swsp	a3,40(sp)
    1546:	d23a                	c.swsp	a4,36(sp)
    1548:	d03e                	c.swsp	a5,32(sp)
    154a:	ce42                	c.swsp	a6,28(sp)
    154c:	cc46                	c.swsp	a7,24(sp)
    154e:	ca72                	c.swsp	t3,20(sp)
    1550:	c876                	c.swsp	t4,16(sp)
    1552:	c67a                	c.swsp	t5,12(sp)
    1554:	c47e                	c.swsp	t6,8(sp)
    1556:	342027f3          	csrr	a5,mcause
    155a:	34102473          	csrr	s0,mepc
    155e:	300024f3          	csrr	s1,mstatus
    1562:	0807d463          	bgez	a5,15ea <trap_entry+0xba>
    1566:	7807a75b          	bfoz	a4,a5,30,0
    156a:	04b7685b          	bnec	a4,11,15ba <trap_entry+0x8a>
    156e:	f14020f3          	csrr	ra,mhartid
    1572:	e42008b7          	lui	a7,0xe4200
    1576:	00c09813          	slli	a6,ra,0xc
    157a:	00488e13          	addi	t3,a7,4 # e4200004 <_stack+0xb41c0004>
    157e:	01c80eb3          	add	t4,a6,t3
    1582:	000ea503          	lw	a0,0(t4) # 80000000 <_stack+0x4ffc0000>
    1586:	2a91                	c.jal	16da <mext_interrupt>
    1588:	30049073          	csrw	mstatus,s1
    158c:	34141073          	csrw	mepc,s0
    1590:	5472                	c.lwsp	s0,60(sp)
    1592:	40b6                	c.lwsp	ra,76(sp)
    1594:	42a6                	c.lwsp	t0,72(sp)
    1596:	4316                	c.lwsp	t1,68(sp)
    1598:	4386                	c.lwsp	t2,64(sp)
    159a:	54e2                	c.lwsp	s1,56(sp)
    159c:	5552                	c.lwsp	a0,52(sp)
    159e:	55c2                	c.lwsp	a1,48(sp)
    15a0:	5632                	c.lwsp	a2,44(sp)
    15a2:	56a2                	c.lwsp	a3,40(sp)
    15a4:	5712                	c.lwsp	a4,36(sp)
    15a6:	5782                	c.lwsp	a5,32(sp)
    15a8:	4872                	c.lwsp	a6,28(sp)
    15aa:	48e2                	c.lwsp	a7,24(sp)
    15ac:	4e52                	c.lwsp	t3,20(sp)
    15ae:	4ec2                	c.lwsp	t4,16(sp)
    15b0:	4f32                	c.lwsp	t5,12(sp)
    15b2:	4fa2                	c.lwsp	t6,8(sp)
    15b4:	6161                	c.addi16sp	sp,80
    15b6:	30200073          	mret
    15ba:	0077645b          	bnec	a4,7,15c2 <trap_entry+0x92>
    15be:	3fb1                	c.jal	151a <mtime_handler>
    15c0:	b7e1                	c.j	1588 <trap_entry+0x58>
    15c2:	02376e5b          	bnec	a4,3,15fe <trap_entry+0xce>
    15c6:	3fb9                	c.jal	1524 <mswi_handler>
    15c8:	f14022f3          	csrr	t0,mhartid
    15cc:	00128313          	addi	t1,t0,1
    15d0:	f14023f3          	csrr	t2,mhartid
    15d4:	e66006b7          	lui	a3,0xe6600
    15d8:	00c39513          	slli	a0,t2,0xc
    15dc:	00468593          	addi	a1,a3,4 # e6600004 <_stack+0xb65c0004>
    15e0:	00b50633          	add	a2,a0,a1
    15e4:	00662023          	sw	t1,0(a2)
    15e8:	b745                	c.j	1588 <trap_entry+0x58>
    15ea:	00b7ea5b          	bnec	a5,11,15fe <trap_entry+0xce>
    15ee:	8736                	c.mv	a4,a3
    15f0:	86b2                	c.mv	a3,a2
    15f2:	862e                	c.mv	a2,a1
    15f4:	85aa                	c.mv	a1,a0
    15f6:	8546                	c.mv	a0,a7
    15f8:	3f0d                	c.jal	152a <syscall_handler>
    15fa:	0411                	c.addi	s0,4
    15fc:	b771                	c.j	1588 <trap_entry+0x58>
    15fe:	85a2                	c.mv	a1,s0
    1600:	853e                	c.mv	a0,a5
    1602:	372d                	c.jal	152c <except_handler>
    1604:	842a                	c.mv	s0,a0
    1606:	b749                	c.j	1588 <trap_entry+0x58>

00001608 <default_irq_handler>:
    1608:	8082                	c.jr	ra

0000160a <uart_irq_handler>:
    160a:	c85fe06f          	j	28e <UART0_InterruptHandler>

0000160e <uart1_irq_handler>:
    160e:	e25fe06f          	j	432 <UART1_InterruptHandler>

00001612 <uart2_irq_handler>:
    1612:	f0ffe06f          	j	520 <UART2_InterruptHandler>

00001616 <arch_interrupt_ctrl>:
    1616:	1141                	c.addi	sp,-16
    1618:	c602                	c.swsp	zero,12(sp)
    161a:	0615635b          	bnec	a0,1,1680 <arch_interrupt_ctrl+0x6a>
    161e:	03f00e13          	li	t3,63
    1622:	e40008b7          	lui	a7,0xe4000
    1626:	4605                	c.li	a2,1
    1628:	e4002837          	lui	a6,0xe4002
    162c:	c62a                	c.swsp	a0,12(sp)
    162e:	46b2                	c.lwsp	a3,12(sp)
    1630:	00de7b63          	bgeu	t3,a3,1646 <arch_interrupt_ctrl+0x30>
    1634:	6505                	c.lui	a0,0x1
    1636:	80050593          	addi	a1,a0,-2048 # 800 <flash_set_line_mode+0x20>
    163a:	3045a7f3          	csrrs	a5,mie,a1
    163e:	300467f3          	csrrsi	a5,mstatus,8
    1642:	0141                	c.addi	sp,16
    1644:	8082                	c.jr	ra
    1646:	4732                	c.lwsp	a4,12(sp)
    1648:	0ce88edb          	lea.w	t4,a7,a4
    164c:	00cea023          	sw	a2,0(t4)
    1650:	4f32                	c.lwsp	t5,12(sp)
    1652:	f1402ff3          	csrr	t6,mhartid
    1656:	005f5793          	srli	a5,t5,0x5
    165a:	0cf802db          	lea.w	t0,a6,a5
    165e:	007f9313          	slli	t1,t6,0x7
    1662:	006283b3          	add	t2,t0,t1
    1666:	0003a683          	lw	a3,0(t2)
    166a:	01e61533          	sll	a0,a2,t5
    166e:	00d565b3          	or	a1,a0,a3
    1672:	00b3a023          	sw	a1,0(t2)
    1676:	4732                	c.lwsp	a4,12(sp)
    1678:	00170e93          	addi	t4,a4,1
    167c:	c676                	c.swsp	t4,12(sp)
    167e:	bf45                	c.j	162e <arch_interrupt_ctrl+0x18>
    1680:	6785                	c.lui	a5,0x1
    1682:	88878293          	addi	t0,a5,-1912 # 888 <flash_erase_block+0xa>
    1686:	3042b7f3          	csrrc	a5,mie,t0
    168a:	300477f3          	csrrci	a5,mstatus,8
    168e:	4305                	c.li	t1,1
    1690:	03f00613          	li	a2,63
    1694:	e40025b7          	lui	a1,0xe4002
    1698:	4505                	c.li	a0,1
    169a:	c61a                	c.swsp	t1,12(sp)
    169c:	43b2                	c.lwsp	t2,12(sp)
    169e:	00767563          	bgeu	a2,t2,16a8 <arch_interrupt_ctrl+0x92>
    16a2:	34205073          	csrwi	mcause,0
    16a6:	bf71                	c.j	1642 <arch_interrupt_ctrl+0x2c>
    16a8:	4832                	c.lwsp	a6,12(sp)
    16aa:	f14026f3          	csrr	a3,mhartid
    16ae:	00585713          	srli	a4,a6,0x5
    16b2:	0ce588db          	lea.w	a7,a1,a4
    16b6:	00769e13          	slli	t3,a3,0x7
    16ba:	01c88eb3          	add	t4,a7,t3
    16be:	000eaf03          	lw	t5,0(t4)
    16c2:	01051fb3          	sll	t6,a0,a6
    16c6:	ffffc793          	not	a5,t6
    16ca:	01e7f2b3          	and	t0,a5,t5
    16ce:	005ea023          	sw	t0,0(t4)
    16d2:	4332                	c.lwsp	t1,12(sp)
    16d4:	8430                	exec.it	#19     !addi	t2,t1,1
    16d6:	c61e                	c.swsp	t2,12(sp)
    16d8:	b7d1                	c.j	169c <arch_interrupt_ctrl+0x86>

000016da <mext_interrupt>:
    16da:	064002ef          	jal	t0,173e <__riscv_save_0>
    16de:	00000797          	auipc	a5,0x0
    16e2:	3ae78793          	addi	a5,a5,942 # 1a8c <irq_handler>
    16e6:	0ca780db          	lea.w	ra,a5,a0
    16ea:	0000a303          	lw	t1,0(ra) # 2000 <_data_lmastart+0x480>
    16ee:	842a                	c.mv	s0,a0
    16f0:	86a1a223          	sw	a0,-1948(gp) # 3001015c <g_irq_source>
    16f4:	9302                	c.jalr	t1
    16f6:	f14022f3          	csrr	t0,mhartid
    16fa:	e4200737          	lui	a4,0xe4200
    16fe:	00c29393          	slli	t2,t0,0xc
    1702:	00470513          	addi	a0,a4,4 # e4200004 <_stack+0xb41c0004>
    1706:	00a385b3          	add	a1,t2,a0
    170a:	c180                	c.sw	s0,0(a1)
    170c:	a899                	c.j	1762 <__riscv_restore_0>

0000170e <__riscv_save_12>:
    170e:	7139                	c.addi16sp	sp,-64
    1710:	4301                	c.li	t1,0
    1712:	c66e                	c.swsp	s11,12(sp)
    1714:	a019                	c.j	171a <__riscv_save_10+0x4>

00001716 <__riscv_save_10>:
    1716:	7139                	c.addi16sp	sp,-64
    1718:	5341                	c.li	t1,-16
    171a:	c86a                	c.swsp	s10,16(sp)
    171c:	ca66                	c.swsp	s9,20(sp)
    171e:	cc62                	c.swsp	s8,24(sp)
    1720:	ce5e                	c.swsp	s7,28(sp)
    1722:	a019                	c.j	1728 <__riscv_save_4+0x4>

00001724 <__riscv_save_4>:
    1724:	7139                	c.addi16sp	sp,-64
    1726:	5301                	c.li	t1,-32
    1728:	d05a                	c.swsp	s6,32(sp)
    172a:	d256                	c.swsp	s5,36(sp)
    172c:	d452                	c.swsp	s4,40(sp)
    172e:	d64e                	c.swsp	s3,44(sp)
    1730:	d84a                	c.swsp	s2,48(sp)
    1732:	da26                	c.swsp	s1,52(sp)
    1734:	dc22                	c.swsp	s0,56(sp)
    1736:	de06                	c.swsp	ra,60(sp)
    1738:	40610133          	sub	sp,sp,t1
    173c:	8282                	c.jr	t0

0000173e <__riscv_save_0>:
    173e:	1141                	c.addi	sp,-16
    1740:	c04a                	c.swsp	s2,0(sp)
    1742:	c226                	c.swsp	s1,4(sp)
    1744:	c422                	c.swsp	s0,8(sp)
    1746:	c606                	c.swsp	ra,12(sp)
    1748:	8282                	c.jr	t0

0000174a <__riscv_restore_12>:
    174a:	4db2                	c.lwsp	s11,12(sp)
    174c:	0141                	c.addi	sp,16

0000174e <__riscv_restore_10>:
    174e:	4d02                	c.lwsp	s10,0(sp)
    1750:	4c92                	c.lwsp	s9,4(sp)
    1752:	4c22                	c.lwsp	s8,8(sp)
    1754:	4bb2                	c.lwsp	s7,12(sp)
    1756:	0141                	c.addi	sp,16

00001758 <__riscv_restore_4>:
    1758:	4b02                	c.lwsp	s6,0(sp)
    175a:	4a92                	c.lwsp	s5,4(sp)
    175c:	4a22                	c.lwsp	s4,8(sp)
    175e:	49b2                	c.lwsp	s3,12(sp)
    1760:	0141                	c.addi	sp,16

00001762 <__riscv_restore_0>:
    1762:	4902                	c.lwsp	s2,0(sp)
    1764:	4492                	c.lwsp	s1,4(sp)
    1766:	4422                	c.lwsp	s0,8(sp)
    1768:	40b2                	c.lwsp	ra,12(sp)
    176a:	0141                	c.addi	sp,16
    176c:	8082                	c.jr	ra
    176e:	0092                	c.slli	ra,0x4

00001770 <memcpy>:
    1770:	00c583b3          	add	t2,a1,a2
    1774:	82aa                	c.mv	t0,a0
    1776:	0035f713          	andi	a4,a1,3
    177a:	00357793          	andi	a5,a0,3
    177e:	06f71d63          	bne	a4,a5,17f8 <memcpy+0x88>
    1782:	c30d                	c.beqz	a4,17a4 <memcpy+0x34>
    1784:	ffc78793          	addi	a5,a5,-4
    1788:	40f58333          	sub	t1,a1,a5
    178c:	0663c663          	blt	t2,t1,17f8 <memcpy+0x88>
    1790:	0005c683          	lbu	a3,0(a1) # e4002000 <_stack+0xb3fc2000>
    1794:	0585                	c.addi	a1,1
    1796:	00d50023          	sb	a3,0(a0)
    179a:	0505                	c.addi	a0,1
    179c:	fe659ae3          	bne	a1,t1,1790 <memcpy+0x20>
    17a0:	04b38f63          	beq	t2,a1,17fe <memcpy+0x8e>
    17a4:	963e                	c.add	a2,a5
    17a6:	9a01                	c.andi	a2,-32
    17a8:	02060a63          	beqz	a2,17dc <memcpy+0x6c>
    17ac:	00c58333          	add	t1,a1,a2
    17b0:	499c                	c.lw	a5,16(a1)
    17b2:	49d8                	c.lw	a4,20(a1)
    17b4:	4d94                	c.lw	a3,24(a1)
    17b6:	4dd0                	c.lw	a2,28(a1)
    17b8:	c91c                	c.sw	a5,16(a0)
    17ba:	c958                	c.sw	a4,20(a0)
    17bc:	cd14                	c.sw	a3,24(a0)
    17be:	cd50                	c.sw	a2,28(a0)
    17c0:	419c                	c.lw	a5,0(a1)
    17c2:	41d8                	c.lw	a4,4(a1)
    17c4:	4594                	c.lw	a3,8(a1)
    17c6:	45d0                	c.lw	a2,12(a1)
    17c8:	c11c                	c.sw	a5,0(a0)
    17ca:	c158                	c.sw	a4,4(a0)
    17cc:	c514                	c.sw	a3,8(a0)
    17ce:	c550                	c.sw	a2,12(a0)
    17d0:	02058593          	addi	a1,a1,32
    17d4:	02050513          	addi	a0,a0,32
    17d8:	fc659ce3          	bne	a1,t1,17b0 <memcpy+0x40>
    17dc:	ffc3f313          	andi	t1,t2,-4
    17e0:	00b36c63          	bltu	t1,a1,17f8 <memcpy+0x88>
    17e4:	00458693          	addi	a3,a1,4
    17e8:	00d36863          	bltu	t1,a3,17f8 <memcpy+0x88>
    17ec:	4194                	c.lw	a3,0(a1)
    17ee:	0591                	c.addi	a1,4
    17f0:	c114                	c.sw	a3,0(a0)
    17f2:	0511                	c.addi	a0,4
    17f4:	fe659ce3          	bne	a1,t1,17ec <memcpy+0x7c>
    17f8:	831e                	c.mv	t1,t2
    17fa:	f8b39be3          	bne	t2,a1,1790 <memcpy+0x20>
    17fe:	8516                	c.mv	a0,t0
    1800:	8082                	c.jr	ra
    1802:	0001                	c.nop

00001804 <memset>:
    1804:	832a                	c.mv	t1,a0
    1806:	00c503b3          	add	t2,a0,a2
    180a:	00357693          	andi	a3,a0,3
    180e:	ce81                	c.beqz	a3,1826 <memset+0x22>
    1810:	ffc68693          	addi	a3,a3,-4
    1814:	40d507b3          	sub	a5,a0,a3
    1818:	06f3e463          	bltu	t2,a5,1880 <memset+0x7c>
    181c:	00b50023          	sb	a1,0(a0)
    1820:	0505                	c.addi	a0,1
    1822:	fea79de3          	bne	a5,a0,181c <memset+0x18>
    1826:	04750d63          	beq	a0,t2,1880 <memset+0x7c>
    182a:	05e2                	c.slli	a1,0x18
    182c:	0085d793          	srli	a5,a1,0x8
    1830:	95be                	c.add	a1,a5
    1832:	0105d793          	srli	a5,a1,0x10
    1836:	95be                	c.add	a1,a5
    1838:	40a38633          	sub	a2,t2,a0
    183c:	02000713          	li	a4,32
    1840:	fe067693          	andi	a3,a2,-32
    1844:	00a687b3          	add	a5,a3,a0
    1848:	e285                	c.bnez	a3,1868 <memset+0x64>
    184a:	ffc67693          	andi	a3,a2,-4
    184e:	ca8d                	c.beqz	a3,1880 <memset+0x7c>
    1850:	40d70633          	sub	a2,a4,a3
    1854:	00a687b3          	add	a5,a3,a0
    1858:	8736                	c.mv	a4,a3
    185a:	8205                	c.srli	a2,0x1
    185c:	00000697          	auipc	a3,0x0
    1860:	00c686b3          	add	a3,a3,a2
    1864:	00c68067          	jr	12(a3) # 1868 <memset+0x64>
    1868:	cd4c                	c.sw	a1,28(a0)
    186a:	cd0c                	c.sw	a1,24(a0)
    186c:	c94c                	c.sw	a1,20(a0)
    186e:	c90c                	c.sw	a1,16(a0)
    1870:	c54c                	c.sw	a1,12(a0)
    1872:	c50c                	c.sw	a1,8(a0)
    1874:	c14c                	c.sw	a1,4(a0)
    1876:	c10c                	c.sw	a1,0(a0)
    1878:	953a                	c.add	a0,a4
    187a:	fef517e3          	bne	a0,a5,1868 <memset+0x64>
    187e:	bf6d                	c.j	1838 <memset+0x34>
    1880:	879e                	c.mv	a5,t2
    1882:	f8751de3          	bne	a0,t2,181c <memset+0x18>
    1886:	851a                	c.mv	a0,t1
    1888:	8082                	c.jr	ra
    188a:	0092                	c.slli	ra,0x4

Disassembly of section .itcm_write_flash:

0000188c <bl_hw_board_deinit>:
    188c:	00000317          	auipc	t1,0x0
    1890:	eb2302e7          	jalr	t0,-334(t1) # 173e <__riscv_save_0>
    1894:	fffff097          	auipc	ra,0xfffff
    1898:	d44080e7          	jalr	-700(ra) # 5d8 <uart_deinit>
    189c:	4501                	c.li	a0,0
    189e:	00000097          	auipc	ra,0x0
    18a2:	d78080e7          	jalr	-648(ra) # 1616 <arch_interrupt_ctrl>
    18a6:	00000317          	auipc	t1,0x0
    18aa:	ebc30067          	jr	-324(t1) # 1762 <__riscv_restore_0>

000018ae <bl_hw_board_init>:
    18ae:	00000317          	auipc	t1,0x0
    18b2:	e90302e7          	jalr	t0,-368(t1) # 173e <__riscv_save_0>
    18b6:	fffff097          	auipc	ra,0xfffff
    18ba:	d0a080e7          	jalr	-758(ra) # 5c0 <uart_init>
    18be:	4509                	c.li	a0,2
    18c0:	fffff097          	auipc	ra,0xfffff
    18c4:	f20080e7          	jalr	-224(ra) # 7e0 <flash_set_line_mode>
    18c8:	4515                	c.li	a0,5
    18ca:	fffff097          	auipc	ra,0xfffff
    18ce:	f56080e7          	jalr	-170(ra) # 820 <flash_set_clk>
    18d2:	fffff097          	auipc	ra,0xfffff
    18d6:	dd6080e7          	jalr	-554(ra) # 6a8 <get_flash_ID>
    18da:	fffff097          	auipc	ra,0xfffff
    18de:	d96080e7          	jalr	-618(ra) # 670 <flash_get_current_flash_config>
    18e2:	4505                	c.li	a0,1
    18e4:	00000097          	auipc	ra,0x0
    18e8:	d32080e7          	jalr	-718(ra) # 1616 <arch_interrupt_ctrl>
    18ec:	00000317          	auipc	t1,0x0
    18f0:	e7630067          	jr	-394(t1) # 1762 <__riscv_restore_0>

Disassembly of section .exec.itable:

000018f4 <_ITB_BASE_>:
    18f4:	44030737          	lui	a4,0x44030
    18f8:	458307b7          	lui	a5,0x45830
    18fc:	7580106f          	j	3054 <SRAM2_SIZE+0x94c>
    1900:	0008ae03          	lw	t3,0(a7) # e4000000 <_stack+0xb3fc0000>
    1904:	0057a023          	sw	t0,0(a5) # 45830000 <_stack+0x157f0000>
    1908:	73e012ef          	jal	t0,3046 <SRAM2_SIZE+0x93e>
    190c:	448207b7          	lui	a5,0x44820
    1910:	7620106f          	j	3072 <SRAM2_SIZE+0x96a>
    1914:	724012ef          	jal	t0,3038 <SRAM2_SIZE+0x930>
    1918:	0004a083          	lw	ra,0(s1)
    191c:	0002a303          	lw	t1,0(t0)
    1920:	00044503          	lbu	a0,0(s0)
    1924:	0134a023          	sw	s3,0(s1)
    1928:	440302b7          	lui	t0,0x44030
    192c:	00d373b3          	and	t2,t1,a3
    1930:	400006b7          	lui	a3,0x40000
    1934:	10000613          	li	a2,256
    1938:	00032383          	lw	t2,0(t1)
    193c:	54b000ef          	jal	ra,2686 <_data_lmastart+0xb06>
    1940:	00130393          	addi	t2,t1,1
    1944:	440307b7          	lui	a5,0x44030
    1948:	04247293          	andi	t0,s0,66
    194c:	12c00513          	li	a0,300
