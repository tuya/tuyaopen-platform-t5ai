#!/usr/bin/env python3
# coding=utf-8

import os
import sys

from tools.util import get_system_name
from tools.download_toolchain import (
    download_toolchain, download_bashtools
)


def main():
    root = os.path.dirname(os.path.abspath(__file__))
    platform_root = os.path.abspath(os.path.join(root, ".."))
    toolchain_root = os.path.join(platform_root, "tools")
    os.makedirs(toolchain_root, exist_ok=True)
    print(f"platform_root: {toolchain_root}")

    if not download_toolchain(toolchain_root):
        print("Error: download toolchain failed.")
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
