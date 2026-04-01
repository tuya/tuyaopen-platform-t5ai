#!/usr/bin/env python3
"""
从ELF文件和MAP文件中提取内存使用信息的脚本

原理说明：
1. 读取链接器生成的 app_memory.txt 文件（通过 --print-memory-usage 选项生成）
2. 从 .map 文件中提取内存区域的地址信息
3. 结合两者生成格式化的内存使用报告
"""

import subprocess
import re
import sys
import os
from pathlib import Path


class ElfMemoryAnalyzer:
    """ELF文件内存分析器"""

    def __init__(self, elf_file, map_file=None, toolchain_prefix="arm-none-eabi-", toolchain_path=None, custom_order=None, toolchain_memory_usage_file=None):
        self.elf_file = Path(elf_file)
        self.map_file = Path(map_file) if map_file else self.elf_file.with_suffix('.map')
        
        # 如果指定了 toolchain_memory_usage_file，使用该文件；否则使用默认路径
        if toolchain_memory_usage_file:
            self.memory_file = Path(toolchain_memory_usage_file)
        else:
            # app_memory.txt 由链接器的 --print-memory-usage 选项生成
            self.memory_file = self.elf_file.with_suffix('').parent / 'app_memory.txt'
        
        self.toolchain_prefix = toolchain_prefix
        self.custom_order = custom_order  # 用户自定义的排序规则

        # 内存区域信息
        self.memory_regions = {}

    def _detect_toolchain(self):
        """自动检测本地工具链路径（已废弃，保留用于兼容）"""
        return None

    def parse_memory_file(self):
        """
        从 app_memory.txt 文件中解析内存使用信息
        这个文件由链接器的 --print-memory-usage 选项生成
        格式示例:
        Memory region         Used Size  Region Size  %age Used
                   FLASH:      971220 B         1 MB     92.62%
                    IRAM:        4872 B     235264 B      2.07%
        """
        if not self.memory_file.exists():
            print(f"Warning: Memory file not found: {self.memory_file}")
            return {}

        with open(self.memory_file, 'r') as f:
            content = f.read()

        # 跳过表头，解析每一行内存区域信息
        lines = content.strip().split('\n')
        
        for line in lines[1:]:  # 跳过第一行表头
            # 使用正则表达式解析每行
            # 格式: "           FLASH:      971220 B         1 MB     92.62%"
            match = re.search(r'(\w+):\s+(\d+(?:\.\d+)?)\s*([KMGT]?B)\s+(\d+(?:\.\d+)?)\s*([KMGT]?B)\s+([\d.]+)%', line)
            if match:
                name = match.group(1)
                used_value = float(match.group(2))
                used_unit = match.group(3)
                size_value = float(match.group(4))
                size_unit = match.group(5)
                usage_pct = float(match.group(6))
                
                # 转换为字节
                used_bytes = self._convert_to_bytes(used_value, used_unit)
                size_bytes = self._convert_to_bytes(size_value, size_unit)
                
                self.memory_regions[name] = {
                    'used': used_bytes,
                    'length': size_bytes,
                    'usage': usage_pct
                }

        return self.memory_regions

    def _convert_to_bytes(self, value, unit):
        """将不同单位转换为字节"""
        unit = unit.upper()
        if unit == 'B':
            return int(value)
        elif unit == 'KB':
            return int(value * 1024)
        elif unit == 'MB':
            return int(value * 1024 * 1024)
        elif unit == 'GB':
            return int(value * 1024 * 1024 * 1024)
        elif unit == 'TB':
            return int(value * 1024 * 1024 * 1024 * 1024)
        else:
            return int(value)

    def parse_map_memory_configuration(self):
        """
        从MAP文件中解析Memory Configuration信息
        这是链接器在链接时根据链接器脚本(.ld)生成的内存布局
        主要用于获取内存区域的地址信息
        """
        if not self.map_file.exists():
            print(f"Warning: MAP file not found: {self.map_file}")
            return {}

        with open(self.map_file, 'r') as f:
            content = f.read()

        # 查找Memory Configuration段
        match = re.search(r'Memory Configuration\s+Name\s+Origin\s+Length\s+Attributes\s+(.*?)\*default\*',
                         content, re.DOTALL)
        if not match:
            print("Warning: Memory Configuration not found in MAP file")
            return {}

        mem_config_text = match.group(1)

        # 解析每个内存区域的地址
        # 格式: FLASH            0x0000000002020000 0x0000000000100000 xr
        pattern = r'(\w+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\w+)'

        map_regions = {}
        for match in re.finditer(pattern, mem_config_text):
            name = match.group(1)
            origin = int(match.group(2), 16)
            length = int(match.group(3), 16)
            attributes = match.group(4)

            map_regions[name] = {
                'origin': origin,
                'length': length,
                'attributes': attributes
            }

        return map_regions

    def calculate_section_usage(self):
        """废弃方法，保留用于兼容"""
        return {}

    def parse_map_section_placement(self):
        """废弃方法，保留用于兼容"""
        return {}

    def get_section_info_by_readelf(self):
        """废弃方法，保留用于兼容"""
        pass

    def get_region_sort_key(self, region_name):
        """
        根据内存区域名称生成排序关键字

        排序策略：
        1. 首先检查用户自定义的排序规则
        2. 按优先级分类：Flash > IRAM/RAM/SWAP > ITCM/DTCM > PSRAM > SPINLOCK > 其他
        3. 同一优先级内保持原有顺序（使用索引）

        返回: (优先级类别, 索引)
        """
        # 安全检查
        if not region_name:
            return (999, 999)

        region_name_upper = region_name.upper()

        # 1. 如果用户提供了自定义排序规则（优先级最高）
        if self.custom_order and region_name in self.custom_order:
            return (self.custom_order[region_name], 0)

        # 2. 按照指定的优先级规则分类
        # 优先级定义：
        # 0 - Flash相关（FLASH, ROM, CODE等）
        # 1 - RAM相关（IRAM, RAM, SWAP等，不包括PSRAM）
        # 2 - ITCM/DTCM（紧耦合内存）
        # 3 - PSRAM（伪静态RAM）
        # 4 - SPINLOCK
        # 99 - 其他未知类型

        # 获取该区域在 memory_regions 中的索引（保持原有顺序）
        region_list = list(self.memory_regions.keys())
        index = region_list.index(region_name) if region_name in region_list else 999

        # 优先级3: PSRAM（所有包含PSRAM的区域）
        if 'PSRAM' in region_name_upper:
            return (3, index)

        # 优先级0: Flash存储类
        if any(keyword in region_name_upper for keyword in ['FLASH', 'ROM', 'CODE']):
            return (0, index)

        # 优先级1: RAM类（包括各种RAM变体，但排除PSRAM和SPINLOCK）
        if any(keyword in region_name_upper for keyword in ['RAM', 'IRAM', 'DRAM', 'SRAM', 'SWAP']):
            if 'SPINLOCK' not in region_name_upper:
                return (1, index)

        # 优先级2: ITCM/DTCM（紧耦合内存）
        if any(keyword in region_name_upper for keyword in ['ITCM', 'DTCM', 'TCM']):
            return (2, index)

        # 优先级4: SPINLOCK
        if 'SPINLOCK' in region_name_upper:
            return (4, index)

        # 优先级99: 其他未知类型
        return (99, index)

    def analyze(self):
        """
        分析内存使用情况并输出报告
        """
        # 1. 从 app_memory.txt 解析内存使用信息
        self.parse_memory_file()

        if not self.memory_regions:
            print("Error: No memory usage information found!")
            return

        # 2. 从 MAP 文件解析内存地址配置
        map_regions = self.parse_map_memory_configuration()

        # 3. 合并两个数据源
        for name, mem_info in self.memory_regions.items():
            if name in map_regions:
                mem_info['origin'] = map_regions[name]['origin']
                mem_info['attributes'] = map_regions[name].get('attributes', '')
            else:
                # 如果 MAP 文件中没有，使用默认值
                mem_info['origin'] = 0
                mem_info['attributes'] = ''

        # 4. 生成内存使用报告
        print("="*60)
        print(f"{'name':<21}{'addr':>11}{'size':>11}{'used':>10} {'usage':>7}")
        print("-"*60)

        # 使用智能排序
        sorted_regions = sorted(
            self.memory_regions.keys(),
            key=lambda x: self.get_region_sort_key(x)
        )

        for name in sorted_regions:
            info = self.memory_regions[name]
            origin = info.get('origin', 0)
            length = info['length']
            used = info['used']
            usage_pct = info['usage']

            print(f"{name:<21} 0x{origin:08x} 0x{length:08x} {used:>7} B{usage_pct:>7.2f}%")

        return self.memory_regions


