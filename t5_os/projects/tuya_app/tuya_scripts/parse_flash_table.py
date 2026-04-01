#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os


def parse_size(size_str):
    """解析大小字符串，转换为字节数"""
    size_str = size_str.strip().upper()
    if size_str.endswith('K'):
        return int(size_str[:-1]) * 1024
    elif size_str.endswith('M'):
        return int(size_str[:-1]) * 1024 * 1024
    else:
        # 如果没有单位，假设是字节
        return int(size_str)


def parse_offset(offset_str):
    """解析偏移地址字符串，转换为十进制数"""
    offset_str = offset_str.strip()
    if not offset_str or offset_str == '':
        return None
    if offset_str.startswith('0x') or offset_str.startswith('0X'):
        return int(offset_str, 16)
    else:
        return int(offset_str)


def align_size(size, alignment=68 * 1024):
    """对齐大小到指定边界（68K对齐用于code类型）"""
    return ((size + alignment - 1) // alignment) * alignment


def calculate_code_flash_size(bin_size):
    """计算code类型的flash占用大小: bin_size * 34/32 并68K对齐"""
    occupied = (bin_size * 34) // 32
    return align_size(occupied, 68 * 1024)


def load_partitions(csv_file):
    """加载分区表并计算所有分区的起始地址"""
    partitions = {}
    partition_order = []
    current_offset = 0
    
    with open(csv_file, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            
            # 跳过注释行和空行
            if not line or line.startswith('#'):
                continue
            
            # 跳过标题行
            if line.lower().startswith('name'):
                continue
            
            # 解析CSV行: Name,Offset,Size,Type,Read,Write
            parts = [p.strip() for p in line.split(',')]
            
            if len(parts) < 4:
                continue
            
            name = parts[0]
            offset_str = parts[1]
            size_str = parts[2]
            part_type = parts[3]
            
            # 解析大小
            size_bytes = parse_size(size_str)
            
            # 解析或计算偏移地址
            if offset_str and offset_str != '':
                # 明确指定了偏移地址
                start_addr = parse_offset(offset_str)
                current_offset = start_addr
            else:
                # 使用累积的偏移地址
                start_addr = current_offset
            
            # 根据类型计算实际flash占用大小
            if part_type.lower() == 'code':
                flash_size = calculate_code_flash_size(size_bytes)
            else:
                flash_size = size_bytes
            
            # 存储分区信息
            partitions[name] = {
                'offset': start_addr,
                'size': size_bytes,  # 原始大小
                'flash_size': flash_size,  # 实际flash占用大小
                'type': part_type
            }
            partition_order.append(name)
            
            # 更新下一个分区的偏移地址
            current_offset = start_addr + flash_size
    
    return partitions, partition_order


def main():
    if len(sys.argv) != 3:
        print("用法: python3 parse_flash_table.py <分区名> <start|size>")
        print("示例: python3 parse_flash_table.py ota start")
        print("       python3 parse_flash_table.py ota size")
        sys.exit(1)
    
    partition_name = sys.argv[1]
    action = sys.argv[2].lower()
    
    if action not in ['start', 'size']:
        print(f"错误: 未知操作 '{action}'，请使用 'start' 或 'size'")
        sys.exit(1)
    
    # 获取CSV文件路径
    script_dir = os.path.dirname(os.path.abspath(__file__))
    csv_file = os.path.join(script_dir, '../partitions/bk7258/auto_partitions.csv')
    
    if not os.path.exists(csv_file):
        print(f"错误: 找不到分区表文件: {csv_file}")
        sys.exit(1)
    
    # 加载分区表
    partitions, partition_order = load_partitions(csv_file)
    
    # 查找指定分区
    if partition_name not in partitions:
        print(f"错误: 找不到分区 '{partition_name}'")
        print("可用分区:")
        for name in partition_order:
            print(f"  {name}")
        sys.exit(1)
    
    partition_info = partitions[partition_name]
    
    if action == 'start':
        print(partition_info['offset'])
    elif action == 'size':
        print(partition_info['size'])


if __name__ == '__main__':
    main()
