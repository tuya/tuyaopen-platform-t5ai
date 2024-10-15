::.\Objects\fromelf -c  .\Objects\bk_bootloader_up.axf -o .\Objects\bkdump.txt
.\tools\com\link_t.exe .\bootloader_u\u_bootloader.bin .\bootloader_l\l_bootloader.bin
copy .\bootloader_u\u_bootloader_all.bin  .\generate_out\bootloader.bin
.\tools\rt_partition_tool_cli.exe .\generate_out\bootloader.bin .\tools\partition_bk7256.json
.\tools\encrypt.exe .\generate_out\bootloader.bin 00000000
del .\generate_out\bootloader.out
del .\bootloader_u\u_bootloader_all.bin
pause