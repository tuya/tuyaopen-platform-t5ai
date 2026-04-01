#! /usr/bin/env python3
# vim:fenc=utf-8
#
# Copyright © 2025 cc <cc@tuya>
#
# Distributed under terms of the TUYA license.

import struct
import click
import zlib
import lzma
import os


def add_package_header(original_data, compress_data, ota_type, mode, verbose=False):
    """
    UINT_T magic_ver;               // 0x4D4D4D:固定64字节
    UINT_T ota_type;                // OTA类型
                                    // 压缩升级         0x4C5A4350
                                    // 分段压缩升级     0x4C5A4352
                                    // 可覆盖分段压缩   0x4C5A4351
    UINT_T src_crc32;               // src bin crc32
    UINT_T dst_crc32;               // dst bin crc32
    UINT_T src_length;              // src bin length
    UINT_T dst_length;              // dst bin length
    UINT_T bin_offset;              // bin offset
    UINT_T bin_start_addr;          // bin start addr in ROM    0
    UINT_T patch_length;            // patch length
    UINT_T patch_crc32;             // patch crc32 checksum
    UINT_T step;                    // maxlen + step            not support, set 0
    UINT_T buf_size;                // buf_size                 not support, set 0
    UCHAR_T mode;                   // 0/10:forward   1/11:backward    (10/11为支持还原速度优化)        not support, set 0
    UCHAR_T file_index;             // 文件序号                                 not support, set 0
    USHORT_T min_supp_ver;          // 云端差分程序支持的最低还原代码版本         not support, set 0
    UINT_T attr1;                   // bin_type属性值1                          not support, set 0
                                    // (覆盖)AB分段差分ABSI: B整包固件偏移位置, B分区不写入校验值, 使用bin_crc32整包校验        not support, set 0
    UINT_T attr2;                   // bin_type属性值2                          not support, set 0
    UINT_T extend_len;              // 延长部分的字节数                         not support, set 0
    """
    # Calculate CRC32 checksum
    compress_crc32 = zlib.crc32(compress_data) & 0xffffffff

    # 计算crc32时候统一处理，不跳过bl2，从bl2的起始地址开始，到ap结束位置
    # SMP ---> SMP
    #   1、压缩升级，bl2 + app
    #   2、覆盖分段升级，bl2 + A段
    # AMP ---> SMP
    #   1、覆盖分段升级，bl2 + A段
    original_crc32 = zlib.crc32(original_data) & 0xffffffff

    attr1 = 0
    if ota_type == 0x4C5A4351:  # 覆盖分段
        attr1 = len(original_data)

    if verbose:
        click.echo(f"ota_type:                      {ota_type:#x}")
        click.echo(f"original_crc32:                {original_crc32:#x}")
        click.echo(f"len(original_data):            {len(original_data):#x}")
        click.echo(f"compress_crc32:                {compress_crc32:#x}")
        click.echo(f"len(compress_data):            {len(compress_data):#x}")
        click.echo(f"attr1:                         {attr1:#x}")

    # Pack the header according to the structure definition
    # Format: 12 unsigned ints (I), 1 unsigned char (B), 15 padding bytes (15x)
    patch_header = struct.pack("<16I",
                               0x004D4D4D,                          # magic_ver
                               ota_type,                            # ota_type
                               original_crc32,                      # src_crc32
                               original_crc32,                      # dst_crc32
                               len(original_data),                  # src_length
                               len(original_data),                  # dst_length
                               64,                                  # bin_offset       固定64字节头部，不使用扩展头
                               0,                                   # bin_start_addr
                               len(compress_data),                  # patch_length
                               compress_crc32,                      # patch_crc32
                               0,                                   # step
                               0x1000,                              # buf_size
                               0x20000,                             # min_supp_ver:file_index:mode
                               attr1,                               # attr1
                               0,                                   # attr2
                               0)                                   # extend_len
    return (patch_header + compress_data)

def lz_compress(src):
    filterz = [{  "id": lzma.FILTER_LZMA2,
                  "dict_size": 1024*4,
                  "depth": 0,
                  "mode": lzma.MODE_NORMAL,
                  "nice_len": 273,
                  "lc": 3,
                  "lp": 0,
                  "pb": 2,
                  "mf": lzma.MF_BT4
              }]
    return lzma.compress(data=src, format=lzma.FORMAT_XZ, check=lzma.CHECK_CRC32, filters=filterz)

