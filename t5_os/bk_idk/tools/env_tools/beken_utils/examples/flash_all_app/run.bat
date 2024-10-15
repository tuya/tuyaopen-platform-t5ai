@echo off

REM OTA XIP
python3 -B run.py --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type XIP --pubkey root_ec256_pubkey.pem --clean --build

REM OTA Overwrite
REM python3 -B run.py --flash_aes_key 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --ota_type OVERWRITE --privkey root_ec256_privkey.pem --pubkey root_ec256_pubkey.pem --clean --build
