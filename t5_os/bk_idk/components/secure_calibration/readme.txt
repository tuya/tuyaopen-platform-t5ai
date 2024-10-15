1、编译命令 :make clean;make

2、生成的all.bin路径： package_for_calibration/output/all.bin	（直接烧录这个镜像文件）

note:
	
	1）aes_bootloader_crc.bin 路径：package_for_calibration/Linux_version
	
	2）bootloader.bin的路径 ：package_for_calibration/Linux_version
	
	3）库上版本：将通过数据口的方式将加密的aes_bootloader_crc.bin写到flash的x11000位置；
	
	4）库上版本：可在路径下：calibration/boot/boot_v1/bootloader.c 里面更改 将宏定义NORMAL_BOOTLOADER_PHY_START_ADDR = 0x0；将加密的aes_bootloader_crc.bin写到0x0位置；
	
	5）库上版本：bootloader.bin跳转地址是写死的 0x20f0000(B 区起始地址)；
	6) 库上原来的测试代码被关闭，在calibration/build/mk/config.mk中可自行修改 TE200_UT =Y
	
3、目前，psa的加解密流程已调通（填充方式是 MBEDTLS_RSA_PKCS_V15，模数位 2048）

