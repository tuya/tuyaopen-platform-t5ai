import os
import json
from unittest import TestCase
import bk_packager
from test_misc import get_file_md5sum

curr_dir = os.path.dirname(__file__)

class test_packager_linear(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...
        
    def setUp(self) -> None:
        print("")

    def tearDown(self) -> None: ...

    def test_packager_wd_not_exist(self):
        no_exist_work_path = os.path.abspath(f"{curr_dir}/non-exist-workspace")
        try:
            bk_packager.bk_packager_linear(no_exist_work_path, f"{curr_dir}/non-exist-workspace/configuartion.json")
        except RuntimeError as e:
            self.assertEqual(f"work directory {no_exist_work_path} not exist.", str(e))
            pass

    def test_packager_json_not_exist(self):
        work_path = f"{curr_dir}/workspace/common_wd"
        no_exist_json = os.path.abspath(f"{curr_dir}/workspace/common_wd/non-exist-configuartion.json")
        try:
            bk_packager.bk_packager_linear(work_path, no_exist_json)
        except RuntimeError as e:
            self.assertEqual(f"config json {no_exist_json} not exist.", str(e))

    def test_packager_json_invalid_format(self):
        work_path = f"{curr_dir}/workspace/test_packager_invalid_json"
        invalid_json_path = f"{curr_dir}/workspace/test_packager_invalid_json/configuartion.json"
        try:
            bk_packager.bk_packager_linear(work_path, invalid_json_path)
        except json.JSONDecodeError as e:
            pass

    def test_packager_json_invalid_key(self):
        exist_path = f"{curr_dir}/workspace/test_packager_json_invalid_key"
        overlap_json = f"{curr_dir}/workspace/test_packager_json_invalid_key/configuartion4.json"
        try:
            bk_packager.bk_packager_linear(exist_path, overlap_json)
        except RuntimeError as e:
            self.assertEqual(f"partition exist overlaps!", str(e))

    def test_packager_bin_over_size(self):
        exist_path = f"{curr_dir}/test_packager_oversize"
        over_size_json = f"{curr_dir}/workspace/test_packager_oversize/oversize_configuartion.json"
        try:
            bk_packager.bk_packager_linear(exist_path, over_size_json)
            self.assertTrue(0, "not catch error")
        except RuntimeError as e:
            self.assertIn(f"size is over partitions size.", str(e))

    def test_packager_pack(self):
        exist_path = f"{curr_dir}/workspace/test_packager_linear_bk7259_no_crc"
        pack_json = f"{curr_dir}/workspace/test_packager_linear_bk7259_no_crc/configuartion.json"
        packager = bk_packager.bk_packager_linear(exist_path, pack_json)
        packager.pack()
        gen_file_md5 = get_file_md5sum(f"{curr_dir}/workspace/test_packager_linear_bk7259_no_crc/all-app.bin")
        expect_file_md5 = 'db9f348a11f903052993779ba4c69704'
        self.assertEqual(expect_file_md5, gen_file_md5)
        os.remove(f"{curr_dir}/workspace/test_packager_linear_bk7259_no_crc/all-app.bin")
