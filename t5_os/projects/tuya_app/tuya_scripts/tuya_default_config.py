#! /usr/bin/env python3
# vim:fenc=utf-8
#
# Copyright © 2025 cc <cc@tuya>
#
# Distributed under terms of the MIT license.

import json
import shutil
import os
import re
import csv
import sys
from typing import Dict, List, Tuple, Optional
import inspect

# ANSI转义码定义
RED = '\033[91m'
GREEN = '\033[92m'
CYAN = '\033[96m'
YELLOW = '\033[93m'
RESET = '\033[0m'

def print_error(message: str):
    sys.stderr.write(f"{RED}Error: {message}{RESET}\n")
    sys.stderr.flush()

def print_warning(message: str):
    print(f"{YELLOW}Warning: {message}{RESET}")

def print_success(message: str):
    print(f"{GREEN}{message}{RESET}")

def print_info(message: str):
    print(f"{CYAN}{message}{RESET}")

def kbytes_str(size: int) -> str:
    return f"{size//1024}K"

def parse_size(value: str) -> int:
    try:
        value = str(value).strip().lower()
        if value in ('y', 'n', 'true', 'false'):
            raise ValueError("布尔值无法转换为尺寸")

        if value.startswith('0x'):
            return int(value, 16)
        elif value.endswith('k'):
            return int(value[:-1]) * 1024
        else:
            return int(value)
    except ValueError as e:
        raise ValueError(f"invalid '{value}': {str(e)}")

def size_to_hex_str(size: int) -> str:
    return f"0x{size:x}"

def load_flash_defaults(csv_path: str, flash_map_ver: str) -> Dict[str, int]:
    defaults = {}
    try:
        if flash_map_ver == "1.0":
            # 处理1.0版本的默认值
            defaults = {
                'cpu0_flash_size': 2040 * 1024,
                'cpu1_flash_size': 1700 * 1024,
                'cpu2_flash_size': 272 * 1024,
                'ota_flash_size': 2056 * 1024
            }
            return defaults

        # 处理2.0版本从CSV读取
        with open(csv_path, 'r') as f:
            lines = []
            header_found = False
            for line in f:
                stripped = line.strip()
                if not stripped or stripped.startswith('#'):
                    if 'Name,Offset,Size' in line:
                        lines.append(line.lstrip('#').strip())
                        header_found = True
                    continue
                if not header_found and 'Name' in line and 'Offset' in line and 'Size' in line:
                    lines.append(line.lstrip('#'))
                    header_found = True
                else:
                    lines.append(line)

            # print(f"Processed CSV lines:\n{chr(10).join(lines)}")  # 打印处理后的内容

            reader = csv.DictReader(lines)
            for row in reader:
                name = row['Name'].strip().lower()
                size_str = row['Size'].strip()
                if name == 'app':
                    defaults['cpu0_flash_size'] = parse_size(size_str)
                elif name == 'app1':
                    defaults['cpu1_flash_size'] = parse_size(size_str)
                # elif name == 'app2':
                #     defaults['cpu2_flash_size'] = parse_size(size_str)
                elif name == 'download':
                    defaults['ota_flash_size'] = parse_size(size_str)

            # required = ['cpu0_flash_size', 'cpu1_flash_size', 'cpu2_flash_size']
            required = ['cpu0_flash_size', 'cpu1_flash_size']
            missing = [k for k in required if k not in defaults]
            if missing:
                raise ValueError(f"CSV文件缺少必要分区: {', '.join(missing)}")

            print("CSV解析结果:", defaults)
            return defaults

    except Exception as e:
        print_error(f"解析CSV失败，文件路径: {csv_path}，错误详情: {str(e)}")
        raise

