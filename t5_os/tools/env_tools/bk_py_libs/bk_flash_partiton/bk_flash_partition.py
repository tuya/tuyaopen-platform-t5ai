from __future__ import annotations

import copy
import json
import logging
from abc import ABC, abstractmethod
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
class partition_section:
    firmware: str
    partition: str
    start_addr: str
    size: str


class bk_flash_partition_content_generator(ABC):
    @abstractmethod
    def get_flash_partitions_layout_hdr_content(
        self, part_info: list[partition_info], flash_crc_enable: bool
    ) -> str: ...


class bk_flash_partition:
    def __init__(
        self, part_json: Path, flash_generator: bk_flash_partition_content_generator
    ):
        logger.info(f"read parititons from {part_json}")
        with part_json.open("r") as f:
            json_content = json.load(f)
        part_info_json: list[dict[str, Any]] = json_content["section"]
        self.crc_enable = json_content["crc_enable"]
        self.raw_part_info = copy.deepcopy(part_info_json)
        self.header_path = Path("flash_partition.h")
        self.part_info: list[partition_info] = []
        self._parse_partitions_info(part_info_json)
        self.header_arch = None
        self.generator = flash_generator

    def _parse_partitions_info(self, part_info_json: list[dict[str, Any]]):
        self.part_info = [partition_info(**part) for part in part_info_json]
        self._part_adapter()
        self.part_info.sort(key=lambda x: x.Id)

    def _part_adapter(self):
        app_count = 0
        for part in self.part_info:
            if "bootloader" in part.Name and part.Execute:
                part.Name = "bootloader"
            elif part.Execute:
                part.Name = "application" + (str(app_count) if app_count else "")
                app_count += 1

    def gen_partitions_layout_hdr(self, partition_hdr_file: Path):
        logger.debug(f"Create partition hdr file: {partition_hdr_file}")
        hdr_contents = self.generator.get_flash_partitions_layout_hdr_content(
            self.part_info, self.crc_enable
        )
        with partition_hdr_file.open("w", newline="\n") as f:
            f.write(hdr_contents)

        logger.info(f"gen partition layout header to {partition_hdr_file}")

    def gen_pack_json(self, pack_json: Path):
        def get_pack_name(app_name: str) -> str:
            if "application" in app_name:
                return app_name.replace("application", "app")
            return app_name

        config_dict: dict[str, Any] = {
            "magic": "beken",
            "crc_enable": self.crc_enable,
            "count": 0,
            "section": [],
        }
        execute_partitions = [p for p in self.part_info if p.Execute]
        config_dict["count"] = len(execute_partitions)

        for p in sorted(execute_partitions, key=lambda x: x.Offset):
            part_name = get_pack_name(p.Name)
            sect = partition_section(
                f"{part_name}.bin", part_name, f"0x{p.Offset:08x}", format_size(p.Size)
            )
            sec_dict = asdict(sect)
            config_dict["section"].append(sec_dict)

        logger.info(f"gen package json: {pack_json}")
        with pack_json.open("w", newline="\n") as f:
            json.dump(config_dict, f, sort_keys=False, indent=4)
