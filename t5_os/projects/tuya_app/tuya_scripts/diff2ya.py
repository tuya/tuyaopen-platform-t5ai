#!python3
# coding=utf-8

import lzma
import os
import struct
import sys
import requests
import traceback
import zipfile
from binascii import crc32
from enum import Enum
from optparse import OptionParser
from os.path import getsize
import tarfile
import filetype
import json
from dev_proc.zigbee.zigbee_patch_convert import Patch_convert
from dev_proc.cn_patch.mk_patch import make_cn_patch
from dev_proc.cn_patch.mk_patch import find_cn_bin
from dev_proc.nbiot.nb_patch import make_nb_patch
from dev_proc.nbiot.nb_patch import check_is_nbiot
from dev_proc.nbiot.nb_patch import check_is_xy1100

sys.path.append('..')
#import taskProcessRepo
from base import logFactory
from bsdiff4_2ya import format
from bsdiff4_2ya import globalvar

logging = logFactory.get()

class ErrCode(Enum):
    OK = 0				        #成功
    INVAL_PARAM_ERR = 1	        #参数错误
    MAGIC_ERR = 2			    #文件头magic校验错误
    MAX_FLASH_ERR = 3		    #max_flash长度设置错误
    MAKE_PATCH_ERR = 4	        #单区差分失败
    PATCH_LEN_OVER_ERR = 5	    #生成差分包过大
    ZONE_NUM_ERR = 6	        #多分区数量错误
    MAKE_PATCH_ERR_DOUBLE = 7	#双区差分失败
    REQUEST_BIN_ERR = 8	        #request下载失败
    REQUEST_POST_ERR = 9	    #request post进度失败
    UNSUPPORT_ERR = 10	        #不支持
    ZIGBEE_HEAD_ERR = 11	    #Zigbee头添加错误
    OTHER_PATCH_ERR = 12	    #其他平台差分错误
    UNKNOWN_ERR = 255	        #未知错误

class DiffProgress():
    task_id = 0
    all_part_num = 1
    part_num = 1
    progress = -1
    total_progress = -1

class VerCode:
    verName = "V1.2.0"
    verNum = 3
    verSuppMinDiffNum = 2

class MakeDiffRes:
    diff_code = ErrCode.OK
    str_io = None

class RequestDiffRst():
    taskId = -1
    taskRt = 0
    taskErrorCode = ""

class DiffIgnore():
    offest = 0
    ignore_num = 0
    diff_size = 0


def human_bytes(n):
    """
    return the number of bytes 'n' in more human readable form
    """
    if n < 1024:
        return '%i B' % n
    k = (n - 1) / 1024 + 1
    if k < 1024:
        return '%i KB' % k
    return '%.2f MB' % (float(n) / (2 ** 20))

def diff2ya_patch_make_double(src_data, dst_data, diff_progress):
    # bspatch
    raw_patch = format.diff_raw_double(src_data, dst_data, diff_progress)
    if raw_patch == -1:
        return raw_patch
    print('raw patch size:', len(raw_patch))

    # compress
    filterz =   [{  "id": lzma.FILTER_LZMA2,
                    "dict_size": 1024*4,
                    "depth": 0,
                    "mode": lzma.MODE_NORMAL,
                    "nice_len": 273,
                    "lc": 3,
                    "lp": 0,
                    "pb": 2,
                    "mf": lzma.MF_BT4
                }]
    compress_data = lzma.compress(data=raw_patch, format=lzma.FORMAT_XZ, check=lzma.CHECK_CRC32, filters=filterz)

    # header
    '''
        uint32_t magic_ver;
        uint32_t bin_type;
        uint32_t src_crc32;
        uint32_t dst_crc32;
        uint32_t src_length;
        uint32_t dst_length;
        uint32_t bin_offset;
        uint32_t bin_start_addr;
        uint32_t bin_length;
        uint32_t bin_crc32;
        uint8_t  reserved[24];
    '''
    bin_offset = 64
    patch_header = struct.pack("<10I24x",
                               0x004D4D4D,
                               0,
                               crc32(src_data),
                               crc32(dst_data),
                               len(src_data),
                               len(dst_data),
                               bin_offset,
                               0,
                               len(compress_data),
                               crc32(compress_data))

    print("src size:{}, dst size:{}, patch size:{}"
            .format(len(src_data), len(dst_data), (len(patch_header)+len(compress_data))))
    return (patch_header + compress_data)

def add_diff_flg(src_data, dst_data, block_size, dst_ability):
    start_addr_remain = 0
    start_addr_remain_crc = 0
    if (dst_ability & (1<<8)):
        start_addr_remain = globalvar.get_value('start_addr_remain')
        start_addr_remain_crc = globalvar.get_value('start_addr_remain_crc')
    pos = 0
    add_diff_data = bytearray(256)
    sector_num = 0
    print("len(dst_data):", len(dst_data))
    for index in range(len(dst_data)):
        if index < len(src_data):
            if(src_data[index] != dst_data[index]):
                sector_num = index//block_size
                sector_bit = 0
                pos = sector_num//8+sector_bit
                add_diff_data[pos+1] = (add_diff_data[pos+1] | (1<<(sector_num%8)))&0xff
        else:
            sector_num = index//block_size
            sector_bit = 0
            pos = sector_num//8+sector_bit
            add_diff_data[pos+1] = (add_diff_data[pos+1] | (1<<(sector_num%8)))&0xff

    data_len = pos + 1
    add_diff_data[0] = data_len
    print("data_len:{}, index:{}", data_len, index, add_diff_data[0], add_diff_data[1], add_diff_data[2], add_diff_data[3])
    return add_diff_data[0:data_len+1]