def get_current_config(config_path: str) -> Dict[str, Tuple[str, int]]:
    config = {}
    try:
        if not os.path.exists(config_path):
            return config
        with open(config_path, 'r') as f:
            for line in f:
                line = line.strip()
                if match := re.match(r'^(\w+)=(0[xX][0-9a-fA-F]+|[0-9]+[kK]?)$', line):
                    key = match.group(1)
                    value_str = match.group(2)
                    try:
                        parsed_value = parse_size(value_str)
                        config[key] = (value_str, parsed_value)
                    except ValueError:
                        print_warning(f"跳过非数值配置项: {key}={value_str}")
    except Exception as e:
        raise ValueError(f"读取配置文件失败: {str(e)}")
    return config

def load_json_files(file_paths: List[str], flash_defaults: Dict, sram_defaults: Dict, psram_defaults: Dict) -> Tuple[
    Dict, Dict, Dict, Dict, Optional[str]]:
    total_gpio = {}
    sram = {}
    psram = {}
    flash = {}
    flash_map_ver = "2.0"  # 默认版本

    # 处理用户配置文件不存在的情况
    if not file_paths:
        return {}, {}, {}, {}, flash_map_ver

    for file_path in file_paths:
        try:
            with open(file_path, 'r') as f:
                data = json.load(f)
        except Exception as e:
            raise ValueError(f"解析JSON文件 {file_path} 失败: {str(e)}")

        # 处理flash_map_ver
        if 'flash_map_ver' in data:
            flash_map_ver = data['flash_map_ver']

        # 处理SRAM
        if 'sram' in data:
            for key in ['cpu0_sram_size', 'cpu1_sram_size', 'cpu2_sram_size']:
                if key in data['sram']:
                    sram[key] = parse_size(str(data['sram'][key]))

        # 处理PSRAM
        if 'psram' in data:
            for key in ['cpu0_psram_base', 'cpu0_psram_size', 'cpu1_psram_size']:
                if key in data['psram']:
                    psram[key] = parse_size(str(data['psram'][key]))

        # 处理Flash
        if 'flash' in data:
            # for key in ['cpu0_flash_size', 'cpu1_flash_size', 'cpu2_flash_size']:
            for key in ['cpu0_flash_size', 'cpu1_flash_size']:
                if key in data['flash']:
                    flash[key] = parse_size(str(data['flash'][key]))

        # 处理GPIO
        for item in data.get('gpio_map', []):
            gpio_id = item['gpio_id']
            if gpio_id in total_gpio:
                total_gpio[gpio_id].update(item)
            else:
                total_gpio[gpio_id] = item.copy()

    return total_gpio, sram, psram, flash, flash_map_ver

def generate_header_file(core, items, output_dir):
    field_order = [
        'gpio_id',
        'second_func_en',
        'second_func_dev',
        'io_mode',
        'pull_mode',
        'int_en',
        'int_type',
        'low_power_io_ctrl',
        'driver_capacity'
    ]
    lines = []
    for item in items:
        try:
            values = [item[field] for field in field_order]
        except KeyError as e:
            raise KeyError(f"字段 {e} 在GPIO配置项 {item['gpio_id']} 中缺失")
        line = "    {" + ", ".join(values) + "},\\"
        lines.append(line)

    header_content = f"""#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#ifdef __cplusplus
extern "C" {{
#endif

#define GPIO_DEFAULT_DEV_CONFIG  \\
{{ \\
{chr(10).join(lines)}
}}

#ifdef __cplusplus
}}
#endif

#endif // GPIO_CONFIG_H
"""
    output_path = os.path.normpath(os.path.join(output_dir, f"usr_gpio_cfg{core}.h"))
    with open(output_path, 'w') as f:
        f.write(header_content)

