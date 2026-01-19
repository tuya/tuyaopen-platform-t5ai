from __future__ import annotations

import logging
import re
from dataclasses import dataclass
from pathlib import Path

from bk_misc import check_overlaps, parse_format_size

logger = logging.getLogger(__package__)


@dataclass
class mem_region:
    name: str
    type: str
    offset: int
    size: int


class bk_ram_region:
    def __init__(self, ram_mem_csv: Path) -> None:
        self.ram_mem_csv = ram_mem_csv
        self.sram_base = 0
        self.sram_capacity = 0
        self.psram_base = 0
        self.psram_capacity = 0
        self.total_offset = 0
        self.sram_regions_num = 0
        self.psram_regions_num = 0
        self.regions: list[mem_region] = []
        self.default_setting: list[mem_region] = []

    def set_default_setting(self, default_setting: list[mem_region]) -> None:
        self.default_setting = default_setting

    def set_sram_setting(self, sram_base: int, sram_capacity: int) -> None:
        self.sram_base = sram_base
        self.sram_capacity = sram_capacity

    def set_psram_setting(self, psram_base: int, psram_capacity: int) -> None:
        self.psram_base = psram_base
        self.psram_capacity = psram_capacity

    def _gen_regions(self) -> None:
        if not self.ram_mem_csv.exists():
            raise RuntimeError(f"{self.ram_mem_csv} not exists")
        self._parse_ram_mem_csv()

    def _check_default_setting(self) -> None:
        def find_region(def_region_name: str) -> int:
            for id, region in enumerate(self.regions):
                if region.name == def_region_name:
                    return id
            return -1

        for def_item in self.default_setting:
            region_id = find_region(def_item.name)
            if region_id == -1:
                msg = f"default region {def_item.name} not found"
                raise RuntimeError(msg)
            region = self.regions[region_id]
            if def_item.type != region.type:
                msg = f"{region.name} type is not valid, default type: {def_item.type}"
                raise RuntimeError(msg)
            if def_item.offset != region.offset:
                msg = f"{region.name} addr is not valid, default addr: 0x{def_item.offset:08x}"
                raise RuntimeError(msg)
            if def_item.size != region.size:
                msg = f"{region.name} size is not valid, default size: 0x{def_item.size:08x}"
                raise RuntimeError(msg)

    def _check_region_valid(self) -> None:
        for region in self.regions:
            if region.type == "SRAM":
                base = self.sram_base
                capacity = self.sram_capacity
                self.sram_regions_num += 1
            elif region.type == "PSRAM":
                base = self.psram_base
                capacity = self.psram_capacity
                self.psram_regions_num += 1
            else:
                raise RuntimeError(f"{region.type} is not supported")
            if region.offset < base:
                raise RuntimeError(
                    f"{region.name} addr is not valid, base  addr: 0x{base:08x}"
                )
            limit_addr = base + capacity
            if region.offset + region.size > base + capacity:
                msg = (
                    f"{region.name} is out of range, end addr: 0x{limit_addr:08x},"
                    + f"offset: 0x{region.offset:08x}, size: 0x{region.size:06x}, {capacity=}"
                )
                raise RuntimeError(msg)

    def _check_region_overlaps(self):
        space_sections: list[tuple[int, int]] = []
        for region in self.regions:
            space_sections.append((region.offset, region.size))
        if check_overlaps(space_sections):
            raise RuntimeError("RAM regions config overlaps")

    def _parse_ram_mem_csv(self):
        csv_contents = self.ram_mem_csv.read_text()
        lines = csv_contents.splitlines()

        for line in lines:
            line_content = line.strip()
            if line_content.startswith("#") or len(line_content) == 0:
                continue
            if "PSRAM_CAPCAITY_SIZE=" in line_content:
                self.psram_capacity = parse_format_size(line_content.split("=")[1])
                continue
            self._check_line_valid(line_content)
            self.regions.append(self._parse_line_mem_region(line_content))

    def _parse_line_mem_region(self, line_content: str) -> mem_region:
        region_content = line_content.split(",")
        offset_str = region_content[2].strip()
        if len(offset_str) == 0:
            offset = self.total_offset
        else:
            offset = int(offset_str, 16)
        size = int(region_content[3].strip(), 16)
        self.total_offset = offset + size
        return mem_region(
            name=region_content[0].strip(),
            type=region_content[1].strip(),
            offset=offset,
            size=size,
        )

    @staticmethod
    def _check_line_valid(line_content: str) -> None:
        ret = re.match(r"(?<!\\)\$([A-Za-z_][A-Za-z0-9_]*)", line_content)
        if ret:
            msg = f"auto partition table format error, line:\n{line_content}"
            raise RuntimeError(msg)

    def gen_memory_layout_hdr(self, hdr_file: Path) -> None:
        self._gen_regions()
        self._check_default_setting()
        self._check_region_valid()
        self._check_region_overlaps()
        with hdr_file.open("w", newline="\n") as f:
            f.write(self._get_region_hdr_text())
        logger.info(f"generate ram region header file: {hdr_file}")

    def _get_region_hdr_text(self) -> str:
        hdr_text = ""
        hdr_text += "#pragma once\n"
        if self.sram_regions_num:
            hdr_text += f"#define {'CONFIG_SRAM_BASE':<36} 0x{self.sram_base:08X}\n"
            hdr_text += (
                f"#define {'CONFIG_SRAM_CAPACITY':<36} 0x{self.sram_capacity:08X}\n"
            )
        if self.psram_regions_num:
            hdr_text += f"#define {'CONFIG_PSRAM_BASE':<36} 0x{self.psram_base:08X}\n"
            hdr_text += (
                f"#define {'CONFIG_PSRAM_CAPACITY':<36} 0x{self.psram_capacity:08X}\n"
            )
        for region in self.regions:
            name_addr = f"CONFIG_{region.name.upper()}_ADDR"
            name_size = f"CONFIG_{region.name.upper()}_SIZE"
            hdr_text += f"#define {name_addr:<36} 0x{region.offset:08X}\n"
            hdr_text += f"#define {name_size:<36} 0x{region.size:08X}\n"
        return hdr_text
