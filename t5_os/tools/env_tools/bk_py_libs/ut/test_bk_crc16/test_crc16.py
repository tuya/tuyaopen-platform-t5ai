from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

import bk_crc

curr_dir = Path(__file__).parent


class Test_bk_crc16(TestCase):
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

    def test_crc16(self):
        crc = bk_crc.bk_crc16()
        test_bin_path = curr_dir / "test.bin"
        test_bin_path.write_bytes(bytes([0x00] * 40))
        output_bin_path = curr_dir / "output.bin"
        crc.crc_file(test_bin_path, output_bin_path)
        gen_file_md5 = get_file_md5sum(output_bin_path)
        expect_file_md5 = "40b9f4a1d04f271cbb7d479ceea6e37d"
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_bin_path.unlink()
        test_bin_path.unlink()