def process_sram(sram_defaults: Dict, sram_raw: Dict, config_paths: Dict) -> Dict:
    total_sram = 0xA0000  # 655360 bytes

    parsed = {
        'cpu0_sram_size': sram_raw.get('cpu0_sram_size', sram_defaults['cpu0_sram_size']),
        'cpu1_sram_size': sram_raw.get('cpu1_sram_size', sram_defaults['cpu1_sram_size']),
        'cpu2_sram_size': sram_raw.get('cpu2_sram_size', sram_defaults['cpu2_sram_size']),
    }

    if parsed['cpu2_sram_size'] is not None and parsed['cpu2_sram_size'] >= 0x10000:
        raise ValueError(f"cpu2_sram_size必须小于0x10000: {size_to_hex_str(parsed['cpu2_sram_size'])}")

    sum_configured = sum(v for v in parsed.values() if v is not None)
    remaining = total_sram - sum_configured

    if remaining < 0:
        raise ValueError(f"SRAM总和超过0xA0000 (当前总和{sum_configured})")

    unconfigured = [k for k, v in parsed.items() if v is None]
    if remaining > 0 and unconfigured:
        base = (remaining // len(unconfigured)) // 1024 * 1024
        if base == 0:
            raise ValueError("剩余SRAM空间不足以分配1024对齐的大小")

        for i, key in enumerate(unconfigured):
            if key == 'cpu2_sram_size':
                max_alloc = min(base, 0x10000 - 1)
                if max_alloc <= 0:
                    raise ValueError("无法自动分配符合规则的cpu2_sram_size")
                allocated = max_alloc
            else:
                allocated = base

            allocated = allocated // 1024 * 1024
            parsed[key] = allocated
            remaining -= allocated

    total = sum(parsed.values())
    for v in parsed.values():
        if v % 1024 != 0:
            raise ValueError(f"SRAM值未1024对齐: {size_to_hex_str(v)}")
    if total != total_sram:
        raise ValueError(f"SRAM总和错误 期望0xA0000 实际0x{total:x}")
    if parsed['cpu2_sram_size'] != None and parsed['cpu2_sram_size'] >= 0x10000:
        raise ValueError(f"cpu2_sram_size必须小于0x10000: {size_to_hex_str(parsed['cpu2_sram_size'])}")

    return parsed

def process_psram(psram_default: Dict, psram_raw: Dict, config_paths: Dict) -> Dict:
    cpu0_config = get_current_config(config_paths['cpu0'])

    default_cpu0_base = cpu0_config.get('CONFIG_PSRAM_HEAP_BASE', ('0x60700000', 0x60700000))[1]
    default_cpu0_size = cpu0_config.get('CONFIG_PSRAM_HEAP_SIZE', ('0x200000', 0x200000))[1]

    user_cpu0_base = psram_raw.get('cpu0_psram_base', default_cpu0_base)
    user_cpu0_size = psram_raw.get('cpu0_psram_size', default_cpu0_size)
    user_cpu1_size = psram_raw.get('cpu1_psram_size')
    print(f'cp0:{user_cpu0_base}, cp1:{user_cpu0_size}, cp2:{user_cpu1_size}')

    max_psram = 0x61000000
    if user_cpu0_base + user_cpu0_size >= max_psram:
        raise ValueError(f"PSRAM 配置错误: {user_cpu0_base} + {user_cpu0_size} >= {max_psram}")

    available_space = max_psram - user_cpu0_base

    if user_cpu1_size is None:
        user_cpu1_size = available_space - user_cpu0_size
    else:
        total = user_cpu0_size + user_cpu1_size
        if total != available_space:
            raise ValueError(f"PSRAM总和错误: {size_to_hex_str(total)} != {size_to_hex_str(available_space)}")

    for size in [user_cpu0_size, user_cpu1_size]:
        if size % 32768 != 0:
            raise ValueError(f"PSRAM必须32K对齐: {size_to_hex_str(size)}")

    return {
        'cpu0_psram_base': user_cpu0_base,
        'cpu0_psram_size': user_cpu0_size,
        'cpu1_psram_base': user_cpu0_base + user_cpu0_size,
        'cpu1_psram_size': user_cpu1_size
    }

def process_flash(flash_data: Dict, defaults: Dict, flash_map_ver: str) -> Dict:
    if flash_map_ver == "1.0":
        total_flash = 6068 * 1024
        defaults = {
            'cpu0_flash_size': 2040 * 1024,
            'cpu1_flash_size': 1700 * 1024,
            'cpu2_flash_size': 272 * 1024,
            'ota_flash_size': 2056 * 1024
        }
    else:
        total_flash = 6880 * 1024

    # required_keys = ['cpu0_flash_size', 'cpu1_flash_size', 'cpu2_flash_size']
    required_keys = ['cpu0_flash_size', 'cpu1_flash_size']
    processed = {k: flash_data.get(k, defaults[k]) for k in required_keys}

    for key in required_keys:
        if processed[key] % (68*1024) != 0:
            raise ValueError(f"{key}必须为68K倍数: {kbytes_str(processed[key])}")

    total_used = sum(processed.values())
    ota_size = total_flash - total_used
    if ota_size < 0:
        raise ValueError(f"Flash总空间不足，超出: {-ota_size//1024}K")

    # min_ota = 0.6 * processed['cpu0_flash_size'] + 0.2 * (processed['cpu1_flash_size'] + processed['cpu2_flash_size'])
    min_ota = 0.6 * processed['cpu0_flash_size'] + 0.2 * (processed['cpu1_flash_size'])
    if ota_size < min_ota:
        raise ValueError(f"OTA空间不足: {kbytes_str(ota_size)} < {kbytes_str(min_ota)}")

    processed['ota_flash_size'] = ota_size
    return processed

def update_config(config_path: str, sram: Dict, psram: Dict, cpu: str, sram_defaults: Dict):
    current_config = get_current_config(config_path)
    changes = {}

    sram_keys = [
        ('CONFIG_CPU0_SPE_RAM_SIZE', 'cpu0_sram_size'),
        ('CONFIG_CPU1_APP_RAM_SIZE', 'cpu1_sram_size'),
        ('CONFIG_CPU2_APP_RAM_SIZE', 'cpu2_sram_size')
    ]

    for config_key, sram_key in sram_keys:
        current_val = current_config.get(config_key, (None, 0))[1]
        new_val = sram[sram_key]
        if current_val != new_val:
            changes[config_key] = new_val

    if cpu in ['0', '1']:
        if cpu == '0':
            base_key = 'CONFIG_PSRAM_HEAP_BASE'
            size_key = 'CONFIG_PSRAM_HEAP_SIZE'
            new_base = psram['cpu0_psram_base']
            new_size = psram['cpu0_psram_size']
        else:
            base_key = 'CONFIG_PSRAM_HEAP_BASE'
            size_key = 'CONFIG_PSRAM_HEAP_SIZE'
            new_base = psram['cpu1_psram_base']
            new_size = psram['cpu1_psram_size']

        current_base = current_config.get(base_key, (None, 0))[1]
        current_size = current_config.get(size_key, (None, 0))[1]
        if current_base != new_base:
            changes[base_key] = new_base
        if current_size != new_size:
            changes[size_key] = new_size

    if changes:
        with open(config_path, 'r+') as f:
            lines = []
            existing_keys = set()
            for line in f:
                line_strip = line.strip()
                key_found = False
                for key in changes:
                    if line_strip.startswith(f"{key}="):
                        hex_val = size_to_hex_str(changes[key])
                        lines.append(f"{key}={hex_val}\n")
                        existing_keys.add(key)
                        key_found = True
                        break
                if not key_found and line_strip:
                    lines.append(line)

            for key in changes:
                if key not in existing_keys:
                    hex_val = size_to_hex_str(changes[key])
                    lines.append(f"{key}={hex_val}\n")
            f.seek(0)
            f.write(''.join(lines))
            f.truncate()

def update_flash_csv(csv_path: str, new_flash: Dict, defaults: Dict, flash_map_ver: str):
    if not os.path.exists(csv_path):
        raise FileNotFoundError(f"CSV文件不存在: {csv_path}")

    changed = False
    new_lines = []
    usr_config_size = "1024K" if flash_map_ver == "2.0" else "1836K"
    ota_mgr_offset = "0x6c9000" if flash_map_ver == "2.0" else "0x5fe000"

    with open(csv_path, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            name = row[0].strip().lower()
            if name == 'app' and parse_size(row[2]) != new_flash['cpu0_flash_size']:
                row[2] = kbytes_str(new_flash['cpu0_flash_size'])
                changed = True
            elif name == 'app1' and parse_size(row[2]) != new_flash['cpu1_flash_size']:
                row[2] = kbytes_str(new_flash['cpu1_flash_size'])
                changed = True
            # elif name == 'app2' and parse_size(row[2]) != new_flash['cpu2_flash_size']:
            #     row[2] = kbytes_str(new_flash['cpu2_flash_size'])
            #     changed = True
            elif name == 'download' and parse_size(row[2]) != new_flash['ota_flash_size']:
                row[2] = kbytes_str(new_flash['ota_flash_size'])
                changed = True
            elif name == 'usr_config':
                if row[2] != usr_config_size:
                    row[2] = usr_config_size
                    changed = True
            elif name == 'ota_mgr':
                if row[1] != ota_mgr_offset:
                    row[1] = ota_mgr_offset
                    changed = True
            new_lines.append(row)

    if changed:
        with open(csv_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerows(new_lines)
        print_success("Flash分区表已更新")
    else:
        print_info("Flash分区无变化")

def copy_file(src_path, dst_path):
    if os.path.isdir(dst_path):
        dst_path = os.path.join(dst_path, os.path.basename(src_path))
    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
    shutil.copy2(src_path, dst_path)
    print(f"update file：{os.path.basename(src_path)}")

def update_flash_map_file(t5_path: str, flash_map_ver: str):
    if flash_map_ver == "1.0":
        boot_md5 = "de1a8f2f2d2a4fa7ea85ce8cd4f59619"
        boot_file = os.path.normpath(os.path.join(t5_path, "projects/tuya_app/tuya_scripts/files/bootloader.bin.v1"))
        tkl_flash_file = os.path.normpath(os.path.join(t5_path, "projects/tuya_app/tuya_scripts/files/tkl_flash.c.v1"))
    else:
        boot_md5 = "f8f45b0779a8269fa089ac84ebd9c149"
        boot_file = os.path.normpath(os.path.join(t5_path, "projects/tuya_app/tuya_scripts/files/bootloader.bin.v2"))
        tkl_flash_file = os.path.normpath(os.path.join(t5_path, "projects/tuya_app/tuya_scripts/files/tkl_flash.c.v2"))

    dst_boot = os.path.normpath(os.path.join(t5_path, "bk_idk/components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin"))
    dst_tkl_flash = os.path.normpath(os.path.join(t5_path, "../tuyaos/tuyaos_adapter/src/driver/tkl_flash.c"))

    copy_file(boot_file, dst_boot)
    copy_file(tkl_flash_file, dst_tkl_flash)

    # update md5
    md5_path = os.path.normpath(os.path.join(t5_path, "build.sh"))
    with open(md5_path, "r", encoding="utf-8") as f:
        content = f.read()

    pattern = r'(ori_value\s*=\s*["\']?)([a-fA-F0-9]{32})(["\']?)'
    replacement = rf'\g<1>{boot_md5}\g<3>'
    new_content, count = re.subn(pattern, replacement, content)
    if count == 0:
        raise ValueError("未找到ori_value变量，请检查文件内容")

    with open(md5_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"{boot_file}, {dst_boot}, {boot_md5}")
    print(f"{tkl_flash_file}, {dst_tkl_flash}, {flash_map_ver}")


def main():
    if len(sys.argv) < 3:
        print("Usage: python script.py <output_dir> <json1> [json2 ...]")
        sys.exit(1)

    print(f"{__file__}: {inspect.currentframe().f_back.f_lineno}")
    output_dir = sys.argv[1]
    json_files = sys.argv[2:]

    try:
        csv_path = os.path.normpath(os.path.join(output_dir, "bk7258/bk7258_partitions.csv"))
        config_paths = {
            'cpu0': os.path.join(output_dir, "bk7258/config"),
            'cpu1': os.path.join(output_dir, "bk7258_cp1/config"),
            'cpu2': os.path.join(output_dir, "bk7258_cp2/config")
        }

        # 加载用户配置
        total_gpio, sram_raw, psram_raw, flash_raw, flash_map_ver = load_json_files(
            json_files, {}, {}, {}
        )

        # 加载flash默认配置（根据版本）
        flash_defaults = load_flash_defaults(csv_path, flash_map_ver)

        # 加载SRAM默认配置
        cpu0_config = get_current_config(config_paths['cpu0'])
        sram_defaults = {
            'cpu0_sram_size': cpu0_config.get('CONFIG_CPU0_SPE_RAM_SIZE', ('0', 0))[1],
            'cpu1_sram_size': cpu0_config.get('CONFIG_CPU1_APP_RAM_SIZE', ('0', 0))[1],
            'cpu2_sram_size': cpu0_config.get('CONFIG_CPU2_APP_RAM_SIZE', ('0', 0))[1],
        }

        # 处理各模块配置
        sram = process_sram(sram_defaults, sram_raw, config_paths)
        psram = process_psram({}, psram_raw, config_paths)
        flash = process_flash(flash_raw, flash_defaults, flash_map_ver)

        print("\nFinal SRAM Configuration:")
        for k in ['cpu0_sram_size', 'cpu1_sram_size', 'cpu2_sram_size']:
            if sram[k] == None:
                print(f"{k}: 0x00000000")
            else:
                print(f"{k}: {kbytes_str(sram[k])}")

        print("\nFinal PSRAM Configuration:")
        print(f"CPU0: 0x{psram['cpu0_psram_base']:x} - {size_to_hex_str(psram['cpu0_psram_size'])} / {kbytes_str(psram['cpu0_psram_size'])}")
        print(f"CPU1: 0x{psram['cpu1_psram_base']:x} - {size_to_hex_str(psram['cpu1_psram_size'])} / {kbytes_str(psram['cpu1_psram_size'])}")

        print(f"\nFinal Flash Configuration<flash map version:{flash_map_ver}>:")
        # for k in ['cpu0_flash_size', 'cpu1_flash_size', 'cpu2_flash_size', 'ota_flash_size']:
        for k in ['cpu0_flash_size', 'cpu1_flash_size', 'ota_flash_size']:
            print(f"{k}: {kbytes_str(flash[k])}")

        # 更新配置文件
        update_config(config_paths['cpu0'], sram, psram, '0', sram_defaults)
        update_config(config_paths['cpu1'], sram, psram, '1', sram_defaults)
        update_config(config_paths['cpu2'], sram, psram, '2', sram_defaults)

        # 更新flash.csv
        update_flash_csv(csv_path, flash, flash_defaults, flash_map_ver)

        # bootloader.bin/tkl_flash.c 替换
        t5_path = os.path.normpath(os.path.join(output_dir, "../../../"))
        update_flash_map_file(t5_path, flash_map_ver)

        # 生成GPIO文件
        sorted_items = sorted(total_gpio.values(), key=lambda x: int(x['gpio_id'].split('_')[1]))
        core_groups = {}
        for item in sorted_items:
            core = item.get('bind_core', '0')
            core_groups.setdefault(core, []).append(item)

        for core, items in core_groups.items():
            generate_header_file(core, items, output_dir)

        print("\nConfiguration files updated successfully")

    except ValueError as e:
        print_error(str(e))
        sys.exit(1)
    except Exception as e:
        print_error(f"Unexpected Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
