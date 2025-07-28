from __future__ import annotations

import logging
import os
import shutil
from pathlib import Path

from bk_build_summary import bk_build_summary
from bk_sdk.bk_curr_project import curr_project

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def backup_bootloader_path():
    # copy bootloader elf-map-asm to build directory
    bootloader_build_dir = curr_project.bootloader_build_path.parent
    if not bootloader_build_dir.exists():
        logger.debug("bootloader not build, not backup.")
        return

    bootloader_backup_path = curr_project.bootloader_backup_dir
    bootloader_backup_path.parent.mkdir(parents=True, exist_ok=True)
    if bootloader_backup_path.exists():
        shutil.rmtree(bootloader_backup_path)

    shutil.copytree(bootloader_build_dir, bootloader_backup_path)
    logger.info(f"backup bootloader to {bootloader_backup_path}")


def pack_all_bin(pack_dir: Path, pack_json: Path, output_bin: Path):
    def binary_align_32_byte(bin_path: Path):
        if not bin_path.exists():
            raise RuntimeError(f"{bin_path} no exist.")
        bin_size = output_bin.stat().st_size
        padding_size = (32 - bin_size % 32) % 32
        with bin_path.open("ab") as f:
            f.write(bytes([0xFF] * padding_size))

    if not pack_json.exists():
        raise FileNotFoundError(f"{pack_json} not found")
    pack_dir.mkdir(parents=True, exist_ok=True)
    packager = curr_project.get_packager(pack_dir, pack_json, output_bin)
    packager.pack()
    # cmake_Gen_img 32byte align, so do same here.
    binary_align_32_byte(output_bin)


def gen_build_summary(output_info: str):
    build_pack_dir = curr_project.project_build_package_dir
    build_dir = curr_project.project_build_dir
    sumary_file = build_pack_dir / "build_summary.txt"
    summary = bk_build_summary()
    partitions_info = build_dir / "partitions" / "partitions.txt"
    summary.set_partitions_info(partitions_info)

    for app in curr_project.apps_info:
        app_build_dir = build_dir / app.app_name
        summary.set_app_folder(app.app_name_in_sdk, app_build_dir)

    summary.set_output_file_info(output_info)
    summary.gen_summary(sumary_file)


def firmware_package():
    build_pack_dir = curr_project.project_build_package_dir
    all_app_bin = build_pack_dir / "all-app.bin"
    build_partitions_dir = curr_project.project_build_parititons_dir
    pack_dir_temp = build_pack_dir / "tmp"
    pack_json = build_partitions_dir / "bk_package.json"
    if not pack_dir_temp.exists():
        pack_dir_temp.mkdir()
    curr_project.copy_binaries_to_pack_dir(pack_dir_temp)
    os.chdir(build_pack_dir)
    pack_all_bin(pack_dir_temp, pack_json, all_app_bin)
    return all_app_bin


def main():
    logger.info("Enter Armino Package")
    backup_bootloader_path()
    curr_project.pre_package()
    all_app_bin = firmware_package()
    curr_project.build_summary = f"firmware: {all_app_bin}\n"
    curr_project.post_package()
    try:
        gen_build_summary(curr_project.build_summary)
    except Exception:
        logger.warning("skip generating build summary info")


if __name__ == "__main__":
    set_logging()
    main()
