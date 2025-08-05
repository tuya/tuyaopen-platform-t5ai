from __future__ import annotations

import logging

from bk_flash_partiton import bk_flash_partition_content_generator, partition_info

logger = logging.getLogger(__name__)


def get_license() -> str:
    s_license = """\
// Copyright 2022-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//This is a generated file, don't modify it!

"""

    return s_license


class bk_flash_denpendecny_generator(bk_flash_partition_content_generator):
    # region layout_hdr
    def get_flash_partitions_layout_hdr_content(
        self, part_info: list[partition_info], flash_crc_enable: bool
    ) -> str:
        s_hdr = get_license()

        s_hdr += "#pragma once\n\n"
        s_hdr += f"#define {'CONFIG_FLASH_CRC_ENABLE':<45} {int(flash_crc_enable)}\n"
        s_hdr += f"#define {'CONFIG_PARTITIONS_NUM':<45} {len(part_info) if part_info else 0}\n"
        partition_struct_array = "#define BK_FLASH_PARTITIONS_MAP { \\\n"
        partitions_id_define = ""
        partitions_id_define += (
            f"#define {'BK_PARTITIONS_TABLE_SIZE':<45} {part_info[-1].Id + 1}\n"
        )
        for part in part_info:
            partition_name = part.Name.upper().replace(" ", "_") if part.Name else ""
            macro_name = f"BK_PARTITION_{partition_name}"
            partitions_id_define += f"#define {macro_name:<45} {part.Id}\n"
            macro_offset = f"CONFIG_{partition_name}_PARTITION_OFFSET"
            macro_size = f"CONFIG_{partition_name}_PARTITION_SIZE"

            s_hdr += f"#define {macro_offset:<45} 0x{part.Offset:08x}\n"
            s_hdr += f"#define {macro_size:<45} 0x{part.Size:08x}\n"
            exe_mode = "PAR_OPT_EXECUTE_" + ("EN" if part.Execute else "DIS")
            read_mode = "PAR_OPT_READ_" + ("EN" if part.Read else "DIS")
            write_mode = "PAR_OPT_WRITE_" + ("EN" if part.Write else "DIS")
            partitions_options = f"{exe_mode} | {read_mode} | {write_mode}"
            partition_struct_array += (
                f'    [{part.Id}] = {{BK_FLASH_EMBEDDED, "{part.Name}", {macro_offset}, '
                + f"{macro_size}, {partitions_options}}}, \\\n"
            )

        partition_struct_array += "}\n"
        return s_hdr + "\n" + partitions_id_define + "\n" + partition_struct_array

    # endregion get_flash_partitions_layout_hdr_content
