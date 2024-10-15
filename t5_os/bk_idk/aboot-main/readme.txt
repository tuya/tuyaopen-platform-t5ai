一、当前目录下生成bootloader.bin的步骤：
1）make clean
2）make
3）在PACK下运行generate.bat,在generate_out生成bootloader.bin以及bootloader_crc.bin;
	在bootloader_u下会同步生成u_bootloader.bin；在bootloader_l目录下同步生成l_bootloader.bin
