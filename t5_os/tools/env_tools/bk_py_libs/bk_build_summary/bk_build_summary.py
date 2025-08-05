from __future__ import annotations

import logging
from pathlib import Path

from bk_misc import parse_format_size

logger = logging.getLogger(__package__)

OUTPUT_LINE_MAX_LEN = 60


def find_string_in_file(file_path: Path, target: str):
    target_bytes = target.encode("utf-8")
    target_len = len(target_bytes)
    buffer_size = 4096

    try:
        with file_path.open("rb") as file:
            offset = 0
            prev_chunk = b""

            while True:
                chunk = file.read(buffer_size)
                if not chunk:
                    return -1

                search_data = prev_chunk + chunk
                pos = search_data.find(target_bytes)

                if pos != -1:
                    return offset + pos - len(prev_chunk)

                prev_chunk = chunk[-target_len + 1 :] if target_len > 1 else b""
                offset += len(chunk)

    except FileNotFoundError:
        logger.error(f"file '{file_path}' not exist.")
    except Exception as e:
        logger.error(f"error: {e}, error type: {type(e)}")
    return -1


class bk_build_summary:
    def __init__(self) -> None:
        self.partitions_info = None
        self.app_folder: list[tuple[str, Path]] = []
        self.out_info = None

    def set_partitions_info(self, part_info: Path) -> None:
        if not part_info.exists():
            msg = f"{part_info} not exists"
            raise RuntimeError(msg)

        self.partitions_info = part_info

    def set_app_folder(self, app_name: str, app_path: Path) -> None:
        if not app_name:
            raise RuntimeError("app name is not valid.")
        if not app_path.is_dir():
            raise RuntimeError("app path not exist.")
        self.app_folder.append((app_name, app_path))

    def set_output_file_info(self, out_info: str):
        self.out_info = out_info

    def gen_summary(self, output_file: Path) -> None:
        if output_file.exists():
            output_file.unlink()

        summary = ""
        summary += self._get_header()
        if self.partitions_info:
            summary += f"{' Partitions Table ':=^{OUTPUT_LINE_MAX_LEN}}\n"
            summary += self.partitions_info.read_text()

        if len(self.app_folder):
            summary += self._get_apps_memory()

        if self.out_info:
            summary += f"{' Output Info ':=^{OUTPUT_LINE_MAX_LEN}}\n"
            summary += self.out_info

        logger.info(f"save build summary to {output_file}")
        with output_file.open("w", newline="\n") as f:
            f.write(summary)

    def _get_header(self) -> str:
        head_title = "  BUILD  SUMMARY  "
        head = ""
        separete_line = f"{'':-^{OUTPUT_LINE_MAX_LEN}}\n"
        head += separete_line
        head += f"{head_title:-^{OUTPUT_LINE_MAX_LEN}}\n"
        head += separete_line
        return head

    def _get_apps_memory(self) -> str:
        memory_info = ""
        memory_info += f"{' APP Memory Info ':=^{OUTPUT_LINE_MAX_LEN}}\n"
        for app in self.app_folder:
            app_name = app[0]
            app_path = app[1]
            memory_info += self._get_app_mem_info(app_name, app_path)
        return memory_info

    @classmethod
    def _get_app_mem_info(cls, app_name: str, app_path: Path):
        mem_info = ""
        map_file = app_path / "app.map"
        mem_file = app_path / "app_memory.txt"
        mem_info += f">>>>>>>>>> {app_name}\n"
        prerequisite = True
        if not map_file.exists():
            logger.warning(f"{map_file} not exists.")
            prerequisite = False
        if not mem_file.exists():
            logger.warning(f"{mem_file} not exists.")
            prerequisite = False

        try:
            link_info = cls._get_link_info(mem_file)
            map_info = cls._get_map_info(map_file)
            mem_info += cls._combine_link_and_map_info(link_info, map_info)
        except RuntimeError as e:
            logger.error(str(e))
            prerequisite = False
        except IndexError:
            logger.error("Index Error")
            prerequisite = False
        if not prerequisite:
            mem_info += "no found memory info\n"
        mem_info += f"<<<<<<<<<< {app_name}\n"
        return mem_info

    @classmethod
    def _combine_link_and_map_info(
        cls,
        link_info: list[tuple[str, int, int, str]],
        map_info: list[tuple[str, int, int]],
    ):
        if len(link_info) != len(map_info):
            raise RuntimeError("map and link len not align")
        mem_region_info = ""

        mem_region_info += (
            f"{'name':<21}"
            + f"{' addr':^11}"
            + f"{' size':^11}"
            + f"{'used ':>10}"
            + f"{'usage':>7}"
            + "\n"
        )
        mem_region_info += f"{'':-^{OUTPUT_LINE_MAX_LEN}}\n"
        for link, map in zip(link_info, map_info):
            link_name, link_used_size, link_size, link_usage_rate = link
            map_name, map_addr, map_size = map
            if link_name != map_name or link_size != map_size:
                raise RuntimeError("data invalid")
            mem_region_info += f"{link_name:<21}"  # 21
            mem_region_info += f" 0x{map_addr:08x}"  # 11
            mem_region_info += f" 0x{map_size:08x}"  # 11
            mem_region_info += f" {link_used_size:>7} B"  # 10
            mem_region_info += f"{link_usage_rate:>7}\n"
        return mem_region_info

    @classmethod
    def _get_map_info(cls, map_file: Path):
        all_mem_info: list[tuple[str, int, int]] = []

        offset = find_string_in_file(map_file, "Memory Configuration")
        if offset == -1:
            logger.error(f"{map_file} size is {map_file.stat().st_size}")
            raise RuntimeError("not found 'Memory Configuration'")

        with map_file.open("r") as f:
            f.seek(offset)
            raw_mem_info: str = f.read(2048)

        start = raw_mem_info.find("FLASH")
        end = raw_mem_info.find("*default")
        if start == -1:
            raise RuntimeError("not found 'FLASH'")
        if end == -1:
            raise RuntimeError("not found '*default'")
        mem_regions = raw_mem_info[start:end].strip().split("\n")

        for mem in mem_regions:
            mem_info = mem.split()
            mem_name = mem_info[0]
            mem_addr = int(mem_info[1], 16)
            mem_size = int(mem_info[2], 16)
            all_mem_info.append((mem_name, mem_addr, mem_size))
        return all_mem_info

    @classmethod
    def _get_link_info(cls, mem_file: Path):
        mem_info: list[tuple[str, int, int, str]] = []
        raw_link_mem_info = mem_file.read_text().strip().replace(":", "")
        start = raw_link_mem_info.find("FLASH")

        link_mem_info = raw_link_mem_info[start:]
        lines = link_mem_info.split("\n")

        for line in lines:
            info = line.split()
            name = info[0]
            used_size = parse_format_size(info[1] + info[2])
            region_size = parse_format_size(info[3] + info[4])
            usage_rate = info[5]
            mem_info.append((name, used_size, region_size, usage_rate))
        return mem_info
