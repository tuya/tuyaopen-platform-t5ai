from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

import bk_packager

curr_dir = Path(__file__).parent


class test_packager_format_crc(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...

    def setUp(self) -> None:
        print("")

    def tearDown(self) -> None: ...

    def test_packager_format_crc_bk7259(self):
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

        workdir = curr_dir / "workspace/test_packager_format_crc"
        pack_json = workdir / "configuartion.json"
        bootloader_path = workdir / "bootloader.bin"
        cm_app_path = workdir / "cm-app.bin"
        ca_app_path = workdir / "ca-app.bin"
        all_app_path = workdir / "all-app.bin"
        prepare()
        packager = bk_packager.bk_packager_format_crc(workdir, pack_json)
        packager.pack()
        gen_file_md5 = get_file_md5sum(all_app_path)
        expect_file_md5 = "669a3e0df3b27112e46322d0f6e71442"
        self.assertEqual(expect_file_md5, gen_file_md5)
        clear()
