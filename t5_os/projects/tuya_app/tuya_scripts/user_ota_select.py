#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OTA模式选择工具

此脚本用于从应用配置文件中解析OTA升级模式。
支持compress（压缩模式）和compatible（兼容模式）。

使用方法:
    python3 user_ota_select.py <app_config.json>

示例:
    python3 vendor/T5/t5_os/projects/tuya_app/tuya_scripts/user_ota_select.py \
        apps/t5_common_user_config_ai_baseline_ty/app_resource_config.json

输出:
    成功时输出mode的值（compress 或 compatible），退出码为0
    失败时输出错误信息到stderr，退出码为1

Copyright © 2025 Tuya Inc.
"""

import json
import os
import sys
import argparse

# ANSI转义码定义
RED = '\033[91m'
GREEN = '\033[92m'
CYAN = '\033[96m'
YELLOW = '\033[93m'
RESET = '\033[0m'


def print_error(message: str):
    """打印错误信息到stderr"""
    sys.stderr.write(f"{RED}错误: {message}{RESET}\n")
    sys.stderr.flush()


def print_info(message: str):
    """打印普通信息到stderr"""
    sys.stderr.write(f"{CYAN}{message}{RESET}\n")
    sys.stderr.flush()


def print_success(message: str):
    """打印成功信息到stderr"""
    sys.stderr.write(f"{GREEN}{message}{RESET}\n")
    sys.stderr.flush()


def load_config(config_file: str) -> dict:
    """
    加载配置文件
    
    Args:
        config_file: 配置文件路径
        
    Returns:
        解析后的JSON对象
        
    Raises:
        FileNotFoundError: 文件不存在
        json.JSONDecodeError: JSON解析失败
    """
    if not os.path.exists(config_file):
        raise FileNotFoundError(f"配置文件不存在: {config_file}")
    
    try:
        with open(config_file, 'r', encoding='utf-8') as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        raise json.JSONDecodeError(f"JSON解析失败: {str(e)}", e.doc, e.pos)


def get_ota_mode(config: dict) -> str:
    """
    从配置中获取OTA模式
    
    Args:
        config: 配置字典
        
    Returns:
        OTA模式值（compress 或 compatible）
        
    Raises:
        ValueError: 配置中没有ota_mode或mode字段
    """
    if 'ota_mode' not in config:
        raise ValueError("配置文件中未找到 'ota_mode' 字段")
    
    ota_mode = config['ota_mode']
    
    if not isinstance(ota_mode, dict):
        raise ValueError("'ota_mode' 字段格式错误，应为对象")
    
    if 'mode' not in ota_mode:
        raise ValueError("'ota_mode' 中未找到 'mode' 字段")
    
    mode = ota_mode['mode']
    
    if not mode or not isinstance(mode, str):
        raise ValueError("'mode' 字段值无效")
    
    # 验证mode值是否合法
    valid_modes = ['compress', 'partition', 'compatible']
    if mode not in valid_modes:
        raise ValueError(f"'mode' 值无效: '{mode}'，必须是 {' 或 '.join(valid_modes)}")
    
    return mode


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='OTA模式选择工具 - 从配置文件中解析OTA升级模式',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
OTA模式说明:
  compress    - 压缩模式：SMP固件使用压缩升级模式
  partition   - 分段模式：SMP固件使用覆盖分段升级模式
  compatible  - 兼容模式：由AMP架构升级到SMP的固件，首次架构切换必须设置兼容模式

示例:
  python3 user_ota_select.py apps/t5_common_user_config_ai_baseline_ty/app_resource_config.json
  
  输出示例:
    compress
        """
    )
    
    parser.add_argument(
        'config_file',
        help='应用配置文件路径 (JSON格式)'
    )
    
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='显示详细信息'
    )
    
    args = parser.parse_args()
    
    try:
        if args.verbose:
            print_info("正在解析OTA配置...")
            print_info(f"配置文件: {args.config_file}")
        
        # 加载配置文件
        config = load_config(args.config_file)
        
        if args.verbose:
            print_info("配置文件加载成功")
        
        # 获取OTA模式
        mode = get_ota_mode(config)
        
        if args.verbose:
            print_success(f"OTA模式: {mode}")
        
        # 输出模式值到stdout（不带任何格式，便于脚本使用）
        print(mode)
        
        return 0
        
    except FileNotFoundError as e:
        print_error(str(e))
        return 1
    except json.JSONDecodeError as e:
        print_error(f"JSON解析失败: {str(e)}")
        return 1
    except ValueError as e:
        print_error(str(e))
        return 1
    except Exception as e:
        print_error(f"未预期的错误: {str(e)}")
        if args.verbose if 'args' in locals() else False:
            import traceback
            traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
