#! /usr/bin/env python3
# vim:fenc=utf-8
#
# Copyright © 2024 cc <cc@tuya>
#
# Distributed under terms of the MIT license.

"""
创建 UA 文件（包含 BL2 + CP + AP）
"""
import os
import csv
import argparse
import sys
import hashlib

# ANSI 颜色码
CYAN = '\033[96m'
RESET = '\033[0m'

def print_info(message: str):
    """打印信息"""
    print(f"{CYAN}{message}{RESET}")

class Part_Item:
    # 类属性：存储所有的实例
    instances = []

    def __init__(self, name, start, length):
        self.name = name
        self.start = start
        self.length = length
        # 将每个新创建的实例添加到类属性中
        Part_Item.instances.append(self)

    def __repr__(self):
        return f"Part_Item(name='{self.name}', start address='{self.start}', length={self.length})"

    @classmethod
    def find_partition(cls, name=None):
        if name is None:
            print("spec partition name")
            return None

        for item in cls.instances:
            if name.lower() in item.name.lower():
                return item

        return None

# 解析 CSV 文件并返回大小
def convert_to_bytes(length_str):
    """将大小字符串转换为字节数"""
    if length_str.endswith('k') or length_str.endswith('K'):
        return (int(length_str[:-1]) * 1024)  # kB 转换为字节
    elif length_str.endswith('M') or length_str.endswith('m'):
        return int(length_str[:-1]) * 1024 * 1024  # MB 转换为字节
    elif length_str.endswith('G') or length_str.endswith('g'):
        return int(length_str[:-1]) * 1024 * 1024 * 1024  # GB 转换为字节
    return int(length_str)  # 直接返回整数

def parse_file(file):
    """解析分区表 CSV 文件"""
    last_end_address = 0
    with open(file, newline='', encoding='utf-8') as csvfile:
        csvreader = csv.reader(csvfile)

        # 逐行读取数据
        for row in csvreader:
            # 忽略以#开头的行或空行
            if not row or row[0].startswith('#'):
                continue

            # 去除每个字段的空白
            name = row[0].strip()
            start_address = row[1].strip() if len(row) > 1 else ''
            length = row[2].strip() if len(row) > 2 else '0'

            # 计算当前项的大小（字节）
            length_bytes = convert_to_bytes(length) if length else 0

            # 如果 start_address 为空，使用上一个分区的结束地址
            if start_address:
                current_start_address = start_address
                # 如果是十六进制地址，更新 last_end_address
                if start_address.startswith('0x') or start_address.startswith('0X'):
                    last_end_address = int(start_address, 16)
            else:
                current_start_address = hex(last_end_address)

            # CSV 中的 length 是 Flash 实际占用空间
            # 固件可用空间 = Flash 占用 × 32/34
            # 例如：68K Flash 占用 → 64K 固件可用空间
            Part_Item(name, current_start_address, length_bytes)

            # 更新结束地址（使用 Flash 占用大小）
            last_end_address += length_bytes

def pad_file_to_4k(path: str, max_size: int = None) -> None:
    """
    将二进制文件填充到 4096 字节的整数倍（末尾补 0xFF），且不超过 max_size。
    用于保证固件大小为 4K 对齐（LD 中 flash_pad 只对齐了结束地址，
    FLASH ORIGIN 可能未 4K 对齐，故需在拼接前对 bin 做对齐）。
    """
    size = os.path.getsize(path)
    if max_size is not None and size > max_size:
        print(f"错误: 文件 {path} 大小 ({size}) 超过分区限制 ({max_size})", file=sys.stderr)
        sys.exit(1)
    pad = (4096 - size % 4096) % 4096
    if max_size is not None:
        pad = min(pad, max_size - size)
    if pad:
        with open(path, 'ab') as f:
            f.write(bytes([0xFF] * pad))


