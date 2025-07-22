#!/usr/bin/env python3
# coding=utf-8

import os
import platform
import urllib.request
import ssl

from tools.util import (
    get_country_code, rm_rf, calc_sha256,
    extract_archive,
)


def get_toolchain_package_info():
    country_code = get_country_code()
    system = platform.system().lower()
    machain = platform.machine().lower()
    sys_mac = f"{system}_{machain}"
    print(f"get package from [{country_code} {sys_mac}]")

    if country_code == "China":
        host = "https://images.tuyacn.com/smart/embed/package/tuyaopen"
        host_mac = host
    else:
        host = "https://armkeil.blob.core.windows.net/developer\
/Files/downloads/gnu-rm/10.3-2021.10"
        host_mac = "https://developer.arm.com/-/media/Files\
/downloads/gnu/13.3.rel1/binrel"

    if sys_mac == "linux_x86_64":
        name = "gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2"
        package_info = {
            "url": f"{host}/{name}",
            "name": name,
            "size": 157089706,
            "sha256": "97dbb4f019ad1650b732faffcc881689\
cedc14e2b7ee863d390e0a41ef16c9a3",
            "folder": "gcc-arm-none-eabi-10.3-2021.10"
        }
    elif sys_mac == "linux_aarch64":
        name = "gcc-arm-none-eabi-10.3-2021.10-aarch64-linux.tar.bz2"
        package_info = {
            "url": f"{host}/{name}",
            "name": name,
            "size": 168772350,
            "sha256": "f605b5f23ca898e9b8b665be208510a5\
4a6e9fdd0fa5bfc9592002f6e7431208",
            "folder": "gcc-arm-none-eabi-10.3-2021.10"
        }
    elif sys_mac == "darwin_x86_64":

        name = "arm-gnu-toolchain-13.3.rel1-darwin-x86_64-arm-none-eabi.tar.xz"
        package_info = {
            "url": f"{host_mac}/{name}",
            "name": name,
            "size": 138514276,
            "sha256": "1ab00742d1ed0926e6f227df39d767f8\
efab46f5250505c29cb81f548222d794",
            "folder": "arm-gnu-toolchain-13.3.rel1-darwin-x86_64-arm-none-eabi"
        }
    elif sys_mac == "darwin_arm64":
        name = "arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi.tar.xz"
        package_info = {
            "url": f"{host_mac}/{name}",
            "name": name,
            "size": 128649856,
            "sha256": "fb6921db95d345dc7e5e487dd43b745e\
3a5b4d5c0c7ca4f707347148760317b4",
            "folder": "arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi"
        }
    elif sys_mac.startswith("windows"):
        name = "gcc-arm-none-eabi-10.3-2021.10-win32.zip"
        package_info = {
            "url": f"{host}/{name}",
            "name": name,
            "size": 200578763,
            "sha256": "d287439b3090843f3f4e29c7c41f81d9\
58a5323aecefcf705c203bfd8ae3f2e7",
            "folder": "gcc-arm-none-eabi-10.3-2021.10"
        }
    else:
        print("##############################")
        print(f"Warning: Toolchain not support [{sys_mac}]")
        print("Please download toolchain from ->")
        print("https://developer.arm.com/downloads/-/gnu-rm")
        print("##############################")
        package_info = {}

    return package_info


LAST_PROGRESS = 0


def _show_progress(block_num, block_size, total_size):
    global LAST_PROGRESS
    downloaded = block_num * block_size
    progress = 0
    if total_size > 0:
        progress = min(100, (downloaded / total_size) * 100)

    progress = int(progress)
    if LAST_PROGRESS != progress:
        print(f"\rprogress: {progress}%", end="")
        LAST_PROGRESS = progress if progress < 100 else 0
    pass


def _download_from_url(url, download_file):
    try:
        context = ssl._create_unverified_context()
        with urllib.request.urlopen(url, context=context) as response:
            total_size = int(response.headers.get('Content-Length', 0))
            block_size = 8192  # 8KB
            bytes_downloaded = 0

            with open(download_file, 'wb') as f:
                while True:
                    buffer = response.read(block_size)
                    if not buffer:
                        break

                    f.write(buffer)
                    bytes_downloaded += len(buffer)
                    block_num = bytes_downloaded // block_size

                    _show_progress(block_num, block_size, total_size)

    except Exception as e:
        print(f"Error: download failed: {str(e)}")
        rm_rf(download_file)
        return False

    return True


def wget_toolchain_package(toolchain_root,
                           package_info) -> bool:
    url = package_info["url"]
    name = package_info["name"]
    download_file = os.path.join(toolchain_root, name)

    if os.path.exists(download_file):
        print(f"[Toolchain package is exiets]: {download_file}")
        return True

    print(f"[Downloading package]: {url}")
    if not _download_from_url(url, download_file):
        return False

    return True


def check_toolchain_package(toolchain_root,
                            package_info) -> bool:
    '''
    Check size and sha256
    '''
    name = package_info["name"]
    package = os.path.join(toolchain_root, name)

    real_size = os.stat(package).st_size
    exp_size = package_info["size"]
    if real_size != exp_size:
        print(f"Error size: real[{real_size}], expectation[{exp_size}]")
        return False

    real_sha256 = calc_sha256(package)
    exp_sha256 = package_info["sha256"]
    if real_sha256 != exp_sha256:
        print(f"Error sha256: real[{real_sha256}], expectation[{exp_sha256}]")
        return False

    print("Toolchain package is OK.")
    return True


def unzip_toolchain_package(toolchain_root, package_info) -> bool:
    name = package_info["name"]
    package = os.path.join(toolchain_root, name)
    print(f"[Extracting toolchain package]: {package}")
    return extract_archive(package, toolchain_root)


def download_toolchain(toolchain_root) -> bool:
    package_info = get_toolchain_package_info()
    # print(f"package_info: {package_info}")
    if not package_info:
        return False

    folder = package_info["folder"]
    folder_path = os.path.join(toolchain_root, folder)
    if os.path.exists(folder_path):
        print(f"[Toolchain folder is exists]: {folder_path}")
        return True

    if not wget_toolchain_package(toolchain_root, package_info):
        return False

    if not check_toolchain_package(toolchain_root, package_info):
        return False

    if not unzip_toolchain_package(toolchain_root, package_info):
        return False

    return True
