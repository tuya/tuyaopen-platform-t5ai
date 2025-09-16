#!/usr/bin/env python3
# coding=utf-8

import os
import sys
import platform

from tools.util import get_system_name, copy_file
from tools.download_toolchain import (
    download_toolchain, download_bashtools
)


def copy_diff2ya(root):
    system = platform.system().lower()
    machain = platform.machine().lower()
    sys_mac = f"{system}_{machain}"
    dist_name = "diff2ya"

    if sys_mac == "linux_x86_64":
        suffix = "linux_x86_64"
    elif sys_mac == "linux_aarch64":
        suffix = "linux_aarch64"
    elif sys_mac == "darwin_x86_64":
        suffix = "darwin_x86_64"
    elif sys_mac == "darwin_arm64":
        suffix = "darwin_arm64"
    elif sys_mac.startswith("windows"):
        suffix = "windows.exe"
        dist_name = "diff2ya.exe"
    else:
        print(f"Warning: diff2ya not support [{sys_mac}]")
        return False

    diff2ya_src = os.path.join(root, "tools", "diff2ya",
                               f"diff2ya_{suffix}")
    diff2ya_dist = os.path.join(root, "t5_os", "projects", "tuya_app",
                                "tuya_scripts", dist_name)

    return copy_file(diff2ya_src, diff2ya_dist, True)


def main():
    root = os.path.dirname(os.path.abspath(__file__))
    platform_root = os.path.abspath(os.path.join(root, ".."))
    toolchain_root = os.path.join(platform_root, "tools")
    os.makedirs(toolchain_root, exist_ok=True)
    print(f"platform_root: {toolchain_root}")

    if not download_toolchain(toolchain_root):
        print("Error: download toolchain failed.")
        sys.exit(1)

    if not copy_diff2ya(root):
        print("Error: copy diff2ya tool failed.")
        sys.exit(1)

    if "windows" == get_system_name():
        # Windows need bash tools
        tools_root = os.path.join(root, "tools")
        if not download_bashtools(tools_root):
            print("Error: download bashtools failed.")
            sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
