#!/usr/bin/env python3
# coding=utf-8
# 参数说明：
# $1 - params path: $1/build_param.[cmake/config/json]
# $2 - user cmd: build/clean/...

import os
import subprocess
import sys
import json

from tools.util import (
    copy_file, need_settarget, record_target,
    calc_md5sum, do_subprocess, get_system_name
)
from tools.do_with_assets import do_with_assets


def clean(build_root):
    '''
    make clean
    make clean -C ./bk_idk/
    '''
    cmd = f"cd {build_root} && make clean"
    do_subprocess(cmd)

    pass


def parser_para_file(json_file):
    if not os.path.isfile(json_file):
        print(f"Error: Not found [{json_file}].")
        return {}
    try:
        f = open(json_file, 'r', encoding='utf-8')
        json_data = json.load(f)
        f.close()
    except Exception as e:
        print(f"Parser json error:  [{str(e)}].")
        return {}
    return json_data


def set_environment(root, build_param_path, param_data):
    os.environ["TUYA_PROJECT_DIR"] = root
    os.environ["BUILD_PARAM_DIR"] = build_param_path
    os.environ["TUYA_HEADER_DIR"] = param_data["OPEN_HEADER_DIR"]
    os.environ["TUYA_LIBS_DIR"] = param_data["OPEN_LIBS_DIR"]
    os.environ["TUYA_LIBS"] = param_data["PLATFORM_NEED_LIBS"]
    os.environ["TUYA_APP_NAME"] = param_data["CONFIG_PROJECT_NAME"]
    open_root = param_data["OPEN_ROOT"]
    board_path = os.path.join(open_root, "boards", "T5AI")
    os.environ["TUYAOS_BOARD_PATH"] = board_path
    pass


def check_bootloader_bin(build_root) -> bool:
    boot_file = os.path.join(build_root, "cp",
                             "components", "bk_libs",
                             "bk7258", "bootloader",
                             "normal_bootloader",
                             "bootloader.bin")
    real_md5sum = calc_md5sum(boot_file)
    exp_md5sum = "f8f45b0779a8269fa089ac84ebd9c149"
    if real_md5sum != exp_md5sum:
        print(f"Error md5sum: real[{real_md5sum}], expectation[{exp_md5sum}]")
        return False

    return True


def setup_build(root, build_root, build_param_path, param_data):
    set_environment(root, build_param_path, param_data)

    if not check_bootloader_bin(build_root):
        print("Error: check bootloader bin failed.")
        return False

    return True


def build(build_root, target, app_name, app_ver) -> bool:
    cmd = f"cd {build_root} && make {target} PROJECT=tuya_app"

    cmd += f" APP_NAME={app_name}"
    cmd += f" APP_VERSION={app_ver}"
    cmd += " -j"

    ret = do_subprocess(cmd)

    if ret != 0:
        print("Error: Build error.")
        return False
    return True

def create_ua_file(build_root, target, ua_bin_file):
    build_path = os.path.join(build_root, "build", target, "tuya_app")
    cp_bin = os.path.join(build_path, target, "app.bin")
    ap_bin = os.path.join(build_path, f"{target}_ap", "app.bin")

    project_path = os.path.join(build_root, "projects", "tuya_app")
    partition_file = os.path.join(project_path, "partitions",\
                                  target, "auto_partitions.csv")
    create_ua_py = os.path.join(project_path, "tuya_scripts", "create_ua_file.py")

    cmd = f"python {create_ua_py} {partition_file} {cp_bin} {ap_bin}  --ua_file={ua_bin_file}"
    if 0 != do_subprocess(cmd):
        print("Error: create_ua_file.py failed.")
        return False

    return True

def get_section_offset(build_root, map_file, section_name):
    get_section_offset_py = os.path.join(build_root, "projects", "tuya_app",\
                                        "tuya_scripts", "get_map_section.py")

    try:
        result = subprocess.run(
            ["python", get_section_offset_py, map_file, section_name],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"执行脚本失败: {e.stderr}")
        return None
    except FileNotFoundError:
        print(f"错误: 未找到脚本 {get_section_offset_py}")
        return None