def create_padded_file(bl2_path, cp_path, ap_path, output_path, total_length=7836 * 1024):
    """创建填充后的 UA 文件（BL2 + CP + AP）"""

    # 查找分区信息
    bl2 = Part_Item.find_partition('primary_tuyaboot')
    cp = Part_Item.find_partition('primary_cp_app')
    ap = Part_Item.find_partition('primary_ap_app')

    # 检查分区是否找到
    if not bl2:
        print("错误: 未找到 'primary_tuyaboot' 分区", file=sys.stderr)
        sys.exit(1)
    if not cp:
        print("错误: 未找到 'primary_cp_app' 分区", file=sys.stderr)
        sys.exit(1)
    if not ap:
        print("错误: 未找到 'primary_ap_app' 分区", file=sys.stderr)
        sys.exit(1)

    # bl2.length 是 Flash 占用空间，计算固件可用空间
    # 固件可用空间 = Flash 占用 × 32/34
    bl2_available = int((bl2.length * 32) / 34)
    cp_available = int((cp.length * 32) / 34)
    ap_available = int((ap.length * 32) / 34)

    # 拼接前仅将 AP bin 填充到 4K 整数倍（不超过分区大小）
    pad_file_to_4k(ap_path, ap_available)

    # 获取每个文件的实际大小（AP 为填充后大小）
    length_bl2 = os.path.getsize(bl2_path)
    length_cp = os.path.getsize(cp_path)
    length_ap = os.path.getsize(ap_path)

    # 计算需要填充的大小（填充到可用空间）
    padding_bl2_length = bl2_available - length_bl2
    padding_cp_length = cp_available - length_cp

    # 验证文件大小不超过可用空间
    if padding_bl2_length < 0:
        print(f"错误: BL2 文件大小 ({length_bl2}) 超过可用空间 ({bl2_available}，Flash占用 {bl2.length})", file=sys.stderr)
        sys.exit(1)
    if padding_cp_length < 0:
        print(f"错误: CP 文件大小 ({length_cp}) 超过可用空间 ({cp_available}，Flash占用 {cp.length})", file=sys.stderr)
        sys.exit(1)

    # 打印信息
    # bin size: 固件文件实际大小
    # real size: 填充后的固件可用空间（= available）
    # partition size: Flash 实际占用空间（= bl2.length）
    print_info(f"bl2 size {length_bl2}, limit {bl2_available}, partition size {bl2.length}, padding {padding_bl2_length}")
    print_info(f"cp size {length_cp}, limit {cp_available}, partition size {cp.length}, padding {padding_cp_length}")
    print_info(f"ap size {length_ap}, limit {ap_available}, partition size {ap.length}\n")

    with open(output_path, 'wb') as output_file:
        # 读取BL2二进制文件并写入输出文件
        with open(bl2_path, 'rb') as bl2_file:
            bl2_data = bl2_file.read(length_bl2)
            bl2_md5 = hashlib.md5(bl2_data).hexdigest()
            print_info(f"bl2 MD5: {bl2_md5}")
            output_file.write(bl2_data)

        # 写入填充
        output_file.write(bytes([0xFF] * padding_bl2_length))

        # 读取CP二进制文件并写入输出文件
        with open(cp_path, 'rb') as cp_file:
            output_file.write(cp_file.read(length_cp))

        # 写入填充
        output_file.write(bytes([0xFF] * padding_cp_length))

        # 读取AP二进制文件并写入输出文件
        with open(ap_path, 'rb') as ap_file:
            output_file.write(ap_file.read(length_ap))

    with open(f'{output_path}_raw', 'wb') as raw_output_file:
        # original ua
        with open(cp_path, 'rb') as cp_file:
            raw_output_file.write(cp_file.read(length_cp))
        raw_output_file.write(bytes([0xFF] * padding_cp_length))
        with open(ap_path, 'rb') as ap_file:
            raw_output_file.write(ap_file.read(length_ap))

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='handle arguments')
    parser.add_argument('partition_file', type=str, help='partition file')
    parser.add_argument('bl2_bin', type=str, help='bootloader2 bin file')
    parser.add_argument('cp_bin', type=str, help='cp bin file')
    parser.add_argument('ap_bin', type=str, help='ap bin file')
    parser.add_argument('--ua_file', type=str, default='ua.bin', help='ua path')

    args = parser.parse_args()

    parse_file(args.partition_file)

    create_padded_file(args.bl2_bin, args.cp_bin, args.ap_bin, args.ua_file)