@click.command()
@click.argument('input_file', type=click.Path(exists=True))
@click.option('-o', '--output', 'output_file',  required=True, type=click.Path(), help='输出OTA包文件路径')
@click.option('--total-size',   'total_size',   required=True, type=click.INT, help='固件区大小')
@click.option('--ota-size',     'ota_size',     required=True, type=click.INT, help='OTA区大小')
@click.option('--a-size',       'a_size',       required=True, type=click.INT, help='A段大小')
@click.option('--gap-size',     'gap_size',     required=True, type=click.INT, help='预留空间大小')
@click.option('-m',             'mode',         required=True, type=click.INT, help='打包模式: 1-强制压缩 2-强制覆盖分段压缩')
@click.option('-v',             '--verbose',    is_flag=True,  help='详细输出模式')
@click.help_option('-h', '--help', help='显示此帮助信息')
def main(input_file, output_file, total_size, ota_size, a_size, gap_size, mode, verbose):
    if verbose:
        click.echo("========================================")
        click.echo(f"输入文件:      {input_file}")
        click.echo(f"输出文件:      {output_file}")
        click.echo(f"固件区大小:    {total_size:#x}")
        click.echo(f"OTA区大小:     {ota_size:#x}")
        click.echo(f"A段大小:       {a_size:#x}")
        click.echo(f"gap size:      {gap_size:#x}")
        click.echo(f"打包模式:      {mode:#x}")
        click.echo("========================================")

    with open(input_file, 'rb') as in_file:
        all_app = in_file.read()

        # 1. 生成全量升级包，对比升级包大小与ota分区大小，如果强制分段，不用提前生成全量包
        if mode == 1:
            all_app_compress = lz_compress(all_app)
            compress_ug = add_package_header(all_app, all_app_compress, 0x4C5A4350, mode, verbose)
            if len(compress_ug) < ota_size:
                click.echo("使用全量压缩升级 (0x4C5A4350)")
                with open(output_file, 'wb') as out_file:
                    out_file.write(compress_ug)
            else:
                raise click.ClickException(f"压缩包过大,无法生成OTA包. 压缩后大小: {len(compress_ug)}, 可用: {ota_size}")
        elif mode == 2:
            # a: SMP 可覆盖分段压缩升级
            # b: 从 AMP 升级到 SMP，必须使用可覆盖分段压缩升级
            # Warning
            #   #1:  不支持分段压缩升级方式，即便A段升级包小于ota分区大小
            #        因为如果A段变化较大，本次生产的A段升级包大小大于运行中固件的OTA分区大小，会导致B段被擦除
            #   #2:  A段固件仅包含ota升级功能，其大小变化较稳定，即便后期添加其他升级方式，如以太网/4G等，相关驱动
            #        已经集成在A段固件中，仅业务层的逻辑修改，不会导致A段固件剧增，因为不知道前版本固件A段大小，
            #        保险起见，A段后面都预留部分空间，生成的压缩包需要小于A_SIZE + GAP_SIZE
            #
            # 2. A段生成升级包，比较生成的升级包与 A段及预留区域 外的剩余空间大小
            #    如果该次生成的升级包小于(total_size-(a_size + gap_size))大小，则执行可覆盖分段压缩升级 0x4C5A4352

            first_segment = all_app[:a_size]
            first_segment_compress = lz_compress(first_segment)
            compress_ug = add_package_header(first_segment, first_segment_compress, 0x4C5A4351, mode, verbose)

            actually_a_size = int((a_size/32) * 34)
            if len(compress_ug) < (total_size - (actually_a_size + gap_size)):
                click.echo("使用可覆盖分段压缩升级 (0x4C5A4351)")
                # 可覆盖分段压缩：包头 + 第一段压缩包 + a_size到all_app结尾的原始数据
                second_segment = all_app[a_size:]
                with open(output_file, 'wb') as out_file:
                    out_file.write(compress_ug)
                    out_file.write(second_segment)
                # for test
                test_file = os.path.join(os.path.dirname(output_file), "sa_compress_file.xz")
                with open(test_file, 'wb') as out_file:
                    out_file.write(first_segment_compress)
            else:
                raise click.ClickException(f"压缩包过大,无法生成OTA包. 压缩后大小: {len(first_segment_compress)}, 可用: {total_size - (a_size + gap_size)}")
        else:
            raise click.ClickException(f"ERROR MODE: {mode}, 1-强制压缩 2-强制覆盖分段压缩")


if __name__ == '__main__':
    main()
