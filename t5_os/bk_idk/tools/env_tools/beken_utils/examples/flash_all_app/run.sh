# Get sign slot size of secureboot_no_tfm
#../../main.py sign sign_args --partition_csv ./pack_server/immutable/partitions.csv --ota_type OVERWRITE

# Get sign slot size of secureboot_overwrite
#../../main.py sign sign_args --partition_csv ./pack_server/immutable/partitions.csv --ota_type OVERWRITE --bl1_secureboot_en

# secureboot_no_tfm, OTA XIP, BL1 secure boot disabled, flash fixed AES key.
#python3 -B run.py --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type XIP --privkey root_ec256_privkey.pem --pubkey root_ec256_pubkey.pem --app_slot_size 1605632 --project security/secureboot_no_tfm --clean --build
python3 -B run.py --flash_aes_type NONE --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type XIP --privkey root_ec256_privkey.pem --pubkey root_ec256_pubkey.pem --app_slot_size 1605632 --project security/secureboot_no_tfm --clean --build

# secureboot_no_tfm, OTA OVERWRITE, BL1 secure boot disabled, flash fixed AES key.
#python3 -B run.py --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type OVERWRITE --privkey root_ec256_privkey.pem --pubkey root_ec256_pubkey.pem --app_slot_size 1728512 --ota_slot_size 921600 --project security/secureboot_no_tfm --clean --build

# secureboot_overwrite, OTA OVERWRITE, BL1 secure boot enabled, flash RANDOM AES key.
# python3 -B run.py --flash_aes_type RANDOM --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type OVERWRITE --privkey root_ec256_privkey.pem --pubkey root_ec256_pubkey.pem --app_slot_size 2015232 --ota_slot_size 1048576 --project security/secureboot_overwrite --clean --bl1_secureboot_en --bl2_load_addr 0x02003e00 --build

# secureboot_overwrite, OTA OVERWRITE, BL1 secure boot enabled, flash FIXED AES key.
#python3 -B run.py --flash_aes_type FIXED --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type OVERWRITE --privkey root_ec256_privkey.pem --pubkey root_ec256_pubkey.pem --app_slot_size 2015232 --ota_slot_size 1048576 --project security/secureboot_overwrite --clean --bl1_secureboot_en --bl2_load_addr 0x02003e00 --build

#cp install/*.bin /win30/ming.liu/bk7236/tmp1
