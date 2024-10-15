::.\cmake_Gen_image.exe genfile -injsonfile ./json/config.json -infile  bootloader.bin  -outfile ./output/bootloader.bin -genjson ./json/partition_bootloader.json
.\encrypt.exe ./output/bootloader.bin 00000000
.\cmake_Gen_image.exe genfile -injsonfile ./json/configuration.json -infile ./output/bootloader_crc.bin calibration_crc.bin aes_bootloader_crc.bin -outfile ./output/all.bin

pause