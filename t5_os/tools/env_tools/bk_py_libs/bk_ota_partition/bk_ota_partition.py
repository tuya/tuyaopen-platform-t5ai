from __future__ import annotations

import copy
import json
import logging
import re
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

from bk_misc import format_size

logger = logging.getLogger(__package__)


@dataclass
class partition_info:
    Id: int
    Name: str
    Offset: int
    Size: int
    Execute: bool
    Read: bool
    Write: bool


@dataclass
class ota_partition:
    magic: str
    name: str
    flash_name: str
    offset: str
    len: str


@dataclass
class pack_partition:
    firmware: str
    partition: str
    start_addr: str
    size: str


class bk_ota_partition:
    def __init__(self, part_json: Path) -> None:
        logger.info(f"read parititons from {part_json}")
        with part_json.open("r") as f:
            json_content = json.load(f)
        part_info: list[partition_info] = [
            partition_info(**item) for item in json_content["section"]
        ]
        self.crc_enable = json_content["crc_enable"]
        self.raw_part_info = copy.deepcopy(part_info)
        self._part_adapter(part_info)
        self.part_info = part_info
        self.header_path = "flash_partition.h"
        self.header_arch = None

    def _part_adapter(self, part_sections: list[partition_info]):
        app_count = 0
        for item in part_sections:
            if "bootloader" in item.Name and item.Execute:
                item.Name = "bootloader"
                continue
            if item.Execute:
                item.Name = "application" + (str(app_count) if app_count else "")
                app_count += 1
                continue

    def gen_ab_ota_json(self, ota_json: Path):
        def get_ota_part_name(part_name: str) -> str:
            if "application" in part_name:
                return "appa"
            return part_name

        part_table_dict: dict[str, list[Any]] = {
            "part_table": [],
        }
        app_total_size = 0x0
        appa_offset = 0x0
        part_table_bark = copy.deepcopy(self.part_info)
        part_table_temp: list[partition_info] = list()
        for p in part_table_bark:
            if re.match(r"^bootloader(\d)*$", p.Name):
                part_table_temp.append(p)
                appa_offset = p.Offset + p.Size
            if "application" in p.Name:
                if p.Name == "application":
                    app_total_size = p.Size
                    part_table_temp.append(p)
                else:
                    app_total_size += p.Size
            if re.match(r"^s_app(\d)*$", p.Name):
                part_table_temp.append(p)
            if re.match(r"^ota_fina_executive(\d)*$", p.Name):
                part_table_temp.append(p)
        for p in part_table_temp:
            part_name = get_ota_part_name(p.Name)
            p.Name = part_name

        for p in sorted(part_table_temp, key=lambda x: x.Offset):
            magic = "0x45503130"
            if p.Name == "s_app" or p.Execute:
                flash_name = "beken_onchip_crc"
            else:
                flash_name = "beken_onchip"
            if p.Name == "appa":
                part_len = format_size(app_total_size)
                part_offset = f"0x{appa_offset:08x}"
            else:
                part_len = format_size(p.Size)
                part_offset = f"0x{p.Offset:08x}"

            ota_part = ota_partition(magic, p.Name, flash_name, part_offset, part_len)
            part_table_dict["part_table"].append(asdict(ota_part))

        logger.info(f"gen package json: {ota_json}")
        with ota_json.open("w", newline="\n") as f:
            json.dump(part_table_dict, f, sort_keys=False, indent=4)

    def gen_ota_json(self, ota_json: Path):
        def get_ota_part_name(part_name: str) -> str:
            if "application" in part_name:
                return part_name.replace("application", "app")
            if part_name == "ota":
                return "download"
            return part_name

        part_table_dict: dict[str, list[Any]] = {
            "part_table": [],
        }
        part_table_bark = copy.deepcopy(self.part_info)
        part_table_temp: list[partition_info] = list()
        for p in part_table_bark:
            if re.match(r"^bootloader(\d)*$", p.Name):
                part_table_temp.append(p)
            if re.match(r"^application(\d)*$", p.Name):
                part_table_temp.append(p)
            if re.match(r"^ota(\d)*$", p.Name):
                part_table_temp.append(p)

        for p in part_table_temp:
            part_name = get_ota_part_name(p.Name)
            p.Name = part_name

        for p in sorted(part_table_temp, key=lambda x: x.Offset):
            magic = "0x45503130"
            flash_name = "beken_onchip_crc" if p.Execute else "beken_onchip"
            ota_part = ota_partition(
                magic, p.Name, flash_name, f"0x{p.Offset:08x}", format_size(p.Size)
            )
            part_table_dict["part_table"].append(asdict(ota_part))
        logger.info(f"gen package json: {ota_json}")
        with ota_json.open("w", newline="\n") as f:
            json.dump(part_table_dict, f, sort_keys=False, indent=4)

    def gen_ab_configuartion_json(self, config_json: Path):
        json_content: dict[str, Any] = {}
        json_content.update({"magic": "beken"})
        sections: list[dict[str, str]] = []
        bootloader_start = 0
        bootloader_size = 0
        app_size = 0
        app_start = 0xFFFFFFFF
        for part in self.part_info:
            if part.Name == "bootloader":
                bootloader_start = part.Offset
                bootloader_size = part.Size
                continue
            if part.Execute:
                app_start = min(app_start, part.Offset)
                app_size += part.Size

        if self.crc_enable:
            bootloader_start = int(bootloader_start / 34 * 32)
            bootloader_size = int(bootloader_size / 34 * 32 / 1024)
            app_start = int(app_start / 34 * 32)
            app_size = int(app_size / 34 * 32 / 1024)

        bootloader_sect = pack_partition(
            "bootloader.bin",
            "bootloader",
            f"0x{bootloader_start:08x}",
            f"{bootloader_size}K",
        )
        app_sect = pack_partition(
            "app_ab.bin", "app", f"0x{app_start:08x}", f"{app_size}K"
        )
        sections.append(asdict(bootloader_sect))
        sections.append(asdict(app_sect))
        json_content.update({"count": len(sections)})
        json_content.update({"section": sections})
        logger.info(f"gen ab configuartion json: {config_json}")
        with config_json.open("w", newline="\n") as f:
            json.dump(json_content, f, indent=4)
