#
# Copyright (c) 2012-2021 Andes Technology Corporation
# All rights reserved.
#

make_dir 			:= $(shell pwd)
make_dir_bl			:= $(shell pwd)/calibration
make_dir_out		:= $(make_dir)/out/debug/bk7236_v2022/ubl/bootrom
tool_dir			:= $(make_dir)/package_for_calibration/Linux_version
lg_output_dir		:= $(make_dir)/package_for_calibration/output
lg_input_base_dir	:= $(make_dir)/package_for_calibration

$(shell mkdir -p $(lg_output_dir))
#compile bootloader and monitor, and then packaged into one imgage.
subsystem:
	cd $(make_dir_bl)      && $(MAKE) -j32

	./cmake_encrypt_crc -crc $(make_dir_out)/bootrom.bin
	cp $(make_dir_out)/bootrom_crc.bin $(make_dir_out)/calibration_crc.bin
	cp $(make_dir_out)/calibration_crc.bin $(tool_dir)/calibration_crc.bin

	$(tool_dir)/tools/cmake_Gen_image genfile -injsonfile $(lg_input_base_dir)/json/configuration.json -infile $(tool_dir)/non_sec_bootloader.bin  $(tool_dir)/rsa_privkey_gen.bin  $(tool_dir)/aes_encrypt_key_content_gen.bin $(tool_dir)/calibration_crc.bin $(tool_dir)/bootloader.bin -outfile $(lg_output_dir)/all_calibartion.bin
clean:
	@rm -f $(make_dir)/*/*.elf
	@rm -f $(make_dir)/*/*.bin
	@rm -f $(make_dir)/*/*.asm
	@rm -f $(make_dir)/*/*.map

	cd $(make_dir_bl) && make clean