def create_ug_file(build_root, target, ua_bin_file, ug_bin_file):
    ap_map_file = os.path.join(build_root, "build", target, "tuya_app",\
                               f"{target}_ap", "app.map")

    ap_ty_section_addr = get_section_offset(build_root, ap_map_file, "_ty_section_start")
    ap_start_section_addr = get_section_offset(build_root, ap_map_file, "__vector_core0_table")

    # 1MB (1048576)
    split_point = int(ap_ty_section_addr, 10) - int(ap_start_section_addr, 10) + 1048576

    print(f"ap_start_section_addr: {ap_start_section_addr}")
    print(f"ap_ty_section_addr: {ap_ty_section_addr}")
    print(f"split_point: {split_point}")

    scripts_path = os.path.join(build_root, "projects", "tuya_app", "tuya_scripts")
    ota_bin_tool  = os.path.join(scripts_path, "diff2ya")
    format_bin_py = os.path.join(scripts_path, "format_up_bin.py")

    bin_path = os.path.dirname(ug_bin_file)
    tmp_bin_file = os.path.join(bin_path, "tmp_ug_file.bin")

    cmd = f"python {format_bin_py} {ua_bin_file} {tmp_bin_file} 6b8000 1000 0 1000 10D0 {split_point} -v"
    if 0 != do_subprocess(cmd):
        print("Error: format bin failed.")
        return False

    cmd = f"{ota_bin_tool} {tmp_bin_file} {tmp_bin_file} {ug_bin_file} 0"
    if 0 != do_subprocess(cmd):
        print("Error: ota bin failed.")
        return False

    return True

def copy_assets(build_root, target, param_data):
    build_path = os.path.join(build_root, "build", target, "tuya_app", "package")
    app_all_bin = os.path.join(build_path, "all-app.bin")
    if not os.path.exists(app_all_bin):
        print(f"Error: Not found {app_all_bin}.")
        return False

    ua_file_bin = os.path.join(build_path, "ua_file.bin")
    ug_file_bin = os.path.join(build_path, "ug_file.bin")

    create_ua_file(build_root, target, ua_file_bin)
    create_ug_file(build_root, target, ua_file_bin, ug_file_bin)

    app_name = param_data["CONFIG_PROJECT_NAME"]
    app_ver = param_data["CONFIG_PROJECT_VERSION"]
    output_path = param_data["BIN_OUTPUT_DIR"]
    debug_path = os.path.join(output_path, "debug")
    os.makedirs(output_path, exist_ok=True)
    os.makedirs(debug_path, exist_ok=True)
    try:
        copy_file(app_all_bin,
                  os.path.join(output_path, f"{app_name}_QIO_{app_ver}.bin"))
        copy_file(ua_file_bin,
                  os.path.join(output_path, f"{app_name}_UA_{app_ver}.bin"))
        copy_file(ug_file_bin,
                  os.path.join(output_path, f"{app_name}_UG_{app_ver}.bin"))
    except Exception as e:
        print(f"Error: copy assets: {str(e)}")
        return False

    return True


def do_with_compile(root, build_root, user_cmd,
                    target, build_param_path, param_data):
    # Setup build
    if not setup_build(root, build_root, build_param_path, param_data):
        sys.exit(1)

    # clean project
    app_target_file = os.path.join(build_root, ".app")
    app_name = param_data["CONFIG_PROJECT_NAME"]
    if "clean" == user_cmd or need_settarget(app_target_file, app_name):
        clean(build_root)
        if "clean" == user_cmd:
            sys.exit(0)

    # build project
    record_target(app_target_file, app_name)
    app_ver = param_data["CONFIG_PROJECT_VERSION"]
    if not build(build_root, target, app_name, app_ver):
        sys.exit(1)
    pass


def main():
    '''
    1. 提前配置一些环境变量
    1. 如果编译的项目变化，则清理现场
    1. 拷贝产物到输出路径中
    '''
    if len(sys.argv) < 2:
        print(f"Error: At least 2 parameters are needed {sys.argv}.")
    build_param_path = sys.argv[1]
    user_cmd = sys.argv[2]
    target = "bk7258"

    root = os.path.dirname(os.path.abspath(__file__))
    build_root = os.path.join(root, "t5_os")
    build_param_file = os.path.join(build_param_path, "build_param.json")
    param_data = parser_para_file(build_param_file)
    if not len(param_data):
        sys.exit(1)

    if "linux" == get_system_name():
        do_with_compile(root, build_root, user_cmd, target,
                        build_param_path, param_data)
    else:
        do_with_assets(root, build_root, user_cmd,
                       target, param_data)

    # copy asset
    if not copy_assets(build_root, target, param_data):
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
