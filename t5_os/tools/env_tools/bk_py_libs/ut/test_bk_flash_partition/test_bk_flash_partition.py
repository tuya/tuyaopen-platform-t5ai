from pathlib import Path
from unittest import TestCase

from bk_flash_partitions_generator import bk_flash_test_partition_content_generator
from ut_msic import get_file_md5sum

from bk_flash_partiton import bk_flash_partition

curr_dir = Path(__file__).parent


class test_bk_flash_partition(TestCase):
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

    def test_gen_partitions_header(self):
        workdir = curr_dir / "workspace"
        partition_json = workdir / "gen_partitions.json"
        generator = bk_flash_test_partition_content_generator()
        flash_part = bk_flash_partition(partition_json, generator)
        header_path = workdir / "partitions.h"
        flash_part.gen_partitions_layout_hdr(header_path)
        self.assertTrue(header_path.exists())
        header_hash = get_file_md5sum(header_path)
        expect_header_hash = "30983ecbe7e2e255bec57fd0a656ce82"
        self.assertEqual(expect_header_hash, header_hash)
        header_path.unlink()

    def test_gen_pack_json(self):
        workdir = curr_dir / "workspace"
        partition_json = workdir / "gen_partitions.json"
        package_json = workdir / "bk_package.json"
        generator = bk_flash_test_partition_content_generator()
        flash_part = bk_flash_partition(partition_json, generator)
        flash_part.gen_pack_json(package_json)
        self.assertTrue(package_json.exists())
        json_hash = get_file_md5sum(package_json)
        expect_header_hash = "8ba79511c8ab26d08b7bc185173fcf8c"
        self.assertEqual(expect_header_hash, json_hash)
        package_json.unlink()
