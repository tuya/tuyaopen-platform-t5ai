from __future__ import annotations

from dataclasses import asdict, dataclass
from typing import Any

OUTPUT_LINE_MAX_LEN = 60


@dataclass
class bk_partition:
    Id: int
    Name: str
    Offset: int
    Size: int
    Execute: bool
    Read: bool
    Write: bool

    def __post_init__(self) -> None:
        if self.Size == 0 or self.Size % 1024 != 0:
            msg = f"Partitons {self.Name} size vale {self.Size} is not valid."
            raise ValueError(msg)

    def get_part_dict(self) -> dict[str, Any]:
        return asdict(self)

    def get_format_info(self) -> str:
        info = ""
        info += self.Name + ","
        info += f"0x{self.Offset:08x},"  # offfset
        size_kb = int(self.Size / 1024)
        info += f"{size_kb}K,"  # size
        info += str(self.Execute) + ","
        info += str(self.Read) + ","
        info += str(self.Write)
        return info

    @classmethod
    def get_pretty_format_info_head(cls):
        name = "Name"
        offset = "Offset"
        size = "Size"
        head = f"{name:<26}" + f"{offset:^22}" + f"{size:^12}" + "\n"
        head += f"{'':-^{OUTPUT_LINE_MAX_LEN}}\n"
        return head

    @classmethod
    def get_partitions_pretty_info(
        cls,
        part_name: str,
        part_offset: int,
        part_size: int,
    ):
        info = ""
        info += f"{part_name:<26}"  # 26
        offset = f"0x{part_offset:08x}"
        info += f"{offset:^22}"  # offfset # 22
        size_kb = f"{int(part_size / 1024)}K"
        info += f"{size_kb:^12}"  # size # 12
        info += "\n"
        return info

    def get_pretty_format_info(self) -> str:
        return self.get_partitions_pretty_info(self.Name, self.Offset, self.Size)

    @classmethod
    def get_unused_part_info(cls, offset: int, size: int):
        name = "(unused)"
        return bk_partition.get_partitions_pretty_info(name, offset, size)
