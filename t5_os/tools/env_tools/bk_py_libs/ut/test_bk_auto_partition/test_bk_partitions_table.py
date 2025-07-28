import json
from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

from bk_auto_partition import bk_partitions_table, partition_limit

curr_dir = Path(__file__).parent


class test_partitions_table(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...

    def setUp(self) -> None:
        print("")

    def tearDown(self) -> None: ...

    def test_partitions_table_gen_csv_with_crc(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions.csv"
        gen_csv_path = work_space / "gen_partitions.csv"
        table = bk_partitions_table(csv_path, crc_enable=True)
        table.gen_partition_csv(gen_csv_path)
        self.assertTrue(gen_csv_path.exists(), f"{gen_csv_path} not generate")
        expect_csv_hash = "ff4a79b1d610faa5f2b923b16ad69a28"
        gen_csv_hash = get_file_md5sum(gen_csv_path)
        self.assertEqual(expect_csv_hash, gen_csv_hash)
        gen_csv_path.unlink()

    def test_partitions_table_crc_check(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_invalid_crc.csv"
        try:
            bk_partitions_table(csv_path, crc_enable=True)
        except RuntimeError as e:
            self.assertIn("partition align error", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_crc_check_overlap(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_overlap.csv"
        try:
            bk_partitions_table(csv_path, crc_enable=True)
        except RuntimeError as e:
            self.assertIn("partition table config overlaps", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_gen_json_with_crc(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions.csv"
        gen_json_path = work_space / "gen_partitions.json"
        table = bk_partitions_table(csv_path, crc_enable=True)
        table.gen_partition_json(gen_json_path)
        self.assertTrue(gen_json_path.exists(), f"{gen_json_path} not generate")
        expect_json_hash = "f42ec7f80d3e2976ed2495a489ea264a"
        gen_json_hash = get_file_md5sum(gen_json_path)
        self.assertEqual(expect_json_hash, gen_json_hash)
        gen_json_path.unlink()

    def test_partitions_table_gen_partitions_show(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions.csv"
        gen_txt_path = work_space / "gen_partitions.txt"
        table = bk_partitions_table(csv_path, crc_enable=True)
        table.gen_pretty_format_table(gen_txt_path)
        self.assertTrue(gen_txt_path.exists(), f"{gen_txt_path} not generate")
        expect_txt_hash = "a5eebd99f7fa1f53a53ce19eaafe8e39"
        gen_txt_hash = get_file_md5sum(gen_txt_path)
        self.assertEqual(expect_txt_hash, gen_txt_hash)
        gen_txt_path.unlink()

    def test_partitions_table_unused_pretty_print(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_unused.csv"
        gen_txt_path = work_space / "gen_partitions_unused.txt"
        table = bk_partitions_table(csv_path, crc_enable=True)
        table.gen_pretty_format_table(gen_txt_path)
        self.assertTrue(gen_txt_path.exists(), f"{gen_txt_path} not generate")
        expect_txt_hash = "46e53795c0cd77db7664c87e81af3a8f"
        gen_txt_hash = get_file_md5sum(gen_txt_path)
        self.assertEqual(expect_txt_hash, gen_txt_hash)
        gen_txt_path.unlink()

    def test_partitions_table_miss_bootloader(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_no_bootloader.csv"
        default_setting_json = work_space / "flash_partitions_setting.json"
        table = bk_partitions_table(csv_path, crc_enable=True)
        setting_json = json.loads(default_setting_json.read_text())
        setting = [partition_limit(**item) for item in setting_json]
        try:
            table.set_default_setting(setting)
        except RuntimeError as e:
            self.assertEqual("primary_bootloader is not exists", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_index_error(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_index_error.csv"
        default_setting_json = work_space / "flash_partitions_setting.json"
        table = bk_partitions_table(csv_path, crc_enable=True)
        setting_json = json.loads(default_setting_json.read_text())
        setting = [partition_limit(**item) for item in setting_json]
        try:
            table.set_default_setting(setting)
        except RuntimeError as e:
            self.assertEqual("primary_bootloader index error", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_with_limit(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions.csv"
        gen_json_path = work_space / "gen_partitions.json"
        default_setting_json = work_space / "flash_partitions_setting.json"
        table = bk_partitions_table(csv_path, crc_enable=True)
        setting_json = json.loads(default_setting_json.read_text())
        setting = [partition_limit(**item) for item in setting_json]
        table.set_default_setting(setting)
        table.gen_partition_json(gen_json_path)
        self.assertTrue(gen_json_path.exists(), f"{gen_json_path} not generate")
        expect_json_hash = "f42ec7f80d3e2976ed2495a489ea264a"
        gen_json_hash = get_file_md5sum(gen_json_path)
        self.assertEqual(expect_json_hash, gen_json_hash)
        gen_json_path.unlink()

    def test_partitions_table_sort_paritions(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_with_user.csv"
        gen_json_path = work_space / "gen_partitions2.json"
        table = bk_partitions_table(csv_path, crc_enable=True)
        reserved_partitions = [
            "primary_bootloader",
            "primary_cp_app",
            "primary_ap_app",
            "sys_rf",
            "sys_net",
            "ota",
            "usr_config",
            "easyflash",
            "easyflash_ap",
        ]
        table.sort_partitions(reserved_partitions)

        table.gen_partition_json(gen_json_path)
        self.assertTrue(gen_json_path.exists(), f"{gen_json_path} not generate")
        expect_json_hash = "48309c01863dbfbe924eefc49c207dc6"
        gen_json_hash = get_file_md5sum(gen_json_path)
        self.assertEqual(expect_json_hash, gen_json_hash)
        gen_json_path.unlink()

    def test_partitions_table_flash_size_limit(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions.csv"
        try:
            bk_partitions_table(csv_path, "4M", crc_enable=True)
        except RuntimeError as e:
            self.assertIn("partition out of flash size", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_partitions_table_csv_flash_size_limit(self):
        work_space = curr_dir / "workspace/with_crc"
        csv_path = work_space / "auto_partitions_flash_size.csv"
        try:
            bk_partitions_table(csv_path, crc_enable=True)
        except RuntimeError as e:
            self.assertIn("partition out of flash size", str(e))
        else:
            self.assertTrue(False, "not catch error")