def format_size(size_bytes):
    """格式化字节大小"""
    if size_bytes < 1024:
        return f"{size_bytes} B"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.2f} KB"
    else:
        return f"{size_bytes / (1024 * 1024):.2f} MB"


def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_elf_memory.py <elf_file> [map_file] [OPTIONS]")
        print("\nOptions:")
        print("  --toolchain-path PATH                指定工具链路径")
        print("  --toolchain_memory_usage_file FILE   指定链接器内存使用信息文件")
        print("\nExample:")
        print("  python3 analyze_elf_memory.py app.elf")
        print("  python3 analyze_elf_memory.py app.elf app.map")
        print("  python3 analyze_elf_memory.py app.elf --toolchain-path vendor/T5/toolchain/gcc-arm-none-eabi-10.3-2021.10")
        print("  python3 analyze_elf_memory.py app.elf --toolchain_memory_usage_file build/app_memory.txt")
        print("\n说明:")
        print("  模式1: 使用 --toolchain_memory_usage_file 参数")
        print("    直接读取链接器生成的内存使用信息文件（通过 --print-memory-usage 选项生成）")
        print("  模式2: 不使用 --toolchain_memory_usage_file 参数")
        print("    从默认路径读取 app_memory.txt，结合 MAP 文件生成内存使用报告")
        sys.exit(1)

    elf_file = sys.argv[1]
    map_file = None
    toolchain_path = None
    toolchain_memory_usage_file = None

    # 解析命令行参数
    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '--toolchain-path' and i + 1 < len(sys.argv):
            toolchain_path = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--toolchain_memory_usage_file' and i + 1 < len(sys.argv):
            toolchain_memory_usage_file = sys.argv[i + 1]
            i += 2
        elif not map_file and not sys.argv[i].startswith('--'):
            map_file = sys.argv[i]
            i += 1
        else:
            i += 1

    analyzer = ElfMemoryAnalyzer(
        elf_file, 
        map_file, 
        toolchain_path=toolchain_path,
        toolchain_memory_usage_file=toolchain_memory_usage_file
    )
    analyzer.analyze()


if __name__ == "__main__":
    main()