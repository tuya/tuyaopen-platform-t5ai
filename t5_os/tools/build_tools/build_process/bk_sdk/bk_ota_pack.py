from __future__ import annotations

import binascii
import json
import logging
import os
import shutil
from pathlib import Path

import bk_packager
from bk_crc import bk_crc16
from bk_misc import parse_format_size
from bk_ota_partition import bk_ota_partition

from .bk_curr_project import curr_project

logger = logging.getLogger(Path(__file__).name)
project_dir = curr_project.project_path
armino_path = curr_project.app0_src_root_path
ota_tool = curr_project.tools_path / "env_tools/rtt_ota/ota-rbl/ota_packager_python.py"
header_path = curr_project.tools_path / "env_tools/rtt_ota/ota-rbl"

OTA_PACK_BIN_ALIGN_LEN = 64


# region gen json
def gen_ota_pack_json():
    partitions_dir = curr_project.project_build_parititons_dir
    partitions_json = partitions_dir / "partitions.json"
    ota_partition_json = partitions_dir / "bk_ota_partitions.json"
    ota_partition = bk_ota_partition(partitions_json)
    if curr_project.is_ab_project:
        ota_partition.gen_ab_ota_json(ota_partition_json)
        ota_partition.gen_ab_configuartion_json(partitions_dir / "configurationab.json")
    else:
        ota_partition.gen_ota_json(ota_partition_json)


# end region
# region bl attach table
def format_string_to_bytes(string: str, length: int) -> bytes:
    string_bytes = string.encode()
    if len(string_bytes) < length:
        string_bytes += bytes(length - len(string_bytes))
    return string_bytes


def serialize_partitions_table(partitions_json: Path) -> bytes:
    def int_to_bytes(value: int):
        return value.to_bytes(4, "little")

    serialize_bytes = bytes()
    if not partitions_json.exists():
        raise RuntimeError(f"{partitions_json} not exists.")
    with partitions_json.open("r") as f:
        part_info = json.load(f)
    for part in part_info["part_table"]:
        part_bytes = bytes()
        magic_number = int(part["magic"], 16)
        part_bytes += int_to_bytes(magic_number)
        name = part["name"]
        part_bytes += format_string_to_bytes(name, 24)
        flash_name = part["flash_name"]
        part_bytes += format_string_to_bytes(flash_name, 24)
        offset = int(part["offset"], 16)
        part_bytes += int_to_bytes(offset)
        length = parse_format_size(part["len"])
        part_bytes += int_to_bytes(length)
        crc32_result = binascii.crc32(part_bytes) & 0xFFFFFFFF
        part_bytes += int_to_bytes(crc32_result)
        serialize_bytes += part_bytes
    return serialize_bytes


def handle_bootloader_bin(pack_dir: Path):
    if not pack_dir.exists():
        pack_dir.mkdir()
    # copy bootloader cp ap binary
    bootloader_name = "bootloader.bin"
    origin_bootloader_path = curr_project.bootloader_archive_path
    pack_bootloader_path = pack_dir / bootloader_name
    ota_json = curr_project.project_build_parititons_dir / "bk_ota_partitions.json"
    part_bytes = serialize_partitions_table(ota_json)
    shutil.copy(origin_bootloader_path, pack_bootloader_path)
    with pack_bootloader_path.open("ab") as f:
        pos = f.tell()
        if pos % 32 != 0:
            f.write(bytes(32 - pos % 32))
        f.write(part_bytes)

    logger.info("attach ota partitions to bootloader")


# end region
# region pack ota
def pack_ota_rbl_non_ab(origin_ota_app_bin: Path):
    ota_bin = Path("app_pack.rbl")
    cmd = (
        f"python3 {ota_tool} -i {origin_ota_app_bin} -o {ota_bin} "
        + f"-g {header_path} -ap {armino_path} -pjd {project_dir} packager"
    )
    ret = os.system(cmd)
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")
    logger.info(f"generate ota firmware {ota_bin}")
    return ota_bin


