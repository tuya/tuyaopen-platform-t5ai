#!/usr/bin/env python3

import logging
from .security import *
from .gen_license import get_license
from .common import *

s_phy2code_start = "\
#define FLASH_CEIL_ALIGN(v, align) ((((v) + ((align) - 1)) / (align)) * (align))\n\
#define FLASH_PHY2VIRTUAL_CODE_START(phy_addr) FLASH_CEIL_ALIGN(FLASH_PHY2VIRTUAL(FLASH_CEIL_ALIGN((phy_addr), 34)), CPU_VECTOR_ALIGN_SZ)\n\
"

def gen_security_config_file(security_csv, outfile):
    security = Security(security_csv)
    f = open(outfile, 'w+')
    logging.debug(f'Create {outfile}')
    f.write(get_license())

    line = f'#include "_ota.h"\n'
    f.write(line)
    line = f'#include "_ppc.h"\n'
    f.write(line)
    line = f'#include "_mpc.h"\n'
    f.write(line)

    line = f'#undef %-45s\n' %("MCUBOOT_SIGN_RSA")
    f.write(line)
    if (security.secureboot_en):
        line = f'#define %-45s %d\n' %("CONFIG_SECUREBOOT", 1)
        f.write(line)

    if (security.bl2_root_key_type == 'rsa2048'):
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA", "1\n")
        f.write(line)
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA_LEN", "2048\n")
        f.write(line)
    elif (security.bl2_root_key_type == 'rsa3072'):
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA", "1\n")
        f.write(line)
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_RSA_LEN", "3072\n")
        f.write(line)
    elif (security.bl2_root_key_type == 'ec256'):
        line = f'#define %-45s %s' %("MCUBOOT_SIGN_EC256", "1\n")
        f.write(line)
    else:
        logging.error(f'unsupported bl2 key type\n')
        exit(1)

    macro_name = f'CONFIG_CODE_ENCRYPTED'
    if security.is_flash_aes_fixed():
        line = f'#define %-45s %d\n' %(macro_name, 1)
    elif security.is_flash_aes_random():
        line = f'#define %-45s %d\n' %(macro_name, 2)
    else:
        line = f'#define %-45s %d\n' %(macro_name, 0)

    f.write(line)
    line = f'#define %-45s %s' %("FLASH_VIRTUAL2PHY(virtual_addr)", "((((virtual_addr) >> 5) * 34) + ((virtual_addr) & 31))\n")
    f.write(line)
    line = f'#define %-45s %s' %("FLASH_PHY2VIRTUAL(phy_addr)", "((((phy_addr) / 34) << 5) + ((phy_addr) % 34))\n")
    f.write(line)
    line = f'#define %-45s %s' %("CEIL_ALIGN_34(addr)", "(((addr) + 34 - 1) / 34 * 34)\n")
    f.write(line)

    empty_line(f)
    f.write(s_phy2code_start)
    empty_line(f)
