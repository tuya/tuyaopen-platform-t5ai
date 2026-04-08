#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Tuya固件打包工具

此脚本用于自动化固件打包流程，包括：
- 创建输出目录
- 生成OTA升级文件
- 复制固件和调试文件
- 支持compress，partition和compatible三种OTA模式

使用方法:
    python3 tuya_package.py --app-name <name> --version <ver> --platform <platform> \\
        --project-dir <dir> --app-config <config.json> [--ci-path <path>]

Copyright © 2025 Tuya Inc.
"""

import os
import sys
import argparse
import subprocess
import shutil
import glob
from pathlib import Path

# ANSI转义码定义
RED = '\033[91m'
GREEN = '\033[92m'
CYAN = '\033[96m'
YELLOW = '\033[93m'
RESET = '\033[0m'


def print_error(message: str):
    """打印错误信息"""
    sys.stderr.write(f"{RED}错误: {message}{RESET}\n")
    sys.stderr.flush()


def print_warning(message: str):
    """打印警告信息"""
    print(f"{YELLOW}警告: {message}{RESET}")


def print_success(message: str):
    """打印成功信息"""
    print(f"{GREEN}{message}{RESET}")


def print_info(message: str):
    """打印普通信息"""
    print(f"{CYAN}{message}{RESET}")


def run_command(cmd: list, shell: bool = False, capture_output: bool = False,
                check: bool = True, silent: bool = False) -> subprocess.CompletedProcess:
    """
    执行命令

    Args:
        cmd: 命令列表或字符串
        shell: 是否使用shell执行
        capture_output: 是否捕获输出
        check: 是否检查返回码
        silent: 是否静默执行（不输出到终端）

    Returns:
        subprocess.CompletedProcess对象
    """
    try:
        if silent and not capture_output:
            result = subprocess.run(
                cmd,
                shell=shell,
                check=check,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )
        else:
            result = subprocess.run(
                cmd,
                shell=shell,
                check=check,
                capture_output=capture_output,
                text=True
            )
        return result
    except subprocess.CalledProcessError as e:
        print_error(f"命令执行失败: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
        if capture_output and e.stderr:
            print_error(e.stderr)
        raise


def ensure_dir(path: str):
    """确保目录存在，不存在则创建"""
    Path(path).mkdir(parents=True, exist_ok=True)


def copy_files(src_pattern: str, dst_dir: str):
    """
    复制文件（支持通配符）

    Args:
        src_pattern: 源文件路径（可包含通配符）
        dst_dir: 目标目录
    """
    files = glob.glob(src_pattern)
    if not files:
        print_warning(f"没有找到匹配的文件: {src_pattern}")
        return

    ensure_dir(dst_dir)
    for file in files:
        if os.path.isfile(file):
            shutil.copy2(file, dst_dir)
            # print_info(f"  复制: {file} -> {dst_dir}")


class TuyaPackager:
    """Tuya固件打包器"""

    def __init__(self, args):
        self.app_name    = os.environ.get('TUYA_APP_NAME', '').strip()
        self.version     = os.environ.get('TUYA_APP_VERSION', '').strip()
        self.platform    = os.environ.get('TUYA_APP_PLATFORM', '').strip() or 'bk7258'
        self.project_dir = os.environ.get('TUYA_PROJECT_DIR', '').strip()
        # TuyaOpen: TUYA_APP_CONFIG_FILE 在 TuyaOpen 中不存在，设为可选
        self.app_config  = os.environ.get('TUYA_APP_CONFIG_FILE', '').strip()
        self.verbose     = args.verbose

        # 检查必需的环境变量
        if not self.app_name:
            raise RuntimeError("环境变量 TUYA_APP_NAME 未设置")
        if not self.version:
            raise RuntimeError("环境变量 TUYA_APP_VERSION 未设置")
        if not self.project_dir:
            raise RuntimeError("环境变量 TUYA_PROJECT_DIR 未设置")

        if self.verbose:
            print_info(f"应用名称: {self.app_name}")
            print_info(f"版本号: {self.version}")
            print_info(f"平台: {self.platform}")
            print_info(f"项目目录: {self.project_dir}")
            if self.app_config:
                print_info(f"配置文件: {self.app_config}")

        # TuyaOpen: TUYA_PROJECT_DIR 指向 <TuyaOpen root>/platform/T5AI
        self.work_dir = os.path.join(self.project_dir, "t5_os")
        # TuyaOpen: TuyaOpen 根目录 = TUYA_PROJECT_DIR/../..
        self.open_root = os.path.abspath(os.path.join(self.project_dir, "..", ".."))
        os.chdir(self.work_dir)

        # 设置路径
        self.tuya_app_demo_path = "projects/tuya_app"
        self.scripts_dir = f"{self.tuya_app_demo_path}/tuya_scripts"

        # 设置输出路径
        # TuyaOpen: 优先使用 TUYA_BIN_OUTPUT_DIR（由 build_example.py 传入的 dist 目录）
        tuyaopen_output = os.environ.get('TUYA_BIN_OUTPUT_DIR', '').strip()
        if tuyaopen_output:
            self.output_path = tuyaopen_output
        else:
            self.output_path = os.path.join(
                self.open_root, f"apps/{self.app_name}/output/{self.version}"
            )

        self.debug_path = os.path.join(self.output_path, "debug")

        # 工具路径
        self.tools = {
            'diff2ya': f"{self.scripts_dir}/diff2ya",
            'format_bin': f"{self.scripts_dir}/format_up_bin.py",
            'create_ua': f"{self.scripts_dir}/create_ua_file.py",
            'parse_flash': f"{self.scripts_dir}/parse_flash_table.py",
            'get_section': f"{self.scripts_dir}/get_map_section.py",
            'smp_ota': f"{self.scripts_dir}/create_ug_file.py",
            # 'smp_ota': f"{self.scripts_dir}/create_ug_file",
            'ota_select': f"{self.scripts_dir}/user_ota_select.py",
        }

        # 文件路径
        self.files = {
            'bl2_bin': "projects/tuya_app/tuya_scripts/files/bl2.bin",
            'cp_bin': f"build/{self.platform}/tuya_app/{self.platform}/app.bin",
            'ap_bin': f"build/{self.platform}/tuya_app/{self.platform}_ap/app.bin",
            'all_bin': f"build/{self.platform}/tuya_app/package/all-app.bin",
            'ua_bin': f"build/{self.platform}/tuya_app/package/ua_file.bin",
            'ug_bin': f"build/{self.platform}/tuya_app/package/ug_file.bin",
            'ty_ota': f"build/{self.platform}/tuya_app/package/ty_ug_file.bin",
            'smp_ty_ota': f"build/{self.platform}/tuya_app/package/ty_smp_ug_file.bin",
            'ap_map': f"build/{self.platform}/tuya_app/{self.platform}_ap/app.map",
            'partition': f"{self.tuya_app_demo_path}/partitions/{self.platform}/auto_partitions.csv",
        }

    def create_directories(self):
        """创建必要的目录"""
        if self.verbose:
            print_info(f"创建输出目录...{self.output_path}")
        ensure_dir(self.output_path)
        ensure_dir(self.debug_path)
        ensure_dir(os.path.join(self.debug_path, self.platform))
        ensure_dir(os.path.join(self.debug_path, f"{self.platform}_ap"))

    def get_section_offset(self, map_file: str, section: str) -> int:
        """获取section偏移地址"""
        result = run_command(
            ['python3', self.tools['get_section'], map_file, section],
            capture_output=True
        )
        return int(result.stdout.strip())

    def get_ota_mode(self) -> str:
        """获取OTA模式"""
        # TuyaOpen: 无 app_config 时默认使用压缩模式
        if not self.app_config or not os.path.isfile(self.app_config):
            return "compress"
        result = run_command(
            ['python3', self.tools['ota_select'], self.app_config],
            capture_output=True
        )
        return result.stdout.strip()

    def get_partition_size(self, partition_name: str) -> int:
        """
        获取分区大小

        Args:
            partition_name: 分区名称（如 'ota'）

        Returns:
            分区大小（字节）
        """
        # 保存当前目录
        current_dir = os.getcwd()
        try:
            # 获取绝对路径
            parse_flash_abs = os.path.abspath(self.tools['parse_flash'])

            # 切换到分区表所在目录
            partition_dir = os.path.dirname(self.files['partition'])
            os.chdir(partition_dir)

            # 执行命令获取分区大小（使用绝对路径）
            result = run_command(
                ['python3', parse_flash_abs, partition_name, 'size'],
                capture_output=True
            )
            return int(result.stdout.strip())
        finally:
            # 恢复原目录
            os.chdir(current_dir)

    def calculate_split_point(self) -> int:
        """获取涂鸦段信息"""
        if self.verbose:
            print_info("获取涂鸦段信息...")
        ap_ty_section_addr = self.get_section_offset(self.files['ap_map'], '_ty_section_start')
        ap_start_section_addr = self.get_section_offset(self.files['ap_map'], '__vector_core0_table')

        """bl2 + cp + a-section"""

        bl2_physical_size = self.get_partition_size('primary_tuyaboot')
        bl2_raw_size = int((bl2_physical_size * 32) / 34)

        cp_physical_size = self.get_partition_size('primary_cp_app')
        cp_raw_size = int((cp_physical_size * 32) / 34)

        split_point =  bl2_raw_size + cp_raw_size + ap_ty_section_addr - ap_start_section_addr

        print_info(f"ap_start_section_addr: {ap_start_section_addr}")
        print_info(f"ap_ty_section_addr: {ap_ty_section_addr}")
        print_info(f"split_point: {split_point}")

        return split_point

    def create_ua_file(self):
        """创建UA文件"""
        if self.verbose:
            print_info("创建UA文件...")
        run_command([
            'python3', self.tools['create_ua'],
            self.files['partition'],
            self.files['bl2_bin'],
            self.files['cp_bin'],
            self.files['ap_bin'],
            f"--ua_file={self.files['ua_bin']}"
        ])

    def generate_ota_files(self, split_point: int, ota_size: int, ota_mode: str):
        """
        生成OTA升级文件

        Args:
            split_point: CP+AP分割点
            ota_size: OTA分区大小（从分区表获取）
            ota_mode: OTA模式（compress/partition/compatible）
        """
        print_info(f"生成OTA文件 (模式: {ota_mode}, OTA分区大小: {ota_size})...")

        # TuyaOpen: 仅支持压缩模式
        if ota_mode == "compress":
            cmd = [
                # self.tools['smp_ota'],
                sys.executable, self.tools['smp_ota'],
                self.files['ua_bin'],
                '-o', self.files['smp_ty_ota'],
                '--total-size', '8024064',
                '--ota-size', str(ota_size),
                '--a-size', str(split_point),
                '--gap-size', '524288',
                '-m', '1'
            ]
            if self.verbose:
                cmd.append('-v')
            run_command(cmd)

        else:
            raise ValueError(f"不支持的OTA模式: {ota_mode}")

    def copy_output_files(self):

        # 复制主固件文件
        output_files = [
            (self.files['all_bin'], f"{self.app_name}_QIO_{self.version}.bin"),
            (self.files['ua_bin'], f"{self.app_name}_UA_{self.version}.bin"),
            # (self.files['ty_ota'], f"{self.app_name}_UG_OLD_{self.version}.bin"),
            (self.files['smp_ty_ota'], f"{self.app_name}_UG_{self.version}.bin"),
        ]

        for src, dst_name in output_files:
            dst = os.path.join(self.output_path, dst_name)
            shutil.copy2(src, dst)
            if self.verbose:
                print_info(f"  {src} -> {dst}")

    def copy_debug_files(self):

        cp_debug = os.path.join(self.debug_path, self.platform)
        ap_debug = os.path.join(self.debug_path, f"{self.platform}_ap")

        # CP调试文件
        copy_files(f"build/{self.platform}/tuya_app/{self.platform}/app*", cp_debug)
        copy_files(f"build/{self.platform}/tuya_app/{self.platform}/size_map*", cp_debug)
        copy_files(f"build/{self.platform}/tuya_app/{self.platform}/sdkconfig", cp_debug)
        copy_files(f"projects/tuya_app/cp/config/{self.platform}/*", cp_debug)

        # AP调试文件
        copy_files(f"build/{self.platform}/tuya_app/{self.platform}_ap/app*", ap_debug)
        copy_files(f"build/{self.platform}/tuya_app/{self.platform}_ap/size_map*", ap_debug)
        copy_files(f"build/{self.platform}/tuya_app/{self.platform}_ap/sdkconfig", ap_debug)
        copy_files(f"projects/tuya_app/ap/config/{self.platform}_ap/*", ap_debug)

        # 分区表
        copy_files(f"projects/tuya_app/partitions/{self.platform}/*", self.debug_path)

    def package(self):
        """执行打包流程"""
        try:
            if self.verbose:
                print_info("开始固件打包")

            # 检查all-app.bin是否存在
            if not os.path.exists(self.files['all_bin']):
                print_error(f"编译产物不存在: {self.files['all_bin']}")
                return 1

            # 创建目录
            self.create_directories()

            # 计算分割点
            split_point = self.calculate_split_point()

            # 创建UA文件
            self.create_ua_file()

            # 获取OTA分区大小
            ota_size = self.get_partition_size('ota')
            if self.verbose:
                print_info(f"OTA分区大小: {ota_size} 字节")

            # 获取OTA模式
            ota_mode = self.get_ota_mode()

            # 生成OTA文件
            self.generate_ota_files(split_point, ota_size, ota_mode)

            if self.verbose:
                print_info("输出文件...")
            # 复制输出文件
            self.copy_output_files()

            # 复制调试文件
            self.copy_debug_files()

            # 打印成功信息
            if self.verbose:
                print_info("固件打包完成")
                print_success(f"\n输出目录: {self.output_path}")

            return 0

        except Exception as e:
            print_error(f"打包失败: {str(e)}")
            if self.verbose:
                import traceback
                traceback.print_exc()
            return 1


def tuya_package_process():
    """主函数 - 可被其他脚本调用（从环境变量读取参数）"""
    parser = argparse.ArgumentParser(
        description='Tuya固件打包工具',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='显示详细信息'
    )

    # 使用 parse_known_args() 而不是 parse_args()，忽略未知参数
    # 这样从 Make 调用时不会因为额外参数而报错
    args, unknown = parser.parse_known_args()

    if unknown and args.verbose:
        print_warning(f"忽略未知参数: {' '.join(unknown)}")

    packager = TuyaPackager(args)
    return packager.package()

def firmware_info():
    """
    获取固件信息（用于 bk_build_package.py）

    Returns:
        固件信息字符串，格式：app_name_version
    """
    # 直接从环境变量读取，避免创建完整的 TuyaPackager 实例
    app_name = os.environ.get('TUYA_APP_NAME', '').strip()
    version = os.environ.get('TUYA_APP_VERSION', '').strip()

    if not app_name or not version:
        # 如果环境变量未设置，返回空字符串
        return ""

    return f"{app_name}_{version}\n"

def main():
    tuya_package_process()

if __name__ == "__main__":
    sys.exit(main())