def pack_ota_rbl_ab(
    pack_dir: Path, bootloader_size: int, origin_ota_app_bin: Path, all_app_bin: Path
):
    ota_bin = Path("app_ab_crc.rbl")
    ota_app_temp_bin = pack_dir / "ota_app_temp.bin"
    soc_name = curr_project.soc_name
    cmd = (
        f"python3 {ota_tool} -i {origin_ota_app_bin} -o {ota_app_temp_bin} "
        + f"-g {header_path} -ap {armino_path} -soc {soc_name} -pjd {project_dir} packager"
    )
    # raise RuntimeError(cmd)
    ret = os.system(cmd)
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")

    crc_handler = bk_crc16()
    crc_handler.crc_file(ota_app_temp_bin, ota_bin)
    logger.info(f"generate ota firmware {ota_bin}")
    ota_app_temp_bin.unlink()

    with all_app_bin.open("rb+") as dest_f, ota_bin.open("rb") as src_f:
        dest_f.seek(bootloader_size)
        write_data = src_f.read()
        dest_f.write(write_data)
    logger.info(f"overwrite all_app.bin with {ota_bin}")
    return ota_bin


def pack_ota_rbl(
    pack_dir: Path, pack_json: Path, origin_ota_app_bin: Path, all_app_bin: Path
) -> Path:
    if not curr_project.is_ab_project:
        return pack_ota_rbl_non_ab(origin_ota_app_bin)

    with open(pack_json, "r") as f:
        pack_info = json.load(f)
    bootloader_size_fmt = "0"
    for part in pack_info["section"]:
        if part["partition"] == "bootloader":
            bootloader_size_fmt = part["size"]
            break
    bootloader_size = parse_format_size(bootloader_size_fmt)
    if bootloader_size == 0:
        raise RuntimeError("bootloader parse error")
    return pack_ota_rbl_ab(pack_dir, bootloader_size, origin_ota_app_bin, all_app_bin)


def pack_ota_app_bin(pack_dir: Path, output_bin: Path):
    pack_json = curr_project.project_build_parititons_dir / "bk_package.json"
    with pack_json.open("r") as f:
        apps_part_info = json.load(f)
    sections: list[dict[str, str]] = apps_part_info["section"]
    for index, part in enumerate(sections):
        if "bootloader" in part["partition"]:
            sections.pop(index)
            apps_part_info["count"] -= 1
            break

    if curr_project.flash_crc_enable:
        for part in sections:
            addr = int(int(part["start_addr"], 16) / 34 * 32)
            part["start_addr"] = f"0x{addr:08x}"
            size = parse_format_size(part["size"]) / 34 * 32
            size_format = int(size / 1024)
            part["size"] = f"{size_format}K"

    app_pack_json = pack_dir / "ota_apps_pack.json"
    with app_pack_json.open("w") as f:
        json.dump(apps_part_info, f, indent=4)

    ota_app_bin = output_bin
    packager = bk_packager.bk_packager_linear(pack_dir, app_pack_json, ota_app_bin)
    packager.pack()
    ota_app_size = ota_app_bin.stat().st_size
    padding_len = (
        OTA_PACK_BIN_ALIGN_LEN - (ota_app_size % OTA_PACK_BIN_ALIGN_LEN)
    ) % OTA_PACK_BIN_ALIGN_LEN
    logger.info(f"ota app size {ota_app_size}, padding len {padding_len}")
    if padding_len > 0:
        with ota_app_bin.open("ab") as f:
            f.write(bytes([0xFF]) * padding_len)


def ota_pack():
    build_pack_dir = curr_project.project_build_package_dir
    pack_dir_temp = build_pack_dir / "tmp"
    origin_ota_app_bin = pack_dir_temp / "app_pack.bin"
    pack_ota_app_bin(pack_dir_temp, origin_ota_app_bin)

    build_partitions_dir = curr_project.project_build_parititons_dir
    pack_json = build_partitions_dir / "bk_package.json"
    all_app_bin = build_pack_dir / "all-app.bin"
    ota_bin = pack_ota_rbl(pack_dir_temp, pack_json, origin_ota_app_bin, all_app_bin)
    return ota_bin.absolute()


# end region
