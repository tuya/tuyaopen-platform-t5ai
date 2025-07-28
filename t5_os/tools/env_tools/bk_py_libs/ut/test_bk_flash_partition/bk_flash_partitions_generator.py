from __future__ import annotations

import logging

from bk_flash_partiton import bk_flash_partition_content_generator, partition_info

logger = logging.getLogger(__name__)


class bk_flash_test_partition_content_generator(bk_flash_partition_content_generator):
    # region layout_hdr
    def get_flash_partitions_layout_hdr_content(
        self, part_info: list[partition_info], flash_crc_enable: bool
    ) -> str:
        self.part_info = part_info
        s_hdr = "#pragma once\n\n"
        s_hdr += f"#define {'CONFIG_FLASH_CRC_ENABLE':<45} {int(flash_crc_enable)}\n"
        s_hdr += f"#define {'CONFIG_PARTITIONS_NUM':<45} {len(part_info) if part_info else 0}\n"
        partitions_id_define = ""
        for part in part_info:
            partition_name = part.Name.upper().replace(" ", "_") if part.Name else ""
            macro_name = f"BK_PARTITION_{partition_name}"
            partitions_id_define += f"#define {macro_name:<45} {part.Id}\n"
            macro_offset = f"CONFIG_{partition_name}_PARTITION_OFFSET"
            macro_size = f"CONFIG_{partition_name}_PARTITION_SIZE"

            s_hdr += f"#define {macro_offset:<45} 0x{part.Offset:08x}\n"
            s_hdr += f"#define {macro_size:<45} 0x{part.Size:08x}\n"

        return s_hdr + "\n" + partitions_id_define

    # endregion get_flash_partitions_layout_hdr_content
