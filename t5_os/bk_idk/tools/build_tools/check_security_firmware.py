#!/usr/bin/env python3

from os import listdir
from os.path import isdir,join
import sys
import re
import os

class CheckSecurity:
    def __init__(self, path):
        self.project_full_path = path
        self.secure_config_path = os.path.join(self.project_full_path,'config')
        print(f'===============path = {path}=======\r\n')


    def get_securiy_flag(self):
        is_secure_boot = False
        if os.path.exists(self.secure_config_path):
            with open(self.secure_config_path, 'r') as config_file:
                config_string = config_file.read()

            config_lines = config_string.strip().split('\n')

            config_dict = {}

            for line in config_lines:
                if '=' not in line:
                    continue
                key, value = line.split('=')
                config_dict[key.strip()] = value.strip()

            if config_dict.get('CONFIG_SECURITY_FIRMWARE') == 'y':
                is_secure_boot = True

        return is_secure_boot
    
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print('False')
        exit(1)
    else:
        check = CheckSecurity(sys.argv[1])
        if check.get_securiy_flag() == True:
            print('True')
            exit(0)
        else:
            print('False')
            exit(1)