from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

from bk_ram_region import bk_ram_region, mem_region

curr_dir = Path(__file__).parent


class test_bk_ram_region(TestCase):
    @classmethod
    def setUpClass(cls):
        pass

    @classmethod
    def tearDownClass(cls):
        pass

    def setUp(self) -> None:
        print("")
        pass

    def tearDown(self) -> None:
        pass

    def test_bk_ram_region_partitions(self):
        workdir = curr_dir / "workspace"
        mem_csv = workdir / "ram_region.csv"
        mem_header = workdir / "ram_region.h"
        ram_region = bk_ram_region(mem_csv)
        ram_region.set_sram_setting(0x28000000, 0xA0000)
        ram_region.set_psram_setting(0x60000000, 0x800000)
        ram_region.gen_memory_layout_hdr(mem_header)
        self.assertTrue(mem_header.exists())
        gen_file_md5 = get_file_md5sum(mem_header)
        expected_file_md5 = "c2224b2967709e5a61f6df407963ddbc"
        self.assertEqual(expected_file_md5, gen_file_md5)
        mem_header.unlink()

    def test_bk_ram_region_only_sram(self):
        workdir = curr_dir / "workspace"
        mem_csv = workdir / "ram_region_only_sram.csv"
        mem_header = workdir / "ram_region_only_sram.h"
        ram_region = bk_ram_region(mem_csv)
        ram_region.set_sram_setting(0x28000000, 0xA0000)
        ram_region.gen_memory_layout_hdr(mem_header)
        self.assertTrue(mem_header.exists())
        gen_file_md5 = get_file_md5sum(mem_header)
        expected_file_md5 = "5ceac0bf68b7834c03c267573bbbec8f"
        self.assertEqual(expected_file_md5, gen_file_md5)
        mem_header.unlink()

    def test_bk_ram_region_only_psram(self):
        workdir = curr_dir / "workspace"
        mem_csv = workdir / "ram_region_only_psram.csv"
        mem_header = workdir / "ram_region_only_psram.h"
        ram_region = bk_ram_region(mem_csv)
        ram_region.set_psram_setting(0x60000000, 0x800000)
        ram_region.gen_memory_layout_hdr(mem_header)
        self.assertTrue(mem_header.exists())
        gen_file_md5 = get_file_md5sum(mem_header)
        expected_file_md5 = "085911308dcc8cda98744ba7b682f8fa"
        self.assertEqual(expected_file_md5, gen_file_md5)
        mem_header.unlink()

    def test_bk_ram_region_set_default_setting(self):
        workdir = curr_dir / "workspace"
        mem_csv = workdir / "ram_region.csv"
        mem_header = workdir / "ram_regions.h"
        ram_region = bk_ram_region(mem_csv)
        ram_region.set_sram_setting(0x28000000, 0xA0000)
        ram_region.set_psram_setting(0x60000000, 0x800000)
        default_setting = [
            mem_region("AP_SPINLOCK", "SRAM", 0x28000000, 0x10000),
            mem_region("AP_RAM", "SRAM", 0x28010000, 0x54000),
            mem_region("CP_RAM", "SRAM", 0x28064000, 0x10000),
        ]
        ram_region.set_default_setting(default_setting)
        try:
            ram_region.gen_memory_layout_hdr(mem_header)
        except Exception as e:
            self.assertEqual(
                "CP_RAM size is not valid, default size: 0x00010000", str(e)
            )
        else:
            self.assertTrue(False, "not catch error")

    def test_bk_ram_region_set_default_setting_no_region(self):
        workdir = curr_dir / "workspace"
        mem_csv = workdir / "ram_region_only_psram.csv"
        mem_header = workdir / "ram_regions.h"
        ram_region = bk_ram_region(mem_csv)
        ram_region.set_sram_setting(0x28000000, 0xA0000)
        ram_region.set_psram_setting(0x60000000, 0x800000)
        default_setting = [mem_region("AP_SPINLOCK", "SRAM", 0x28000000, 0x10000)]
        ram_region.set_default_setting(default_setting)
        try:
            ram_region.gen_memory_layout_hdr(mem_header)
        except Exception as e:
            self.assertEqual("default region AP_SPINLOCK not found", str(e))
        else:
            self.assertTrue(False, "not catch error")
            mem_header.unlink()
