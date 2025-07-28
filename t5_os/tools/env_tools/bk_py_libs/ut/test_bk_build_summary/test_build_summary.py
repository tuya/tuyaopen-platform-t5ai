from pathlib import Path
from unittest import TestCase

from ut_msic import get_file_md5sum

from bk_build_summary import bk_build_summary

curr_dir = Path(__file__).parent


class Test_bk_build_summary(TestCase):
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

    def test_build_summary_parititons(self):
        workdir = curr_dir / "workspace"
        partitions_info = workdir / "partitions.txt"
        output_file = workdir / "summary1.txt"
        summary = bk_build_summary()
        summary.set_partitions_info(partitions_info)
        summary.gen_summary(output_file)
        self.assertTrue(output_file.exists())
        expect_file_md5 = "a1805d6e368bc27a7f7640a792578f6e"
        gen_file_md5 = get_file_md5sum(output_file)
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_file.unlink()

    def test_build_summary_app_memory(self):
        workdir = curr_dir / "workspace"
        output_file = workdir / "summary2.txt"
        summary = bk_build_summary()
        summary.set_app_folder("AP", workdir)
        summary.set_app_folder("CP", workdir)
        summary.gen_summary(output_file)
        self.assertTrue(output_file.exists())
        expect_file_md5 = "6fb10dcfac77cffaee59319a2fe35907"
        gen_file_md5 = get_file_md5sum(output_file)
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_file.unlink()

    def test_build_summary_output_file(self):
        workdir = curr_dir / "workspace"
        output_file = workdir / "summary3.txt"
        out_info = "firmware: all_app.bin\n" + "ota binary: ota.rbl\n"
        summary = bk_build_summary()
        summary.set_output_file_info(out_info)
        summary.gen_summary(output_file)
        self.assertTrue(output_file.exists())
        expect_file_md5 = "7f0848fb989fdfc580093b7e4cdae690"
        gen_file_md5 = get_file_md5sum(output_file)
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_file.unlink()

    def test_build_summary_all(self):
        workdir = curr_dir / "workspace"
        partitions_info = workdir / "partitions.txt"
        output_file = workdir / "summary4.txt"
        summary = bk_build_summary()
        summary.set_partitions_info(partitions_info)
        summary.set_app_folder("AP", workdir)
        summary.set_app_folder("CP", workdir)
        out_info = "firmware: all_app.bin\n" + "ota binary: ota.rbl\n"
        summary.set_output_file_info(out_info)
        summary.gen_summary(output_file)
        self.assertTrue(output_file.exists())
        expect_file_md5 = "fd857f18a5cb097a6095f85378c65fe5"
        gen_file_md5 = get_file_md5sum(output_file)
        self.assertEqual(expect_file_md5, gen_file_md5)
        output_file.unlink()
