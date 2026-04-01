#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
分区表选择工具

此脚本用于根据应用配置文件中的OTA模式，自动选择并复制对应的分区表文件。
- compress模式：使用tuya_ota_compress_table.csv（纯压缩模式）
- partition模式：使用tuya_ota_partition_table.csv（分段模式）

功能说明：
1. 解析 app_resource_config.json 文件，获取ota模式
2. 判断目标文件 auto_partitions.csv 是否与源文件相同
3. 如果不相同则覆盖替换文件，并在t5_os目录下执行make clean操作
4. 如果文件相同则不做任何操作

使用方法:
    python3 tuya_partition_table_select.py <app_config.json>

示例:
    python3 vendor/T5/t5_os/projects/tuya_app/tuya_scripts/tuya_partition_table_select.py \\
        apps/t5_common_user_config_ai_baseline_ty/app_resource_config.json

注意：需要设置环境变量 TUYA_PROJECT_DIR 指向工程根目录

Copyright © 2025 Tuya Inc.
"""

import json
import os
import sys
import shutil
import subprocess
import filecmp
import click

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
        OTA模式值（compress 或 partition）
        
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
    
    if mode not in ['compress', 'partition']:
        raise ValueError(f"不支持的OTA模式: {mode}，仅支持 'compress' 或 'partition'")
    
    return mode


def select_partition_table(mode: str, verbose: bool = False) -> bool:
    """
    根据OTA模式选择并复制分区表文件
    
    Args:
        mode: OTA模式（compress 或 partition）
        verbose: 是否输出详细信息
        
    Returns:
        True: 处理成功（文件相同或复制成功）
        False: 处理失败
    """
    # 获取工程根目录
    project_dir = os.environ.get('TUYA_PROJECT_DIR')
    if not project_dir:
        print_error("环境变量 TUYA_PROJECT_DIR 未设置")
        return False
    
    # 固定平台为 bk7258
    platform = 'bk7258'
    
    # 确定源文件
    if mode == 'compress':
        source_file = 'tuya_ota_compress_table.csv'
        mode_desc = '纯压缩模式'
    elif mode == 'partition':
        source_file = 'tuya_ota_partition_table.csv'
        mode_desc = '分段模式'
    else:
        print_error(f"不支持的OTA模式: {mode}")
        return False
    
    # 构建完整路径（基于工程根目录的绝对路径）
    source_path = os.path.join(project_dir, 'vendor/T5/t5_os/projects/tuya_app/tuya_scripts/files', source_file)
    dest_dir = os.path.join(project_dir, 'vendor/T5/t5_os/projects/tuya_app/partitions', platform)
    dest_path = os.path.join(dest_dir, 'auto_partitions.csv')
    t5_os_dir = os.path.join(project_dir, 'vendor/T5/t5_os')
    
    # 规范化路径
    source_path = os.path.normpath(source_path)
    dest_dir = os.path.normpath(dest_dir)
    dest_path = os.path.normpath(dest_path)
    t5_os_dir = os.path.normpath(t5_os_dir)
    
    if verbose:
        print_info(f"OTA模式: {mode} ({mode_desc})")
        print_info(f"源文件: {source_path}")
        print_info(f"目标文件: {dest_path}")
    
    # 检查源文件是否存在
    if not os.path.exists(source_path):
        print_error(f"源文件不存在: {source_path}")
        return False
    
    # 检查目标目录是否存在
    if not os.path.exists(dest_dir):
        print_error(f"目标目录不存在: {dest_dir}")
        return False
    
    # 判断文件是否相同
    files_are_same = False
    if os.path.exists(dest_path):
        files_are_same = filecmp.cmp(source_path, dest_path, shallow=False)
        if verbose:
            if files_are_same:
                print_info("目标文件与源文件内容相同，无需更新")
            else:
                print_info("目标文件与源文件内容不同，需要更新")
    else:
        if verbose:
            print_info("目标文件不存在，需要创建")
    
    # 如果文件相同，不做任何操作
    if files_are_same:
        return True
    
    # 文件不同，需要覆盖并执行make clean
    try:
        # 复制文件
        shutil.copy2(source_path, dest_path)
        print_success(f"成功更新分区表文件 ({mode_desc})")
        if verbose:
            print_success(f"{source_file} -> auto_partitions.csv")
        
        # 执行 make clean
        if verbose:
            print_info(f"在 {t5_os_dir} 目录执行 make clean...")
        
        result = subprocess.run(
            ['make', 'clean'],
            cwd=t5_os_dir,
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            print_success("make clean 执行成功")
            if verbose and result.stdout:
                print_info(f"输出: {result.stdout}")
            return True
        else:
            print_error(f"make clean 执行失败，返回码: {result.returncode}")
            if result.stderr:
                print_error(f"错误: {result.stderr}")
            return False
            
    except Exception as e:
        print_error(f"处理失败: {str(e)}")
        return False


@click.command()
@click.argument('config_file', type=click.Path(exists=True))
@click.option('-v', '--verbose', is_flag=True, help='详细输出模式')
@click.help_option('-h', '--help', help='显示此帮助信息')
def main(config_file, verbose):
    """
    根据配置文件中的OTA模式自动选择并复制对应的分区表文件
    
    CONFIG_FILE: 应用配置文件路径（app_resource_config.json）
    
    注意：需要设置环境变量 TUYA_PROJECT_DIR
    """
    try:
        # 检查环境变量
        project_dir = os.environ.get('TUYA_PROJECT_DIR')
        if not project_dir:
            print_error("环境变量 TUYA_PROJECT_DIR 未设置")
            sys.exit(1)
        
        if verbose:
            print_info("=" * 60)
            print_info(f"工程目录: {project_dir}")
            print_info(f"配置文件: {config_file}")
            print_info(f"目标平台: bk7258")
            print_info("=" * 60)
        
        # 加载配置文件
        config = load_config(config_file)
        
        # 获取OTA模式
        mode = get_ota_mode(config)
        
        # 选择并复制分区表
        if select_partition_table(mode, verbose):
            if verbose:
                print_info("=" * 60)
            sys.exit(0)
        else:
            sys.exit(1)
            
    except FileNotFoundError as e:
        print_error(str(e))
        sys.exit(1)
    except json.JSONDecodeError as e:
        print_error(f"JSON解析失败: {str(e)}")
        sys.exit(1)
    except ValueError as e:
        print_error(str(e))
        sys.exit(1)
    except Exception as e:
        print_error(f"未知错误: {str(e)}")
        sys.exit(1)


if __name__ == '__main__':
    main()
