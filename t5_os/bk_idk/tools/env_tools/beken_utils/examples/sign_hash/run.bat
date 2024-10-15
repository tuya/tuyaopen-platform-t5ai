@echo off

python3 -B run.py --app_version 0.0.1 --app_security_counter 5 --ota_version 0.0.3 --ota_security_counter 5 --project security/secureboot_overwrite --clean --build
