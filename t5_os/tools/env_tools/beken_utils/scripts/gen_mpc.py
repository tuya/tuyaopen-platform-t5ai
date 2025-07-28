#!/usr/bin/env python3

import logging
from .mpc import *
from .gen_license import get_license
from .common import *

def empty_line(f):
    line = f'\n'
    f.write(line)

def gen_mpc(device, block_size, f, sector_num, mpc):
    generator = (dict for dict in mpc.csv.dic_list if dict["Device"] == device)
    count  = len(list(generator))
    sector_num.append(count)
    line = f'const static int %s[%d][3] = {{' % (device, count)
    f.write(line)
    end = 0
    i = 0
    for dict in mpc.csv.dic_list:
        if dict["Device"] == device:
            start = int(dict["Offset"],16)
            if(start % block_size != 0):
                logging.error("address not aligned!\n")
                exit("check mpc.csv\n")
            if (start < end):
                logging.error("address overlapped!\n")
                exit("check mpc.csv\n")
            end = int(dict["Offset"],16)+ size2int(dict["Size"])
            size = size2int(dict["Size"])
            if(dict["Secure"] == "TRUE"):
                sec = 0 #secure value 0
            else:
                sec = 1 #non secure value 1
            line = f'{{0x%x,%d,%d}},' %(start , size / block_size, sec)
            f.write(line)
            i+=1
    line = f'}};\n'
    f.write(line)

def gen_mpc_config_file(mpc_csv='mpc.csv', outfile='_mpc.h'):
    mpc = MPC(mpc_csv)

    logging.debug(f'Create {outfile}')
    f = open(outfile, 'w+')
    f.write(get_license())

    #TODO
    return

    sector_num = []
    gen_mpc("psram", 256*1024, f, sector_num, mpc)
    gen_mpc("qspi0", 256*1024, f, sector_num, mpc)
    gen_mpc("qspi1", 256*1024, f, sector_num, mpc)
    gen_mpc("otp2", 256, f, sector_num, mpc)
    gen_mpc("flash", 64*1024, f, sector_num, mpc)
    gen_mpc("mem0", 4*1024, f, sector_num, mpc)
    gen_mpc("mem1", 4*1024, f, sector_num, mpc)
    gen_mpc("mem2", 4*1024, f, sector_num, mpc)
    gen_mpc("mem3", 4*1024, f, sector_num, mpc)
    gen_mpc("mem4", 4*1024, f, sector_num, mpc)
    gen_mpc("mem5", 4*1024, f, sector_num, mpc)

    empty_line(f)
    line = f'typedef struct {{\n'
    f.write(line)
    line = f'    int dev_num;\n'
    f.write(line)
    line = f'    const int* pointer;\n'
    f.write(line)
    line = f'    int sector_num;\n'
    f.write(line)
    line = f'}} dev_security_t;\n'
    f.write(line)
    empty_line(f)

    list = ["psram","qspi0","qspi1","otp2","flash","mem0","mem1","mem2","mem3","mem4","mem5"]
    line = f'const static dev_security_t mpc_security_table[11] = {{\n'
    f.write(line)
    for i in range(11):
        line = f'    {{%d, *%s, %d}},\n'%(i,list[i],sector_num[i])
        f.write(line)
    line = f'}};\n'
    f.write(line)
    f.close()
