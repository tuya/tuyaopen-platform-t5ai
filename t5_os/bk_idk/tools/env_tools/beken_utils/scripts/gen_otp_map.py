#!/usr/bin/env python3

import logging
import os
import glob
import re
from .otp import *
from .gen_license import get_license
from .common import *

def gen_otp_map_file():
    otp_pattern = 'otp[0-9]*.csv'
    otp_list = glob.glob(otp_pattern)

    if not otp_list:
        return

    otp_instance = []
    regex = re.compile(r'otp(\d+)\.csv')
    sorted_list = sorted(otp_list,key=lambda x:int(regex.search(x).group(1)) if regex.search(x) else -1) 
    for otp_csv in sorted_list:
        match = regex.search(otp_csv)
        number = match.group(1)
        otp = OTP(otp_csv, number)
        otp_instance.append(otp)

    f = open('_otp.h',"w+")
    logging.debug(f'Create _otp.h')

    f.write(get_license())

    f.write("\n#include <stdint.h>\n")

    f.write("\ntypedef enum{\n")
    f.write("    OTP_READ_WRITE = 0,\n")
    f.write("    OTP_READ_ONLY = 0x3,\n")
    f.write("    OTP_NO_ACCESS = 0xF,\n")
    f.write("} otp_privilege_t;\n")

    f.write("\ntypedef enum{\n")
    f.write("    OTP_SECURITY = 0,\n")
    f.write("    OTP_NON_SECURITY,\n")
    f.write("} otp_security_t;\n")

    f.write("\ntypedef struct\n")
    f.write("{\n")
    f.write("    uint32_t  name;\n")
    f.write("    uint32_t  allocated_size;\n")
    f.write("    uint32_t  offset;\n")
    f.write("    otp_privilege_t privilege;\n")
    f.write("    otp_security_t  security;\n")
    f.write("} otp_item_t;\n")

    max_row = 0
    for otp in otp_instance:
        f.write("\ntypedef enum{\n")
        if(len(otp.data) > max_row):
            max_row = len(otp.data)
        for id,item in otp.data.items():
            f.write('    ' + item['name'] + ','+ '\n')
        f.write(f"}} otp{otp.index}_id_t;\n")

    f.write(f"\nextern const otp_item_t otp_map[][{max_row}];\n")
    f.write(f"\nuint32_t otp_map_row(void);\n")
    f.write(f"\nuint32_t otp_map_col(void);\n")

    f.close()

    f = open("_otp.c","w+")

    logging.debug(f'Create _otp.c')
    f.write(get_license())
    f.write('\n#include "_otp.h"\n')

    f.write(f"\nconst otp_item_t otp_map[][{max_row}] = {{\n")

    for otp in otp_instance:
        f.write("{\n")
        max_len = max(len(item['name']) for id,item in otp.data.items())
        for id,item in otp.data.items():
            line = f'    {{%{-max_len-4}s %10d,    0x%x,    %s,    %s}},\n' \
              %((item['name']+','), item['size'], item['offset'],item['privilege'],item['security'])
            f.write(line)
        f.write("},\n")
    f.write("};\n")

    f.write(f"\nuint32_t otp_map_row(void)\n{{\n")
    f.write(f"    return sizeof(otp_map) / sizeof(otp_map[0]);\n}}\n")
    f.write(f"\nuint32_t otp_map_col(void)\n{{\n")
    f.write(f"    return sizeof(otp_map[0]) / sizeof(otp_map[0][0]);\n}}\n")


    f.close()