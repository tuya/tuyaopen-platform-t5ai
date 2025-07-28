import json
from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

import bk_packager

curr_dir = Path(__file__).parent


class test_packager_linear(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...

    def setUp(self) -> None:
        print("")

    def tearDown(self) -> None: ...

    def test_packager_wd_not_exist(self):
        workdir = curr_dir / "non-exist-workspace"
        pack_json = workdir / "configuartion.json"
        try:
            bk_packager.bk_packager_linear(workdir, pack_json)
        except RuntimeError as e:
            self.assertEqual(f"work directory {workdir} not exist.", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_packager_json_not_exist(self):
        workdir = curr_dir / "workspace/common_wd"
        no_exist_json = workdir / "non-exist-configuartion.json"
        try:
            bk_packager.bk_packager_linear(workdir, no_exist_json)
        except RuntimeError as e:
            self.assertEqual(f"config json {no_exist_json} not exist.", str(e))
        else:
            self.assertTrue(False, "not catch error")

    def test_packager_json_invalid_format(self):
        workdir = curr_dir / "workspace/test_packager_invalid_json"
        invalid_json = workdir / "configuartion.json"
        try:
            bk_packager.bk_packager_linear(workdir, invalid_json)
        except json.JSONDecodeError:
            pass

    def test_packager_json_invalid_key(self):
        workdir = curr_dir / "workspace/test_packager_json_invalid_key"
        overlap_json = workdir / "configuartion.json"
        bootloader_path = workdir / "bootloader.bin"
        app_path = workdir / "app.bin"
        bootloader_path.touch()
        app_path.touch()
        try:
            bk_packager.bk_packager_linear(workdir, overlap_json)
        except RuntimeError as e:
            self.assertEqual("partition exist overlaps!", str(e))
        else:
            self.assertTrue(False, "not catch error")
        bootloader_path.unlink()
        app_path.unlink()

    def test_packager_bin_over_size(self):
        workdir = curr_dir / "workspace/test_packager_oversize"
        over_size_json = workdir / "configuartion.json"
        oversize_bin_path = workdir / "oversize.bin"
        oversize_bin_path.write_bytes(bytes([0xFF] * 256))
        try:
            bk_packager.bk_packager_linear(workdir, over_size_json)
        except RuntimeError as e:
            self.assertIn("size is over partitions size.", str(e))
        else:
            self.assertTrue(False, "not catch error")
        oversize_bin_path.unlink()

    def test_packager_pack(self):
        def prepare():
            bootloader_bytes = bytes([0xFF] * 10 + [0x00] * 10)
            cm_app_bytes = bytes([0xFF] * 25 + [0x00] * 25)
            ca_app_bytes = bytes([0xFF] * 15 + [0x00] * 35)
            bootloader_path.write_bytes(bootloader_bytes)
            cm_app_path.write_bytes(cm_app_bytes)
            ca_app_path.write_bytes(ca_app_bytes)

        def clear():
            bootloader_path.unlink()
            cm_app_path.unlink()
            ca_app_path.unlink()
            all_app_path.unlink()

        workdir = curr_dir / "workspace/test_packager_linear"
        pack_json = workdir / "configuartion.json"
        bootloader_path = workdir / "bootloader.bin"
        cm_app_path = workdir / "cm-app.bin"
        ca_app_path = workdir / "ca-app.bin"
        all_app_path = workdir / "all-app.bin"
        prepare()
        packager = bk_packager.bk_packager_linear(workdir, pack_json)
        packager.pack()
        gen_file_md5 = get_file_md5sum(all_app_path)
        expect_file_md5 = "ccdd7d4b5278ba547e9533c13e6a4bdd"
        self.assertEqual(expect_file_md5, gen_file_md5)
        clear()

    def test_packager_linear_non_zero_start(self):
        def prepare():
            cm_app_bytes = bytes([0xFF] * 25 + [0x00] * 25)
            ca_app_bytes = bytes([0xFF] * 15 + [0x00] * 35)
            cm_app_path.write_bytes(cm_app_bytes)
            ca_app_path.write_bytes(ca_app_bytes)

        def clear():
            cm_app_path.unlink()
            ca_app_path.unlink()
            all_app_path.unlink()

        workdir = curr_dir / "workspace/test_packager_linear_non_zero_start"
        pack_json = workdir / "configuartion.json"
        cm_app_path = workdir / "cm-app.bin"
        ca_app_path = workdir / "ca-app.bin"
        all_app_path = workdir / "all-app.bin"
        prepare()
        packager = bk_packager.bk_packager_linear(workdir, pack_json)
        packager.pack()
        gen_file_md5 = get_file_md5sum(all_app_path)
        expect_file_md5 = "f2e0e2b91bf43f574a738b8d93f5432c"
        self.assertEqual(expect_file_md5, gen_file_md5)
        clear()

    def test_packager_linear_zero_start(self):
        # need fill 0xff from address 0
        def prepare():
            cm_app_bytes = bytes([0xFF] * 25 + [0x00] * 25)
            ca_app_bytes = bytes([0xFF] * 15 + [0x00] * 35)
            cm_app_path.write_bytes(cm_app_bytes)
            ca_app_path.write_bytes(ca_app_bytes)

        def clear():
            cm_app_path.unlink()
            ca_app_path.unlink()
            all_app_path.unlink()

        workdir = curr_dir / "workspace/test_packager_linear_non_zero_start"
        pack_json = workdir / "configuartion.json"
        cm_app_path = workdir / "cm-app.bin"
        ca_app_path = workdir / "ca-app.bin"
        all_app_path = workdir / "all-app.bin"
        prepare()
        packager = bk_packager.bk_packager_linear(workdir, pack_json)
        packager.set_start_padding_mode(True)
        packager.pack()
        gen_file_md5 = get_file_md5sum(all_app_path)
        expect_file_md5 = "23913ec21e5429e8e34fe97728407849"
        self.assertEqual(expect_file_md5, gen_file_md5)
        clear()
