#
# Copyright (c) 2012-2021 Andes Technology Corporation
# All rights reserved.
#

make_dir := $(shell pwd)
make_clr_l :=  $(shell pwd)/l_boot
make_clr_up := $(shell pwd)/up_boot

subsystem:

	$(shell mkdir -p $(make_dir)/l_boot/out)
	$(shell mkdir -p $(make_dir)/l_boot/out/temp)
	$(shell mkdir -p $(make_dir)/up_boot/out)
	$(shell mkdir -p $(make_dir)/up_boot/out/temp)
	$(shell mkdir -p $(make_dir)/PACK/bootloader_l)
	$(shell mkdir -p $(make_dir)/PACK/bootloader_u)
	$(shell mkdir -p $(make_dir)/PACK/generate_out)
	cd $(make_dir)/l_boot && $(MAKE)
	cd $(make_dir)/up_boot && $(MAKE)
	

clean:
	@rm -f $(make_dir)/*.elf
	@rm -f $(make_dir)/*.bin
	@rm -f $(make_dir)/*.asm
	@rm -f $(make_dir)/*.map
	@rm -rf $(make_dir)/PACK/bootloader_l/*
	@rm -rf $(make_dir)/PACK/bootloader_u/*
	@rm -rf $(make_dir)/PACK/generate_out/*
	cd $(make_dir)/l_boot && make clean
	cd $(make_dir)/up_boot && make clean

	