def diff2ya_patch_make(file_index, src_data, dst_data, mode, step, buf_size, block_size, diff_progress, ver_code, dst_ability):
    # bspatch
    bin_type_single = 0x53494E47    #
    bin_type_ab_1 = 0x41425349      #AB分段差分
    bin_type_ab_2 = 0x41425238      #可覆盖AB分段差分
    bin_type_compress = 0x4C5A4350  #压缩升级
    bin_type_ab_3 = 0x4C5A4352      #可覆盖分段压缩升级(A段压缩,B段原数据)
    bin_type_ab_4 = 0x4C5A4351      #分段压缩升级(A段压缩,B段原数据)
    patch_tail_header = bytes()

    patch_bin_ability = 0
    if(dst_ability & (1<<5)):
        patch_bin_ability |= 1<<5
    if(dst_ability & (1<<6)):
        patch_bin_ability |= 1<<6

    if (dst_ability & (1<<12)):
        raw_patch = dst_data
        print("use compress mode")
    else:
        raw_patch = format.diff_raw(src_data, dst_data, mode, step, buf_size, block_size, diff_progress)
        if raw_patch == -1:
            return raw_patch
    print('raw patch size:{}', len(raw_patch))

    # compress
    filterz =   [{  "id": lzma.FILTER_LZMA2,
                    "dict_size": 1024*4,
                    "depth": 0,
                    "mode": lzma.MODE_NORMAL,
                    "nice_len": 273,
                    "lc": 3,
                    "lp": 0,
                    "pb": 2,
                    "mf": lzma.MF_BT4
                }]
    compress_data = lzma.compress(data=raw_patch, format=lzma.FORMAT_XZ, check=lzma.CHECK_CRC32, filters=filterz)

    dst_ability_back = globalvar.get_value('dst_ability')
    src_ability_back = globalvar.get_value('src_ability')

    attr1 = 0
    attr2 = 0
    extend_len = 0
    diff_flg_len = 0
    bin_type = bin_type_single
    if (dst_ability & (1<<12)):
        bin_type = bin_type_compress
    if (dst_ability & (1<<3)):
        #1<<3	AB分段差分
        bin_type = bin_type_ab_1
        if (dst_ability & (1<<12)):
            bin_type = bin_type_ab_3
        attr1 = globalvar.get_value('dst_ability_attr1')
        if (dst_ability & (1<<5)):
            # bin_type = 0x11223343
            diff_ignore = globalvar.get_value('diff_ignore')
            attr2 = diff_ignore.offest<<16|diff_ignore.ignore_num
    if (dst_ability & (1<<4)):
		#1<<4	可覆盖AB分段差分
        bin_type = bin_type_ab_2
        if (dst_ability & (1<<12)):
            bin_type = bin_type_ab_4
        attr1 = globalvar.get_value('dst_ability_attr1')
        if (dst_ability & (1<<5)):
            diff_ignore = globalvar.get_value('diff_ignore')
            attr2 = diff_ignore.offest<<16|diff_ignore.ignore_num
    escape_flag = (~((1<<3) | (1<<4)))
    if (src_ability_back & escape_flag == dst_ability_back & escape_flag) and (dst_ability & (1<<9) or dst_ability & (1<<11)):
        if(dst_ability & (1<<9) and mode == 0):
            logging.info("speed updata...")
            src_data_bk_crc = src_data+src_data[0:(len(src_data)//32*2)]
            dst_data_bk_crc = dst_data+dst_data[0:(len(dst_data)//32*2)]
            if(dst_ability & (1<<5)):
                src_data_bk_crc = format.bk_calc_crc(src_data, src_data_bk_crc)
                dst_data_bk_crc = format.bk_calc_crc(dst_data, dst_data_bk_crc)
                # src_crc_io = open("./src_crc.bin", "wb")
                # dst_crc_io = open("./dst_crc.bin", "wb")
                # src_crc_io.write(src_data_bk_crc)
                # dst_crc_io.write(dst_data_bk_crc)
                # src_crc_io.close()
                # dst_crc_io.close()

                dst_ability_attr3 = globalvar.get_value('dst_ability_attr3')
                app_start_addr = dst_ability_attr3
                app_remain = (app_start_addr//32*34)%block_size
                src_data_4k_head = src_data_bk_crc
                dst_data_4k_head = dst_data_bk_crc
                if (dst_ability & (1<<8)):
                    src_data_4k_head = src_data[:app_remain] + src_data_bk_crc
                    dst_data_4k_head = src_data[:app_remain] + dst_data_bk_crc

                add_diff_data = add_diff_flg(src_data_4k_head, dst_data_4k_head, block_size, dst_ability)
                logging.info("bk7238 app_remain:{},app_start_addr:{},src_len:{},src_crc_len:{},src_crc_4k_len:{},dst_len:{},dst_crc_len:{},dst_crc_4k_len:{}", app_remain,app_start_addr, len(src_data), len(src_data_bk_crc), len(src_data_4k_head),len(dst_data), len(dst_data_bk_crc),len(dst_data_4k_head))
            else:
                add_diff_data = add_diff_flg(src_data, dst_data, block_size, dst_ability)
            diff_flg_len = len(add_diff_data)-1
            extend_len += len(add_diff_data)
            compress_data = add_diff_data + compress_data
            mode += 10

        if(dst_ability & (1<<11)):
            logging.info("add extend head...")
            extend_type = 0
            dst_attach_data_len = globalvar.get_value('dst_attach_data_len')
            dst_attach_data = globalvar.get_value('dst_attach_data')
            src_bin_len = globalvar.get_value('len_src')
            dst_bin_len = globalvar.get_value('len_dst')
            part_b_crc = crc32(dst_attach_data)
            extend_header = struct.pack("<5I124x",
                                extend_type,
                                patch_bin_ability,
                                src_bin_len,
                                dst_bin_len,
                                part_b_crc)
            extend_len += len(extend_header)

            compress_data = extend_header + compress_data
            # 如果分段模式下支持扩展头,需要在末尾增加64字节,暂放置crc,平台特性,用于新boot支持扩展头升级到boot不支持扩展头
            if ((dst_ability & (1<<3)) or (dst_ability & (1<<4))):
                platform_ability = 0
                if (dst_ability & (1<<6)):
                    platform_ability |= 1
                patch_head_data = struct.pack("<14I",
                            platform_ability,
                            part_b_crc,0,0,0,0,0,0,0,0,0,0,0,0)
                patch_tail_header = struct.pack("<2I",
                            0x004D4D4D,
                            crc32(patch_head_data)) + patch_head_data
                print("tail head add ",patch_tail_header.hex())
                globalvar.set_value('patch_tail_header', patch_tail_header)

    else:
        logging.warning("src_ability:{} != dst_ability:{}, no extend head...", src_ability_back, dst_ability_back)

    logging.info("dst_ability:{}, bin_type:{}, attr1:{}, attr2:{}, extend_len:{}",dst_ability, bin_type, attr1, attr2, extend_len)

    bin_offset = 64
    patch_header = struct.pack("<12I2B1H3I",
                               0x004D4D4D,
                               bin_type,
                               crc32(src_data),
                               crc32(dst_data),
                               len(src_data),
                               len(dst_data),
                               bin_offset,
                               0,
                               len(compress_data),
                               crc32(compress_data),
                               step,
                               buf_size,
                               mode,
                               file_index,
                               ver_code.verSuppMinDiffNum,
                               attr1,
                               attr2,
                               extend_len)
    print("src size:{}, dst size:{}, patch size:{}"
            .format(len(src_data), len(dst_data), (len(patch_header)+len(compress_data))))

    return (patch_header + compress_data)


def diff2ya_diff_make(src_data, dst_data):
    # bspatch
    raw_patch = format.diff(src_data, dst_data)
    if raw_patch == -1:
        return raw_patch
    print('raw diff size:', len(raw_patch))

    # compress
    filterz =   [{  "id": lzma.FILTER_LZMA2,
                    "dict_size": 1024*4,
                    "depth": 0,
                    "mode": lzma.MODE_NORMAL,
                    "nice_len": 273,
                    "lc": 3,
                    "lp": 0,
                    "pb": 2,
                    "mf": lzma.MF_BT4
                }]
    compress_data = lzma.compress(data=raw_patch, format=lzma.FORMAT_XZ, check=lzma.CHECK_CRC32, filters=filterz)


    bin_offset = 64
    patch_header = struct.pack("<12I1B15x",
                               0x004D4D4D,
                               0,
                               crc32(src_data),
                               crc32(dst_data),
                               len(src_data),
                               len(dst_data),
                               bin_offset,
                               0,
                               len(compress_data),
                               crc32(compress_data),
                               0,
                               0,
                               20)

    print("src size:{}, dst size:{}, patch size:{}"
            .format(len(src_data), len(dst_data), (len(patch_header)+len(compress_data))))

    return (patch_header + compress_data)
    '''
    return (patch_header + raw_patch)
    '''


def main_bsdiff2ya_double():
    p = OptionParser(
        usage="usage: %prog [options] SRC DST PATCH",
        description=("generate a BSDIFF4-format PATCH from SRC to DST "
                     "and write it to PATCH"))

    p.add_option('-v', "--verbose",
                 action="store_true")

    opts, args = p.parse_args()
    src_path = args[0]
    dst_path = args[1]
    patch_path = args[2]

    with open(src_path, 'rb') as f:
        src_data = f.read()

    with open(dst_path, 'rb') as f:
        dst_data = f.read()

    patch_io = open(patch_path, "wb")
    patch_io.write(diff2ya_patch_make_double(src_data, dst_data))
    patch_io.close()

    if len(args) != 3:
        logging.error("requies 3 arguments...")
        make_diff_res.diff_code = ErrCode.INVAL_PARAM_ERR
        return make_diff_res.diff_code.value

    # if opts.verbose:
    if True:
        size = [getsize(args[i]) for i in range(3)]
        print('src: %s, crc32:%08x' % (human_bytes(size[0]), crc32(src_data)))
        print('dst: %s, crc32:%08x' % (human_bytes(size[1]), crc32(dst_data)))
        print('patch: %s (%.2f%% of dst)' %
              (human_bytes(size[2]), 100.0 * size[2] / size[1]))


def main_bsdiff2ya():
    p = OptionParser(
        usage="usage: %prog [options] SRC DST PATCH",
        description=("generate a BSDIFF4-format PATCH from SRC to DST "
                     "and write it to PATCH"))

    p.add_option('-v', "--verbose",
                 action="store_true")

    opts, args = p.parse_args()
    src_path = args[0]
    dst_path = args[1]
    patch_path = args[2]
    mode = args[3]
    step = args[4]
    max_flash = args[5]
    buf_size = args[6]

    mode = int(mode)
    step = int(step)
    buf_size = int(buf_size)

    logging.info("mode:{}, step:{}, buf_size:{}", mode, step, buf_size)
    if (buf_size % 4 != 0):
        logging.error("buf_size:{} err... must 4k align", buf_size)
        return -1
    #return -1

    with open(src_path, 'rb') as f:
        src_data = f.read()

    with open(dst_path, 'rb') as f:
        dst_data = f.read()

    patch_io = open(patch_path, "wb")
    patch_io.write(diff2ya_patch_make(0, src_data, dst_data, mode, step, buf_size))
    patch_io.close()

    os.system('cp ./patch.bin ../../build/examples/linux_simulation/')
    os.system('cp %s ../../build/examples/linux_simulation/' % (src_path))

    os.system('python ./format_up_bin.py %s %s_%s %s' % (src_path, "head", src_path, max_flash))
    os.system('python ./format_up_bin.py %s %s_%s %s' % (dst_path, "head", dst_path, max_flash))

    if len(args) != 7:
        logging.error("requies 7 arguments...")

    # if opts.verbose:
    if True:
        size = [getsize(args[i]) for i in range(3)]
        print()
        logging.info("src: %s, crc32:%08x" , human_bytes(size[0]), crc32(src_data))
        logging.info("dst: %s, crc32:%08x" , human_bytes(size[1]), crc32(dst_data))
        logging.info("patch: %s (%.2f%% of dst)" , human_bytes(size[2]), 100.0 * size[2] / size[1])

def get_diff2ya_patch_AB(file_index, src_data, dst_data, max_flash_dst, buf_size, block_size, diff_progress):
    src_ability_attr1 = globalvar.get_value('src_ability_attr1')
    dst_ability_attr1 = globalvar.get_value('dst_ability_attr1')
    diff_ignore = globalvar.get_value('diff_ignore')
    dst_ability = globalvar.get_value('dst_ability')

    len_src_ab = src_ability_attr1
    len_dst_ab = dst_ability_attr1
    max_len_all = len(src_data) if len(src_data) >= len(dst_data) else len(dst_data)
    src_data_ab = src_data[:len_src_ab]
    dst_data_ab = dst_data[:len_dst_ab]

    # globalvar.set_value('dst_ability', 0)

    make_diff_res = MakeDiffRes()
    ver_code = VerCode()
    max_data = len(src_data_ab) if len(src_data_ab) >= len(dst_data_ab) else len(dst_data_ab)
    if (dst_ability & (1<<6)):
        max_data = max_data if (max_data % diff_ignore.offest == 0) else (max_data + diff_ignore.offest - max_data % diff_ignore.offest)
        max_data += max_data // diff_ignore.offest * diff_ignore.ignore_num

        max_len_all = max_len_all if (max_len_all % diff_ignore.offest == 0) else (max_len_all + diff_ignore.offest - max_len_all % diff_ignore.offest)
        max_len_all += max_len_all // diff_ignore.offest * diff_ignore.ignore_num

    if max_data%block_size:
        max_data = max_data + block_size-max_data%block_size

    if max_len_all%block_size:
        max_len_all = max_len_all + block_size-max_len_all%block_size

    max_patch = max_flash_dst - max_data - buf_size - 2*block_size - 64

    step = 0
    mode = 0
    ab_mode = 0
    # mode = 1 if len(dst_data_ab) >= len(src_data_ab) else 0
    if (dst_ability & (1<<10)):
        dst_diff_test_param = globalvar.get_value('dst_diff_test_param')
        mode = (dst_diff_test_param>>16) & 0xff
        # step = (dst_diff_test_param>>8) & 0xff
        ab_mode = (dst_diff_test_param>>24) & 0xff
        logging.warning("test mode... mode:{}, step:{},ab_mode:{}", mode, step, ab_mode)

    while mode < 1:
        dst_ability_1 = dst_ability & (~(1<<4))
        dst_ability_1 = dst_ability_1 | (1<<3)
        logging.info("AB dst_ability:{}, dst_ability_1:{}", dst_ability, dst_ability_1)
        patch_data = diff2ya_patch_make(file_index, src_data_ab, dst_data_ab, mode, step, buf_size, block_size, diff_progress, ver_code, dst_ability_1)
        if patch_data == -1:
            make_diff_res.diff_code = ErrCode.MAKE_PATCH_ERR
            logging.error("create patch_AB file fail...res:{}, mode:{}, step:{}, buf_size:{}, max_flash_dst:{}, max_patch:{}" ,make_diff_res.diff_code.value, mode, step, buf_size, max_flash_dst, max_patch)
            return make_diff_res.diff_code, patch_data
        patch_data_len = len(patch_data)
        if patch_data_len%block_size:
            patch_data_len = patch_data_len + block_size-patch_data_len%block_size
        if patch_data_len > max_patch:
            mode += 1
        else:
            if (patch_data_len < (max_patch + max_data - max_len_all) and ab_mode != 2):
                # dst_ability = (1<<3)
                # globalvar.set_value('dst_ability', dst_ability)
                return make_diff_res.diff_code, patch_data
            else:
                if (dst_ability & (1<<4)):
                    if (patch_data_len < max_patch):
                        #可覆盖
                        dst_ability_1 = dst_ability & (~(1<<3))
                        # globalvar.set_value('dst_ability', dst_ability)
                        patch_data = diff2ya_patch_make(file_index, src_data_ab, dst_data_ab, mode, step, buf_size, block_size, diff_progress, ver_code, dst_ability_1)
                        return make_diff_res.diff_code, patch_data
            mode += 1
        if mode == 1:
            if (dst_ability & (1<<5) or dst_ability & (1<<6)):
                mode = 2
        if mode >= 1:
            make_diff_res.diff_code = ErrCode.PATCH_LEN_OVER_ERR
            logging.error("create patch file fail...res:{}, mode:{}, step:{}, buf_size:{}, max_flash_dst:{}, max_patch:{}" ,make_diff_res.diff_code.value, mode, step, buf_size, max_flash_dst, max_patch)
            return make_diff_res.diff_code, -1
    make_diff_res.diff_code = ErrCode.INVAL_PARAM_ERR
    logging.error("create patch_AB file fail...res:{}, mode:{}, step:{}, buf_size:{}, max_flash_dst:{}, max_patch:{}" ,make_diff_res.diff_code.value, mode, step, buf_size, max_flash_dst, max_patch)
    return make_diff_res.diff_code, -1

def get_diff2ya_patch(file_index, src_data, dst_data, max_flash_dst, buf_size, block_size, diff_progress):
    make_diff_res = MakeDiffRes()
    ver_code = VerCode()
    dst_ability = globalvar.get_value('dst_ability')
    dst_ability_no_ab = dst_ability
    diff_ignore = globalvar.get_value('diff_ignore')
    src_ability_attr1 = globalvar.get_value('src_ability_attr1')
    dst_ability_attr1 = globalvar.get_value('dst_ability_attr1')
    dst_attach_data = globalvar.get_value('dst_attach_data')

    '''
    if (dst_ability & (1<<12)):
        src_data_a_pack = src_data[:src_ability_attr1]
        dst_data_a_pack = dst_data[:dst_ability_attr1]
        print ("compress a data")
        # compress a data
        filterz =   [{  "id": lzma.FILTER_LZMA2,
                        "dict_size": 1024*4,
                        "depth": 0,
                        "mode": lzma.MODE_NORMAL,
                        "nice_len": 273,
                        "lc": 3,
                        "lp": 0,
                        "pb": 2,
                        "mf": lzma.MF_BT4
                    }]
        compress_a_data = lzma.compress(data=dst_data_a_pack, format=lzma.FORMAT_XZ, check=lzma.CHECK_CRC32, filters=filterz)

        step = 0
        bin_offset = 64

        len_dst_ab = dst_ability_attr1
        dst_data_ab = dst_data[:len_dst_ab]
        max_data = len(dst_data_ab)
        max_patch = max_flash_dst - max_data - buf_size - 2*block_size - 64

        max_len_all = len(dst_data)

        if len(compress_a_data) < (max_patch + max_data - max_len_all):
            # 压缩升级
            bin_type = 0x4C5A4350
        else:
            # 分段压缩升级(A段压缩,B段原数据)
            bin_type = 0x4C5A4351
        patch_header = struct.pack("<12I2B1H3I",
                                0x004D4D4D,
                                bin_type,
                                crc32(src_data_a_pack),
                                crc32(dst_data_a_pack),
                                len(src_data_a_pack),
                                len(dst_data_a_pack),
                                bin_offset,
                                0,
                                len(compress_a_data),
                                crc32(compress_a_data),
                                step,
                                buf_size,
                                0, # mode = 0
                                0, # file_index = 0
                                ver_code.verSuppMinDiffNum,
                                dst_ability_attr1, # attr1
                                globalvar.get_value('dst_ability_attr2'), # attr2,
                                0)                 # extend_len
        print("src size:{}, dst size:{} compress_a_data:{} "
                .format(len(src_data), len(dst_data), len(compress_a_data)))

        patch_data = patch_header + compress_a_data

        make_diff_res.diff_code = ErrCode.OK
        pack_ab_data = patch_data + dst_attach_data

        # update progress
        diff_progress.progress = 1.0
        prr = format.get_diff_progress_test(diff_progress)

        return make_diff_res.diff_code, pack_ab_data
    '''
    mode = 0
    mode = 1 if len(dst_data) > len(src_data) else 0
    mode = 0
    ab_mode = 0
    step = 0#step=4096
    if (dst_ability & (1<<10)):
        dst_diff_test_param = globalvar.get_value('dst_diff_test_param')
        mode = (dst_diff_test_param>>16) & 0xff
        step = (dst_diff_test_param>>8) & 0xff
        ab_mode = (dst_diff_test_param>>24) & 0xff
        logging.warning("test mode... mode:{}, step:{},ab_mode:{}", mode, step, ab_mode)
    if ab_mode!=0:
        mode = 2

    max_step = block_size*5 #20*1024
    is_diff_make = 0
    if (dst_ability & (1<<5) or dst_ability & (1<<6)):
        max_step = 0
    if ((dst_ability & (1<<3)) or (dst_ability & (1<<4))):
        #1<<1	AB分段差分
		#1<<2	可覆盖AB分段差分
        max_step = 0
        is_diff_make = 1
        dst_ability_no_ab = dst_ability_no_ab & (~(1<<3))
        dst_ability_no_ab = dst_ability_no_ab & (~(1<<4))
        globalvar.set_value('dst_ability', dst_ability_no_ab)

    max_data = len(src_data) if len(src_data) >= len(dst_data) else len(dst_data)
    if (dst_ability_no_ab & (1<<6)):
        max_data = max_data if (max_data % diff_ignore.offest == 0) else (max_data + diff_ignore.offest - max_data % diff_ignore.offest)
        max_data += max_data // diff_ignore.offest * diff_ignore.ignore_num
    if max_data%block_size:
        max_data = max_data + block_size-max_data%block_size
    max_patch = max_flash_dst - max_data - buf_size - 2*block_size - 64
    max_patch_step = max_patch - step

    logging.info("max_patch:{}, max_data:{}, max_flash_dst:{}, max_patch_step:{}, buf_size:{}, block_size:{}, dst_ability:{}, dst_ability_no_ab:{}", max_patch, max_data, max_flash_dst, max_patch_step, buf_size, block_size, dst_ability, dst_ability_no_ab)

    while mode < 1:
        logging.info("ver:20220706, verName:{}, verNum:{}, verSuppMinDiffNum:{}", ver_code.verName, ver_code.verNum, ver_code.verSuppMinDiffNum)
        logging.info("***start patch file...mode:{}, step:{}, buf_size:{}, max_flash_dst:{}, max_patch_step:{}", mode, step, buf_size, max_flash_dst, max_patch_step)
        # if (dst_ability_no_ab & (1<<5)) and mode != 0:
        #     make_diff_res.diff_code = ErrCode.UNSUPPORT_ERR
        #     logging.error("mode == 0 and dst_ability_no_ab:{}" , dst_ability_no_ab)
        #     return make_diff_res.diff_code, -1

        if mode == 0 and step != 0:
            make_diff_res.diff_code = ErrCode.INVAL_PARAM_ERR
            logging.error("mode == 0 and step == {} err..." ,step)
            return make_diff_res.diff_code, -1
        dst_ability_normal = (dst_ability & (~(1<<3))) & (~(1<<4))
        patch_data = diff2ya_patch_make(file_index, src_data, dst_data, mode, step, buf_size, block_size, diff_progress, ver_code, dst_ability_normal)
        if patch_data == -1:
            make_diff_res.diff_code = ErrCode.MAKE_PATCH_ERR
            logging.error("create patch file fail...res:{}, mode:{}, step:{}, buf_size:{}, max_flash_dst:{}, max_patch_step:{}" ,make_diff_res.diff_code.value, mode, step, buf_size, max_flash_dst, max_patch_step)
            return make_diff_res.diff_code, patch_data
        patch_data_len = len(patch_data)
        if patch_data_len%block_size:
            patch_data_len = patch_data_len + block_size-patch_data_len%block_size

        if patch_data_len > max_patch_step:
            logging.info("patch len:{}, max_patch:{}" , len(patch_data), max_patch)
            if (dst_ability_no_ab & (1<<5) or dst_ability_no_ab & (1<<6)):
                mode = 2
                break
            if is_diff_make == 0:
                is_diff_make = 1
                diff_data = diff2ya_diff_make(src_data, dst_data)
                logging.info("len(patch_data) > max_patch_step, creat diff file...diff2ya_diff_make_len:{}, diff2ya_patch_make_len:{}, max_patch_step:{}" ,len(diff_data), len(patch_data), max_patch_step)
                if len(diff_data) > max_patch_step:
                    mode = 2
                    break
                elif len(diff_data) == max_patch_step:
                    patch_data = diff_data
                    break
                else:
                    tmp = max_patch_step - len(diff_data)
                    max_step = tmp if tmp < step else max_step

            if mode > 0:
                step += block_size
                if step > max_step:
                    mode = mode + 1
                    step = 0
                max_patch_step = max_patch - step
            else:
                mode = mode + 1
        else:
            # if ((dst_ability & (1<<3)) or (dst_ability & (1<<4))):
            #     logging.info("AB patch create...dst_ability:{}", dst_ability)
                # dst_ability = 0
                # globalvar.set_value('dst_ability', dst_ability)
            break

    if mode >= 1:
    # if 1:
        if ((dst_ability & (1<<3)) or (dst_ability & (1<<4))):
            #1<<1	AB分段差分
		    #1<<2	可覆盖AB分段差分
            logging.info("AB patch create...dst_ability:{}", dst_ability)
            globalvar.set_value('dst_ability', dst_ability)
            make_diff_res.diff_code, patch_data = get_diff2ya_patch_AB(file_index, src_data, dst_data, max_flash_dst, buf_size, block_size, diff_progress)
            if patch_data != -1:
                patch_data = patch_data + dst_attach_data
                if globalvar.get_value('patch_tail_header') != None:
                    patch_data = patch_data + globalvar.get_value('patch_tail_header')
            else:
                make_diff_res.diff_code = ErrCode.PATCH_LEN_OVER_ERR
                logging.error("create patch file fail...res:{}, mode:{}, step:{}, buf_size:{}, max_flash_dst:{}, max_patch:{}" ,make_diff_res.diff_code.value, mode, step, buf_size, max_flash_dst, max_patch)
                return make_diff_res.diff_code, -1
        else:
            make_diff_res.diff_code = ErrCode.PATCH_LEN_OVER_ERR
            logging.error("create patch file fail...res:{}, mode:{}, step:{}, buf_size:{}, max_flash_dst:{}, max_patch:{}" ,make_diff_res.diff_code.value, mode, step, buf_size, max_flash_dst, max_patch)
            return make_diff_res.diff_code, -1
    else:
        logging.info("create patch file succ...res:{}, mode:{}, step:{}, buf_size:{}, max_flash_dst:{}, patch len:{}, max_patch:{}" ,make_diff_res.diff_code.value, mode, step, buf_size, max_flash_dst, len(patch_data), max_patch)

    return make_diff_res.diff_code, patch_data

def get_platform_type(file_index, src_data, dst_data, task_id, magic_src, magic_dst):
    make_diff_res = MakeDiffRes()
    make_diff_res.str_io = -1
    make_diff_res.diff_code = ErrCode.MAGIC_ERR
    # magic = 0x74757961
    magic_cn = 0x74757962
    magic_nb2 = 0x74757963

    logging.info("-----magic_dst:{}-----", magic_dst)
    if (0 == find_cn_bin(src_data, dst_data, task_id, magic_src, magic_dst)):

        logging.info("cn magic...")
        logging.info("-----src_data len:{} dst_data len:{}-----", len(src_data), len(dst_data))
        patch = make_cn_patch(src_data, dst_data, task_id, magic_src, magic_dst)

        if (-1 == patch):
            make_diff_res.str_io = patch
            make_diff_res.diff_code = ErrCode.OTHER_PATCH_ERR
            logging.error("get cn patch err")
            return make_diff_res
        else :
            make_diff_res.diff_code = ErrCode.OK
            make_diff_res.str_io = patch
    elif (check_is_nbiot(src_data, dst_data) == True):
        logging.info("nb magic ...")
        patch = make_nb_patch(src_data, dst_data)
        if (-1 == patch):
            make_diff_res.str_io = patch
            make_diff_res.diff_code = ErrCode.OTHER_PATCH_ERR
            logging.error("get nb patch err")
            return make_diff_res
        else :
            make_diff_res.diff_code = ErrCode.OK
            make_diff_res.str_io = patch

    return make_diff_res

def do_bsdiff2ya(file_index, src_data, dst_data, task_id):
    make_diff_res = MakeDiffRes()
    diff_progress = DiffProgress()
    diff_ignore = DiffIgnore()
    is_double_zone = 0
    src_up_bin_crc = 0
    dst_up_bin_crc = 0
    src_diff_test_param = 0
    dst_diff_test_param = 0
    src_ability = 0
    dst_ability = 0
    src_ability_attr0 = 0
    src_ability_attr1 = 0
    src_ability_attr2 = 0
    dst_ability_attr0 = 0
    dst_ability_attr1 = 0
    dst_ability_attr2 = 0
    src_ability_attr3 = 0
    dst_ability_attr3 = 0
    src_extend_len = 0
    dst_extend_len = 0
    src_extend_data = src_data
    dst_extend_data = dst_data
    dst_attach_data = dst_data
    src_attach_data = src_data

    diff_progress.task_id = task_id

    src_extend_len = (src_data[len(src_data)-4]<<24)+(src_data[len(src_data)-3]<<16)+(src_data[len(src_data)-2]<<8)+(src_data[len(src_data)-1]<<0)
    dst_extend_len = (dst_data[len(dst_data)-4]<<24)+(dst_data[len(dst_data)-3]<<16)+(dst_data[len(dst_data)-2]<<8)+(dst_data[len(dst_data)-1]<<0)
    logging.info("src_extend_len:{}, dst_extend_len:{}" ,src_extend_len, dst_extend_len)
    if ((src_extend_len > (len(src_data) - 64)) or (dst_extend_len > (len(dst_data) - 64))):
        logging.warning("extend_len err...")
        src_extend_len = 0
        dst_extend_len = 0
    len_src = len(src_data) - 64 - src_extend_len
    len_dst = len(dst_data) - 64 - dst_extend_len

    magic = 0x74757961
    globalvar.set_value('len_src', len_src)
    globalvar.set_value('len_dst', len_dst)

    magic_src = (src_data[len_src+0]<<24)+(src_data[len_src+1]<<16)+(src_data[len_src+2]<<8)+(src_data[len_src+3]<<0)
    magic_dst = (dst_data[len_dst+0]<<24)+(dst_data[len_dst+1]<<16)+(dst_data[len_dst+2]<<8)+(dst_data[len_dst+3]<<0)

    make_diff_res_other = get_platform_type(file_index, src_data, dst_data, task_id, magic_src, magic_dst)
    if(make_diff_res_other.diff_code != ErrCode.MAGIC_ERR):
        return make_diff_res_other

    if ((magic_src != magic) or (magic_dst != magic)):
        if ((magic_src != magic) and (magic_dst != magic)):
            is_double_zone = 1
        else:
            logging.error("file magic err...magic:{}, magic_src:{}, magic_dst:{}" ,magic, magic_src, magic_dst)
            make_diff_res.diff_code = ErrCode.MAGIC_ERR
            return make_diff_res
    else:
        is_double_zone = 0

    logging.info("-----is_double_zone:{}-----", is_double_zone)
    if (is_double_zone == 1):
        patch_data = diff2ya_patch_make_double(src_data, dst_data, diff_progress)
        if patch_data == -1:
            logging.error("diff2ya_patch_make_double err...")
            make_diff_res.diff_code = ErrCode.MAKE_PATCH_ERR_DOUBLE
            return make_diff_res
        else:
            make_diff_res.str_io = patch_data
    else:
        max_flash_src = (src_data[len_src+8]<<24)+(src_data[len_src+9]<<16)+(src_data[len_src+10]<<8)+(src_data[len_src+11]<<0)
        max_flash_dst = (dst_data[len_dst+8]<<24)+(dst_data[len_dst+9]<<16)+(dst_data[len_dst+10]<<8)+(dst_data[len_dst+11]<<0)
        if max_flash_src != max_flash_dst:
            make_diff_res.diff_code = ErrCode.MAX_FLASH_ERR
            logging.warning("file max_flash_src != max_flash_dst err...res:{}, max_flash_src:{}, max_flash_dst:{}" , ErrCode.MAX_FLASH_ERR.value, max_flash_src, max_flash_dst)
            # return make_diff_res

        buf_size = (src_data[len_src+16]<<24)+(src_data[len_src+17]<<16)+(src_data[len_src+18]<<8)+(src_data[len_src+19]<<0)
        part_num = (src_data[len_src+20]<<24)+(src_data[len_src+21]<<16)+(src_data[len_src+22]<<8)+(src_data[len_src+23]<<0)
        block_size = (src_data[len_src+24]<<24)+(src_data[len_src+25]<<16)+(src_data[len_src+26]<<8)+(src_data[len_src+27]<<0)
        src_ability = (src_data[len_src+12]<<24)+(src_data[len_src+13]<<16)+(src_data[len_src+14]<<8)+(src_data[len_src+15]<<0)
        dst_ability = (dst_data[len_dst+12]<<24)+(dst_data[len_dst+13]<<16)+(dst_data[len_dst+14]<<8)+(dst_data[len_dst+15]<<0)
        if src_ability != dst_ability:
            make_diff_res.diff_code = ErrCode.INVAL_PARAM_ERR
            logging.warning("src_ability != dst_ability err...res:{}, src_ability:{}, dst_ability:{}" , ErrCode.INVAL_PARAM_ERR.value, src_ability, dst_ability)
            # return make_diff_res
        globalvar.set_value('src_ability', src_ability)
        globalvar.set_value('dst_ability', dst_ability)
        src_up_bin_crc = (src_data[len_src+32]<<24)+(src_data[len_src+33]<<16)+(src_data[len_src+34]<<8)+(src_data[len_src+35]<<0)
        dst_up_bin_crc = (dst_data[len_dst+32]<<24)+(dst_data[len_dst+33]<<16)+(dst_data[len_dst+34]<<8)+(dst_data[len_dst+35]<<0)
        if (dst_ability & (1<<10)):
            src_diff_test_param = src_up_bin_crc
            dst_diff_test_param = dst_up_bin_crc
            logging.warning("test mode... src_diff_test_param:{}, dst_diff_test_param:{}", src_diff_test_param, dst_diff_test_param)
        globalvar.set_value('src_diff_test_param', src_diff_test_param)
        globalvar.set_value('dst_diff_test_param', dst_diff_test_param)

        src_ability_attr0 = (src_data[len_src+36]<<24)+(src_data[len_src+37]<<16)+(src_data[len_src+38]<<8)+(src_data[len_src+39]<<0)
        dst_ability_attr0 = (dst_data[len_dst+36]<<24)+(dst_data[len_dst+37]<<16)+(dst_data[len_dst+38]<<8)+(dst_data[len_dst+39]<<0)
        src_ability_attr1 = (src_data[len_src+40]<<24)+(src_data[len_src+41]<<16)+(src_data[len_src+42]<<8)+(src_data[len_src+43]<<0)
        dst_ability_attr1 = (dst_data[len_dst+40]<<24)+(dst_data[len_dst+41]<<16)+(dst_data[len_dst+42]<<8)+(dst_data[len_dst+43]<<0)
        src_ability_attr2 = (src_data[len_src+44]<<24)+(src_data[len_src+45]<<16)+(src_data[len_src+46]<<8)+(src_data[len_src+47]<<0)
        dst_ability_attr2 = (dst_data[len_dst+44]<<24)+(dst_data[len_dst+45]<<16)+(dst_data[len_dst+46]<<8)+(dst_data[len_dst+47]<<0)
        src_ability_attr3 = (src_data[len_src+48]<<24)+(src_data[len_src+49]<<16)+(src_data[len_src+50]<<8)+(src_data[len_src+51]<<0)
        dst_ability_attr3 = (dst_data[len_dst+48]<<24)+(dst_data[len_dst+49]<<16)+(dst_data[len_dst+50]<<8)+(dst_data[len_dst+51]<<0)
        src_extend_len = (src_data[len_src+60]<<24)+(src_data[len_src+61]<<16)+(src_data[len_src+62]<<8)+(src_data[len_src+63]<<0)
        dst_extend_len = (dst_data[len_dst+60]<<24)+(dst_data[len_dst+61]<<16)+(dst_data[len_dst+62]<<8)+(dst_data[len_dst+63]<<0)
        globalvar.set_value('src_ability_attr0', src_ability_attr0)
        globalvar.set_value('dst_ability_attr0', dst_ability_attr0)
        globalvar.set_value('src_ability_attr1', src_ability_attr1)
        globalvar.set_value('dst_ability_attr1', dst_ability_attr1)
        globalvar.set_value('src_ability_attr2', src_ability_attr2)
        globalvar.set_value('dst_ability_attr2', dst_ability_attr2)
        globalvar.set_value('src_ability_attr3', src_ability_attr3)
        globalvar.set_value('dst_ability_attr3', dst_ability_attr3)

        # logging.info("36:{}, 37:{}, 38:{}, 39:{}, ", src_data[len_src+36],src_data[len_src+37],src_data[len_src+38],src_data[len_src+39])
        logging.info("dst_ability:{},dst_ability_attr0:{}, dst_ability_attr1:{}, dst_ability_attr2:{}, dst_ability_attr3:{}", dst_ability, dst_ability_attr0, dst_ability_attr1,dst_ability_attr2,dst_ability_attr3)
        if src_extend_len != 0 and src_extend_len != 0xffffffff:
            src_extend_data = src_data[(len_src-src_extend_len):len_src]
            len_src -= src_extend_len
        if dst_extend_len != 0 and dst_extend_len != 0xffffffff:
            dst_extend_data = dst_data[(len_dst-dst_extend_len):len_dst]
            len_dst -= dst_extend_len
        globalvar.set_value('src_extend_len', src_extend_len)
        globalvar.set_value('dst_extend_len', dst_extend_len)
        globalvar.set_value('src_extend_data', src_extend_data)
        globalvar.set_value('dst_extend_data', dst_extend_data)

        src_data = src_data[:len_src]
        dst_data = dst_data[:len_dst]

        src_attach_data = src_data[src_ability_attr1:len_src]
        dst_attach_data = dst_data[dst_ability_attr1:len_dst]
        dst_attach_data_len = len(dst_attach_data)
        globalvar.set_value('dst_attach_data_len', dst_attach_data_len)
        globalvar.set_value('dst_attach_data', dst_attach_data)
        if ((dst_ability & (1<<3)) or (dst_ability & (1<<4))):
            #1<<1	AB分段差分
			#1<<2	可覆盖AB分段差分
            '''
            len_src = src_ability_attr1
            len_dst = dst_ability_attr1
            src_data = src_data[:len_src]
            dst_data = dst_data[:len_dst]
            '''
        # if (dst_ability & (1<<5)):
        #bk7235支持可配置buffer_size
        if src_ability_attr2 != dst_ability_attr2:
            make_diff_res.diff_code = ErrCode.INVAL_PARAM_ERR
            logging.error("src_ability_attr2 != dst_ability err...res:{}, src_ability_attr2:{}, dst_ability_attr2:{}" , ErrCode.INVAL_PARAM_ERR.value, src_ability_attr2, dst_ability_attr2)
            return make_diff_res
        diff_ignore.offest = (src_ability_attr2>>16)&0xff
        diff_ignore.ignore_num = src_ability_attr2&0xff
        globalvar.set_value('diff_ignore', diff_ignore)
        logging.info("src_ability_attr2:{}, dst_attach_data_len:{}", src_ability_attr2, dst_attach_data_len)

        logging.info("len_src:{}, len_dst:{}, max_flash_dst:{}, buf_size:{}, part_num:{}, block_size:{}", len_src, len_dst, max_flash_dst, buf_size, part_num, block_size)



        if part_num != 0:
            zone_num_src = (src_data[99]<<24) + (src_data[98]<<16) + (src_data[97]<<8) + (src_data[96]<<0)
            zone_num_dst = (dst_data[99]<<24) + (dst_data[98]<<16) + (dst_data[97]<<8) + (dst_data[96]<<0)
            zone_num = zone_num_dst
            if (zone_num_src != zone_num_dst or zone_num <= 0 or zone_num > 10):
                make_diff_res.diff_code = ErrCode.ZONE_NUM_ERR
                logging.error("do multi zone patch err...zone_num_src:{}, zone_num_dst:{}, part_num:{}", zone_num_src, zone_num_dst, part_num)
                return make_diff_res

            diff_progress.all_part_num = zone_num

            ota_zone_len = 32+32+4+4+2*32+4+32+4+32

            src_start_pos = block_size + 4 + 256
            dst_start_pos = src_start_pos
            src_image_pos = 32+32+32+4+(32+32+4)
            dst_image_pos = 32+32+32+4+(32+32+4)
            src_image_size = (src_data[src_image_pos+3]<<24) + (src_data[src_image_pos+2]<<16) + (src_data[src_image_pos+1]<<8) + (src_data[src_image_pos]<<0)
            dst_image_size = (dst_data[dst_image_pos+3]<<24) + (dst_data[dst_image_pos+2]<<16) + (dst_data[dst_image_pos+1]<<8) + (dst_data[dst_image_pos]<<0)

            dst_head_array = bytearray(dst_data[0:(block_size+4+256)])
            str_io = None

            for index in range(zone_num):
                logging.info("zone_num:{}, index:{}, src_start_pos:{}, dst_start_pos:{}, src_image_pos:{}, dst_image_pos:{}, src_image_size:{}, dst_image_size:{}, ota_zone_len:{}", zone_num, index, src_start_pos, dst_start_pos, src_image_pos, dst_image_pos, src_image_size, dst_image_size, ota_zone_len)
                diff_progress.part_num = index+1

                make_diff_res.diff_code, patch_data = get_diff2ya_patch(file_index, src_data[src_start_pos:(src_start_pos+src_image_size)], dst_data[dst_start_pos:(dst_start_pos+dst_image_size)], max_flash_dst, buf_size, block_size, diff_progress)
                if patch_data == -1:
                    logging.error("get_diff2ya_patch err, zone_num:{}, index:{}, src_start_pos:{}, dst_start_pos:{}, src_image_pos:{}, dst_image_pos:{}, src_image_size:{}, dst_image_size:{}", zone_num, index, src_start_pos, dst_start_pos, src_image_pos, dst_image_pos, src_image_size, dst_image_size)
                    return make_diff_res
                else:
                    patch_len = len(patch_data)
                    dst_head_array[src_image_pos+3] = (patch_len>>24) & 0xff
                    dst_head_array[src_image_pos+2] = (patch_len>>16) & 0xff
                    dst_head_array[src_image_pos+1] = (patch_len>>8) & 0xff
                    dst_head_array[src_image_pos] = (patch_len>>0) & 0xff

                    if str_io == None:
                        str_io = patch_data
                    else:
                        str_io = str_io + patch_data

                src_start_pos += src_image_size
                dst_start_pos += dst_image_size
                src_image_pos += ota_zone_len
                dst_image_pos += ota_zone_len
                src_image_size = (src_data[src_image_pos+3]<<24) + (src_data[src_image_pos+2]<<16) + (src_data[src_image_pos+1]<<8) + (src_data[src_image_pos]<<0)
                dst_image_size = (dst_data[dst_image_pos+3]<<24) + (dst_data[dst_image_pos+2]<<16) + (dst_data[dst_image_pos+1]<<8) + (dst_data[dst_image_pos]<<0)

            make_diff_res.str_io = bytes(dst_head_array) + str_io

        else:
            make_diff_res.diff_code, patch_data = get_diff2ya_patch(file_index, src_data, dst_data, max_flash_dst, buf_size, block_size, diff_progress)
            if patch_data == -1:
                logging.error("get_diff2ya_patch err...max_flash_dst:{}, buf_size:{}, block_size:{}", max_flash_dst, buf_size, block_size)
                return make_diff_res
            else:
                make_diff_res.str_io = patch_data

    if (dst_ability & (1<<2)):
        #Zigbee加头加在这里
        logging.info("zigbee magic ...")
        manu_code = ((dst_ability_attr0>>16)&0xffff)
        image_type = (dst_ability_attr0&0xffff)
        zigbee_rev = src_ability_attr1
        zigbee_rev1 = src_ability_attr2

        src_file_name = globalvar.get_value('src_file_name')
        dst_file_name = globalvar.get_value('dst_file_name')
        src_ver = globalvar.get_value('src_ver')
        dst_ver = globalvar.get_value('dst_ver')
        logging.info("dst_ability_attr0:{}, manu_code:{}, image_type:{}, src_file_name:{}, dst_file_name:{}, src_ver:{}, dst_ver:{}", dst_ability_attr0, manu_code, image_type, src_file_name, dst_file_name, src_ver, dst_ver)
        zigbee_patch = Patch_convert(make_diff_res.str_io,manu_code,image_type,dst_ver,dst_file_name,zigbee_rev, src_extend_data, dst_extend_data)
        if zigbee_patch == -1:
            make_diff_res.diff_code = ErrCode.ZIGBEE_HEAD_ERR
            logging.error("add zigbee head err")
            return make_diff_res
        else :
            make_diff_res.str_io = zigbee_patch
    if (dst_ability & (1<<7)):
        # matter(在生成的差分包前面再加一个固定头)
        logging.info("add matter head")
    return make_diff_res

def is_zip_or_xz_file(file_name):
    kind = filetype.guess(file_name)
    if kind is None:
        print('Cannot guess file type!')
        return 0
    #print('File extension: %s' % kind.extension)
    if kind.extension == "zip":
        return kind.extension
    if kind.extension == "xz":
        return kind.extension
    return 0

def un_zip(file_name):
    """unzip zip file"""
    zip_file = zipfile.ZipFile(file_name)
    if os.path.isdir(file_name + "_files"):
        pass
    else:
        os.mkdir(file_name + "_files")
    for names in zip_file.namelist():
        zip_file.extract(names,file_name + "_files/")
    zip_file.close()


def tar_xz(zip_src, dst_dir):
    print("zip_src:%s, dst_dir: %s." % (zip_src, dst_dir))
    os.system('tar Jxf %s -C %s' % (zip_src, dst_dir))

def mkdir(path):
	folder = os.path.exists(path)
	if not folder:                   #判断是否存在文件夹如果不存在则创建为文件夹
		os.makedirs(path)            #makedirs 创建文件时如果路径不存在会创建这个路径


def del_src_file(path):
    os.system("rm -rf %s/tuyafs/lib/libnvram_srv.so" % (path))
    os.system("rm -rf %s/tuyafs/lib/libtuya_cell_adapter.so" % (path))
    os.system("rm -rf %s/tuyafs/lib/libtuya_gnss_client.so" % (path))
    os.system("rm -rf %s/tuyafs/lib/libtuya_gnss_srv.so" % (path))
    os.system("rm -rf %s/tuyafs/lib/libtuya_netctrl.so" % (path))
    os.system("rm -rf %s/tuyafs/lib/libtuya_nvram_client.so" % (path))

    os.system("rm -rf %s/tuyafs/lib/libtuya_opencpu.so" % (path))
    os.system("rm -rf %s/tuyafs/lib/libtuya_os_adatper.so" % (path))
    os.system("rm -rf %s/tuyafs/lib/libtuya_ril_v2.so" % (path))


def un_file(zip_src, dst_dir):
    file_type = is_zip_or_xz_file(zip_src)
    print("file_type:%s, file_type: %s." % (file_type, file_type))
    if file_type == 0:
        print('This is not zip or xz file!!!')

    if file_type == "zip":
        r = zipfile.is_zipfile(zip_src)
        if r:
            fz = zipfile.ZipFile(zip_src, 'r')
            for file in fz.namelist():
                fz.extract(file, dst_dir)
        else:
            print('This is not zip')

    if file_type == "xz":
        mkdir(dst_dir)
        tar_xz(zip_src, dst_dir)

    del_src_file(dst_dir)



def load_files_map(dir_path, name):
    name_path = os.path.join(dir_path, name)
    d = {}

    if False == os.path.exists(name_path):
        return d

    with open(name_path, "r") as f:
        for line in f.readlines():
            line = line.strip('\n')  #去掉列表中每一个元素的换行符
            print(line)
            if not line or line.startswith("#"):
                continue
            pieces = line.split()
            if not (len(pieces) == 2):
                raise ValueError("malformed line: \"%s\"" % (line,))
            d[pieces[0]] = pieces[1]
    return d

def load_ota_file_map(dir_path, name):
    name_path = os.path.join(dir_path, name)
    k = []
    v = []

    if False == os.path.exists(name_path):
        return k,v

    with open(name_path, "r") as f:
        for line in f.readlines():
            line = line.strip('\n')  #去掉列表中每一个元素的换行符
            print(line)
            if not line or line.startswith("#"):
                continue
            pieces = line.split()
            if not (len(pieces) == 2):
                raise ValueError("malformed line: \"%s\"" % (line,))

            k.append(pieces[0])
            v.append(pieces[1])

    return k,v

def get_file(fn, ota_cfg):
  if fn not in ota_cfg:
    fn = fn.split(".")[0] + ".*"
    if fn not in ota_cfg:
      print ("warning radio-update: '%s' not found in ota_cfg" % (fn))
      return None
  return ota_cfg[fn]


def get_file_name_list(file_dir):
    #for filename in os.listdir(file_dir):
    #    print(os.path.join(file_dir, filename))
    #print("file_dir:%s." % (file_dir))

    file_dir_len = len(file_dir) + 1
    L=[]
    for root, dirs, files in os.walk(file_dir):
        for file in files:
            #if os.path.splitext(file)[1] == '.zip':  # 文件格式
            #print("root:%s." % (root))
            #print("file:%s." % (os.path.join(root, file)[file_dir_len:]))
            L.append((os.path.join(root, file)[file_dir_len:]))
    return L


def del_file(file_path):
    if os.path.exists(file_path):  # 如果文件存在
        os.remove(file_path)


def get_file_path_list(file_dir):
    file_path = os.path.join("update_incr", "files_info_"+file_dir)
    del_file(file_path)

    f = open(file_path,'w')

    file_dir_len = len(file_dir) + 1
    L=[]
    for root, dirs, files in os.walk(file_dir):
        for file in files:
            #if os.path.splitext(file)[1] == '.zip':  # 文件格式
            #print("root:%s." % (root))
            if file == "ota_cfg":
                continue
            #print("file:%s." % (os.path.join(root, file)[file_dir_len:]))
            f.write((os.path.join(root, file)[file_dir_len:]))
            f.write("\n")

    f.close()


def is_file_same(src_file, dst_file):

    #if os.path.islink(src_file):
    #    print("%s a link" % (src_file))

    if os.path.getsize(src_file) == 0:
        return False

    if os.path.getsize(dst_file) == 0:
        return False

    with open(src_file, 'rb') as f:
        src_data = f.read()

    with open(dst_file, 'rb') as f:
        dst_data = f.read()

    if len(src_data) == len(dst_data) and crc32(src_data) == crc32(dst_data):
        #print(">>>>>> %s. file the same." % (src_file))
        #print ("****************************************************************************")
        return False
    return True

def ota_file_map_write(file, cmd):
    fp = open(file,'a')
    fp.write(cmd+"\n")
    fp.close()
    return True


def get_file_cfg_list():
    get_file_path_list("dst_path")
    get_file_path_list("src_path")

    file_dst  =  open ("update_incr/files_info_"+ "dst_path",  'r')
    file_src  =  open ("update_incr/files_info_"+ "src_path",  'r')

    ota_file_map_path = "update_incr/ota_file_map"
    del_file(ota_file_map_path)

    for line_dst in file_dst.readlines():
        line_dst = line_dst.strip('\n')
        file_src.seek(0)
        add_flg = 1
        for line_src in file_src.readlines():
            line_src = line_src.strip('\n')
            #print("line_dst:%s,line_src:%s." % (line_dst, line_src))
            if line_dst == line_src:
                same = is_file_same("dst_path/"+line_dst, "src_path/"+line_src)
                if same == True:
                    print(".>>>>>>patch>>>>>>line_dst:%s." % (line_src))
                    ota_file_map_write(ota_file_map_path, "patch  "+line_src)
                    add_flg = 0
                    break
            if line_dst == line_src:
                add_flg = 0
        if add_flg:
            ota_file_map_write(ota_file_map_path, "add    "+line_dst)
            print(".>>>>>> add >>>>>>line_dst:%s." % (line_dst))

    file_src.seek(0)
    for line_src in file_src.readlines():
        line_src = line_src.strip('\n')
        del_flg = 1
        file_dst.seek(0)
        #print("line_dst:%s,line_src:%s." % (line_dst, line_src))
        for line_dst in file_dst.readlines():
            line_dst = line_dst.strip('\n')
            #print("line_dst:%s,line_src:%s." % (line_dst, line_src))
            if line_dst == line_src:
                del_flg  = 0
                break
        if del_flg:
            ota_file_map_write(ota_file_map_path, "del    "+line_src)
            print(".>>>>>> del >>>>>>line_src:%s." % (line_src))

    file_dst.close()
    file_src.close()


#定义一个函数，递归读取absDir文件夹中所有文件，并塞进zipFile文件中。参数absDir表示文件夹的绝对路径。
def writeAllFileToZip(absDir,zipFile):
    for f in os.listdir(absDir):
        absFile=os.path.join(absDir,f) #子文件的绝对路径
        if os.path.isdir(absFile): #判断是文件夹，继续深度读取。
            relFile=absFile[len(os.getcwd())+1:] #改成相对路径，否则解压zip是/User/xxx开头的文件。
            zipFile.write(relFile) #在zip文件中创建文件夹
            writeAllFileToZip(absFile,zipFile) #递归操作
        else: #判断是普通文件，直接写到zip文件中。
            relFile=absFile[len(os.getcwd())+1:] #改成相对路径
            zipFile.write(relFile)
    return



def do_fota_pre(src_path, dst_path, task_id):
    root_path = os.path.join(os.getcwd(), "")
    zip_src_path = os.path.join(root_path,src_path)
    zip_dst_path = os.path.join(root_path,dst_path)

    file_nums = 0
    file_name_list = []
    file_src_list = []
    file_dst_list = []

    is_src_file_zip = is_zip_or_xz_file(zip_src_path)
    is_dst_file_zip = is_zip_or_xz_file(zip_dst_path)


    print("is_dst_file_zip:%s, is_dst_file_zip: %s." % (is_dst_file_zip, is_dst_file_zip))
    if is_src_file_zip == is_dst_file_zip:

        un_file(src_path, "src_path")
        un_file(dst_path, "dst_path")

        ota_cfg = load_files_map("dst_path", "ota_cfg")
        if ota_cfg == {}:
            print ("warning: no or invalid ota_cfg file found!!!")
            return file_nums, file_name_list, file_src_list, file_dst_list

        flash_size = "119000"
        buf_size = "50000"
        part_num = "0"
        block_size = "1000"
        if ota_cfg != {}:
            for key_file, value_location in ota_cfg.items():
                if key_file == "flash_size":
                    flash_size = value_location
                if key_file == "buf_size":
                    buf_size = value_location
                if key_file == "part_num":
                    part_num = value_location
                if key_file == "block_size":
                    block_size = value_location

        print('format_up_bin.py:flash_size=%s,  buf_size = %s, part_num = %s, block_size = %s.' % (flash_size, buf_size, part_num, block_size))

        get_file_cfg_list()

        print('..............................................................')

        src_list = get_file_name_list("src_path")
        dst_list = get_file_name_list("dst_path")

        k_list,v_list = load_ota_file_map("update_incr", "ota_file_map")
        if len(k_list) == 0:
            print('^^^^^^^^^^^^^^^^^^^^^^No need to upgrade^^^^^^^^^^^^^^^^^^^^^^')

        print('key_=  len(k_list)=%d' % ( len(k_list)))

        index = 0
        for key_ in k_list:
            value_location = v_list[index]
            index+=1
            print('key_= %s, value_location=%s' % (key_, value_location))

            if key_ == "patch":
                print ("warning: patch.")

                print ("  ")
                print ("warning: value_location=%s, value_location=%s." % (value_location, value_location))
                file_name = value_location
                head_file = file_name +"_"+"patch_head"
                src_file_path = os.path.join("src_path",file_name)
                head_file_path = os.path.join("src_path",head_file)

                print('python ./format_up_bin.py %s %s %s %s %s %s' % (src_file_path, head_file_path, flash_size, buf_size, part_num, block_size))
                os.system('python ./format_up_bin.py %s %s %s %s %s %s' % (src_file_path, head_file_path, flash_size, buf_size, part_num, block_size))
                print ("  ")

                head_file = file_name +"_"+"patch_head"
                src_file_path = os.path.join("dst_path",file_name)
                head_file_path = os.path.join("dst_path",head_file)
                print('python ./format_up_bin.py %s %s %s %s %s %s' % (src_file_path, head_file_path, flash_size, buf_size, part_num, block_size))
                os.system('python ./format_up_bin.py %s %s %s %s %s %s' % (src_file_path, head_file_path, flash_size, buf_size, part_num, block_size))
                print ("  ")
                print ("warning: patch.")

            if key_ == "add":
                print ("warning: add.")
                os.system('cp -f ./dst_path/%s ./update_incr/' % (value_location))

            if key_ == "del":
                print ("warning: del.")

        return k_list,v_list


def do_fota_post():
    ota_file_map_path = "update_incr/ota_file_map"
    if False == os.path.exists(ota_file_map_path):
        return False

    ota_cfg = load_files_map("dst_path", "ota_cfg")
    if ota_cfg == {}:
        return False
    flg = 0
    partition_dir = []
    fs_dir = []
    if ota_cfg != {}:
        for key_file, value_location in ota_cfg.items():
            if key_file == "zone":
                flg = 1
                continue
            if flg == 1:
                partition_dir.append(key_file)
                fs_dir.append(value_location)

    del_file("update_incr/file_map")
    with open(ota_file_map_path, "r") as f:
        for line in f.readlines():
            index = 0
            line = line.strip('\n')
            for partition in partition_dir:
                line=line.replace(fs_dir[index], partition)
                index += 1
            line=line.replace('\\', '/')
            #print(">>>>>>>>>>>>>>>>> %s" % (line))
            ota_file_map_write("update_incr/file_map", line[0:7]+'/'+ line[7:])

    #del_file(ota_file_map_path)


    os.system('tar -cJvf update_incr.tar.xz update_incr')
    #os.system('rm -rf update_incr')
    #os.system('rm -rf dst_path')
    #os.system('rm -rf src_path')

def do_fota_file(src_path, dst_path, task_id):
    root_path = os.path.join(os.getcwd(), "")
    zip_src_path = os.path.join(root_path,src_path)
    zip_dst_path = os.path.join(root_path,dst_path)

    print("")
    print("root_path: %s." % (root_path))
    print("src_path: %s." % (src_path))
    print("dst_path: %s." % (dst_path))

    is_src_file_zip = is_zip_or_xz_file(zip_src_path)
    is_dst_file_zip = is_zip_or_xz_file(zip_dst_path)
    if is_src_file_zip and is_dst_file_zip and is_dst_file_zip == is_src_file_zip :
        mkdir("update_incr")

        k_list, v_list = do_fota_pre(src_path, dst_path, task_id)
        index = 0
        i = 0
        for key_ in k_list:
            value_location = v_list[i]
            i+=1
            if key_ == "patch":
                print ("warning: patch.")
                head_file = value_location
                head_file += "_patch_head"
                print ("warning: head_file. %s." % head_file)
                with open(os.path.join("src_path",head_file), 'rb') as f:
                    src_data = f.read()
                    f.close()
                with open(os.path.join("dst_path",head_file), 'rb') as f:
                    dst_data = f.read()
                    f.close()

                index+= 1
                bsdiff_data = do_bsdiff2ya(0, src_data, dst_data, task_id)
                patch_file =  value_location.split("\\")[-1]+"_patch"
                patch_file_path = os.path.join("update_incr",os.path.basename(patch_file))
                print(">>>>>>patch_file[%d]: patch_file_path:%s." % (index, patch_file_path))
                patch_io = open(patch_file_path, "wb")
                make_diff_res = MakeDiffRes()
                make_diff_res = bsdiff_data
                if make_diff_res.diff_code.value != 0:
                    logging.error("make patch err...res:{}", make_diff_res.diff_code.value)
                    patch_io.close()
                    return make_diff_res.diff_code.value

                patch_io.write(make_diff_res.str_io)
                patch_io.close()

                del_file(os.path.join("src_path",head_file) )
                del_file(os.path.join("dst_path",head_file))
                print ("warning: patch.")

        del_file(os.path.join("update_incr", "files_info_"+ "dst_path"))
        del_file(os.path.join("update_incr", "files_info_"+ "src_path"))
        return do_fota_post()

    else:
        with open(src_path, 'rb') as f:
            src_data = f.read()

        with open(dst_path, 'rb') as f:
            dst_data = f.read()

        return do_bsdiff2ya(0, src_data, dst_data, task_id)


def make_fota_auto(src_path, dst_path, patch_path, task_id):

    if is_zip_or_xz_file(src_path) == 0:
        return False

    if is_zip_or_xz_file(dst_path) == 0:
        return False

    ret = do_fota_file(src_path, dst_path, 0)

    if ret != 0:
        size = [getsize(args[i]) for i in range(2)]
        zip_size = getsize("update_incr.tar.xz")

        print()
        logging.info("src: %s, %iB." % (human_bytes(size[0]), size[0]))
        logging.info("dst: %s, %iB" % (human_bytes(size[1]), size[1]))
        logging.info("patch: %s, %iB, (%.2f%% of dst)" % (human_bytes(zip_size), zip_size, 100.0 * zip_size / size[1]))
    return True

def globalvar_init(task_id, src_name, dst_name, src_file_name, dst_file_name, src_ver, dst_ver, process_http_url, tmp_run_dir):
    globalvar._init()
    globalvar.set_value('task_id', task_id)
    globalvar.set_value('src_name', src_name)
    globalvar.set_value('dst_name', dst_name)
    globalvar.set_value('src_file_name', src_file_name)
    globalvar.set_value('dst_file_name', dst_file_name)
    globalvar.set_value('src_ver', src_ver)
    globalvar.set_value('dst_ver', dst_ver)
    globalvar.set_value('process_http_url', process_http_url)
    globalvar.set_value('tmp_run_dir', tmp_run_dir)

def main_bsdiff2ya_auto_mode(src_path, dst_path, patch_path, task_id):

    with open(src_path, 'rb') as f3:
        src_data = f3.read()

    with open(dst_path, 'rb') as f4:
        dst_data = f4.read()

    logging.info("src_data_len:{}, dst_data_len:{}, src_path:{}, dst_path:{}", len(src_data),  len(dst_data), src_path, dst_path)
    os.system('pwd')
    os.system('ls')
    patch_io = open(patch_path, "wb")
    make_diff_res = MakeDiffRes()
    make_diff_res = do_bsdiff2ya(0, src_data, dst_data, task_id)
    if make_diff_res.diff_code.value != 0:
        logging.error("make patch err...res:{}", make_diff_res.diff_code.value)
        patch_io.close()
        return make_diff_res

    patch_io.write(make_diff_res.str_io)
    patch_io.close()
    return make_diff_res

def get_diff_progress():
    gl_diff_progress = globalvar.get_value('diff_progress')
    gl_diff_progress.total_progress = gl_diff_progress.progress / \
        gl_diff_progress.all_part_num + \
        (gl_diff_progress.part_num-1) / gl_diff_progress.all_part_num
    return gl_diff_progress

def main_bsdiff2ya_auto(src_path, dst_path, patch_path, task_id):

    oriFileUrl = src_path
    tarFileUrl = dst_path
    make_diff_res = MakeDiffRes()
    if task_id > 0:
        src_name = globalvar.get_value('src_name')
        dst_name = globalvar.get_value('dst_name')

        try:
            oriFile = requests.get(oriFileUrl)
            # oriFileContent = oriFile.content
            tarFile = requests.get(tarFileUrl)
            # tarFileContent = tarFile.content
            f = open(src_name, "wb")
            for chunk1 in oriFile.iter_content(chunk_size=1):
                if chunk1:
                    f.write(chunk1)
            f.seek(0, os.SEEK_SET)
            f.close

            f1 = open(dst_name, "wb")
            for chunk2 in tarFile.iter_content(chunk_size=1):
                if chunk2:
                    f1.write(chunk2)
            f1.seek(0, os.SEEK_SET)
            f1.close
        except requests.exceptions.ConnectionError:
            make_diff_res.diff_code = ErrCode.REQUEST_BIN_ERR
            return make_diff_res

        if False == make_fota_auto(src_name, dst_name, patch_path, task_id):
            make_diff_res = main_bsdiff2ya_auto_mode(src_name, dst_name, patch_path, task_id)
    else:
        if False == make_fota_auto(src_path, dst_path, patch_path, task_id):
            make_diff_res = main_bsdiff2ya_auto_mode(src_path, dst_path, patch_path, task_id)
            # os.system('cp ./patch.bin ../../build/examples/linux_simulation/')
            # os.system('cp %s ../../build/examples/linux_simulation/' % (src_path))
            '''
            os.system('python ./format_up_bin.py %s %s_%s %s' % (src_path, src_path, "head", max_flash))
            os.system('python ./format_up_bin.py %s %s_%s %s' % (dst_path, dst_path, "head", max_flash))
            '''
    return make_diff_res

def set_globalvar_fun(src_path, dst_path, patch_path, task_id, process_http_url):
    tmp_run_dir = "tmp_run_dir/"
    src_file_name = ""
    dst_file_name = ""
    src_name = ""
    dst_name = ""

    logging.info("\r\nsrc_path:{}, \r\ndst_path:{}", src_path, dst_path)
    src_names = src_path.split("/")
    src_name = src_names[len(src_names) - 1]
    dst_names = dst_path.split("/")
    dst_name = dst_names[len(dst_names) - 1]

    logging.info("src_names:{}, \r\n***src_name:{}***", src_names, src_name)
    logging.info("dst_names:{}, \r\n***dst_name:{}***", dst_names, dst_name)

    src_file_names = src_name.split("_")
    for index in range(len(src_file_names)-2):
        src_file_name += src_file_names[index]
        if (index < (len(src_file_names)-3)):
            src_file_name += "_"
    src_ver = src_file_names[len(src_file_names)-1].split(".b")[0]
    # logging.info("\r\1--------------1:{}, 2:{}, 3:{}-----------------", len(src_file_names)-1, src_file_names[len(src_file_names)-1], src_file_names[len(src_file_names)-1].split(".b"))

    dst_file_names = dst_name.split("_")
    for index in range(len(dst_file_names)-2):
        dst_file_name += dst_file_names[index]
        if (index < (len(dst_file_names)-3)):
            dst_file_name += "_"
    dst_ver = dst_file_names[len(dst_file_names)-1].split(".b")[0]

    src_file_name = src_file_name.split("-")[len(src_file_name.split("-"))-1]
    dst_file_name = dst_file_name.split("-")[len(dst_file_name.split("-"))-1]

    logging.info("src_file_names:{}, \r\n***src_file_name:{}***", src_file_names, src_file_name)
    logging.info("dst_file_names:{}, \r\n***dst_file_name:{}***", dst_file_names, dst_file_name)
    logging.info("\r\n***src_ver:{}***, \r\n***dst_ver:{}***", src_ver, dst_ver)
    if task_id > 0:
        tmp_run_dir = ""
        #tmp_run_dir = "/home/shang/samba/tmp/ota"#云端指定存储路径
        tmp_run_dirs = patch_path.split("/")
        for index in range(len(tmp_run_dirs)-1):
            tmp_run_dir += tmp_run_dirs[index]
            tmp_run_dir += "/"
        logging.info("tmp_run_dirs:{}, \r\n***tmp_run_dir:{}***", tmp_run_dirs, tmp_run_dir)

        #https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/smart/firmware/upgradeDiff/20220428/1651132414-tuyaos_demo_quickstart_STU_1.0.1.bin

        src_name = tmp_run_dir + src_name
        dst_name = tmp_run_dir + dst_name

        logging.info("\r\n***task_id=0,url file, src_name:{}, dst_name:{}, src_file_name:{}, dst_file_name:{}, src_ver:{}, dst_ver:{}***", src_name, dst_name, src_file_name, dst_file_name, src_ver, dst_ver)
        globalvar_init(task_id, src_name, dst_name, src_file_name, dst_file_name, src_ver, dst_ver, process_http_url, tmp_run_dir)
    else:
        src_name = tmp_run_dir + src_name
        dst_name = tmp_run_dir + dst_name

        logging.info("\r\n***task_id=0,local file, src_name:{}, dst_name:{}, src_file_name:{}, dst_file_name:{}, src_ver:{}, dst_ver:{}***", src_name, dst_name, src_file_name, dst_file_name, src_ver, dst_ver)
        globalvar_init(task_id, src_name, dst_name, src_file_name, dst_file_name, src_ver, dst_ver, process_http_url, tmp_run_dir)

#测试命令
#单区
#python ./diff2ya.py https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/smart/firmware/upgradeDiff/20220428/1651132396-tuyaos_demo_quickstart_STU_1.0.0.bin https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/smart/firmware/upgradeDiff/20220428/1651132414-tuyaos_demo_quickstart_STU_1.0.1.bin ./tmp_run_dir/pattcj.bin 1
#python ./diff2ya.py ./tmp_run_dir/oem_bk7231n_light_ty_UG_10.0.0.bin_new ./tmp_run_dir/oem_bk7231n_light_ty_UG_10.0.1.bin_new ./tmp_run_dir/pattcj.bin 0
#双区
#python3 ./diff2ya.py https://public-data-daily-1254153901.cos.ap-shanghai.myqcloud.com/smart/firmware/upgradeDiff/20211213/yixitest_diff_01_ug_1.1.6.bin https://public-data-daily-1254153901.cos.ap-shanghai.myqcloud.com/smart/firmware/upgradeDiff/20211212/yixitest_diff_01_ug_1.1.7.bin ./tmp_run_dir/aaa.bin 1

if __name__ == "__main__":
    #os.chdir('bsdiff4_2ya')
    #os.system('sudo python3 setup.py install')
    #os.chdir('../')
    #os.system('pwd')
    src_name = ""
    dst_name = ""
    myRequestDiffRst = RequestDiffRst()
    make_diff_res = MakeDiffRes()
    payload = {}
    try:
        p = OptionParser(
            usage="usage: %prog [options] SRC DST PATCH",
            description=("generate a BSDIFF4-format PATCH from SRC to DST "
                        "and write it to PATCH"))

        p.add_option('-v', "--verbose",
                    action="store_true")

        opts, args = p.parse_args()

        if len(args) != 4:
            logging.error("requies 3 arguments...")

        src_path = "https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/smart/firmware/upgradeDiff/20220428/1651132396-tuyaos_demo_quickstart_STU_1.0.0.bin"
        dst_path = "https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/smart/firmware/upgradeDiff/20220428/1651132414-tuyaos_demo_quickstart_STU_1.0.1.bin"
        src_path = args[0]
        dst_path = args[1]
        patch_path = args[2]
        task_id_str = args[3]
        task_id = int(task_id_str)

        # http_url = "http://httpbin.org/post"
        # process_http_url = http_url
        # res_http_url = http_url
        process_http_url = "http://localhost:9449/task/process/update"
        res_http_url = "http://localhost:9449/task/rt/update"

        diff_progress = DiffProgress()
        diff_progress.task_id = task_id
        diff_progress.all_part_num = 1
        diff_progress.part_num = 1
        diff_progress.progress = 0

        set_globalvar_fun(src_path, dst_path, patch_path, task_id, process_http_url)

        prr = format.get_diff_progress_test(diff_progress)
        make_diff_res = main_bsdiff2ya_auto(src_path, dst_path, patch_path, task_id)

        diff_progress.progress = 1
        prr = format.get_diff_progress_test(diff_progress)

        if task_id > 1:
            myRequestDiffRst.taskId = task_id
            if make_diff_res.diff_code == ErrCode.OK:
                myRequestDiffRst.taskRt = 2
            else:
                myRequestDiffRst.taskRt = 3
            # myRequestDiffRst.taskRt = make_diff_res.diff_code.value
            myRequestDiffRst.taskErrorCode = make_diff_res.diff_code.name

            payload.update(myRequestDiffRst.__dict__)
            #payload = json.dumps(myRequestrogress)
            headers = {'content-type': "application/json"}
            r = requests.post(res_http_url, data=json.dumps(payload), headers = headers)
            logging.info("task_id:{}, r:{}, payload:{}", task_id, r.text, payload)

    except requests.exceptions.ConnectionError:
        logging.error("task_id:{}, ConnectionError,traceback:\r\n{}", task_id, traceback.format_exc(limit=None, chain=True))
    except requests.exceptions.HTTPError:
        logging.error("task_id:{}, HTTPError,traceback:\r\n{}", task_id, traceback.format_exc(limit=None, chain=True))
    except BaseException  as e:
        logging.error("task_id:{}, SyntaxError,traceback:\r\n{}", task_id, traceback.format_exc(limit=None, chain=True))
    except:
        logging.error("task_id:{}, Unknow Error,traceback:\r\n{}", task_id, traceback.format_exc(limit=None, chain=True))
    finally:
        if task_id > 0:
            src_name = globalvar.get_value('src_name')
            dst_name = globalvar.get_value('dst_name')
            os.system('rm -rf %s' % (src_name))
            os.system('rm -rf %s' % (dst_name))
        logging.info("task_id:{}, res:{}, myRequestDiffRst:{}, payload:{}", task_id, make_diff_res.diff_code.name, myRequestDiffRst, payload)

    # main_bsdiff2ya_auto(src_path, dst_path, patch_path, -1)


