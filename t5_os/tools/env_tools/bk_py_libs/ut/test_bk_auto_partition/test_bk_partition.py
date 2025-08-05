from unittest import TestCase

from bk_auto_partition.bk_partition import bk_partition


class test_partition(TestCase):
    @classmethod
    def setUpClass(cls): ...

    @classmethod
    def tearDownClass(cls): ...

    def setUp(self) -> None:
        print()

    def tearDown(self) -> None: ...

    def test_partition_defatult(self):
        partition = bk_partition(0, "bootloader", 0, 0x1000, False, True, True)
        info = partition.get_part_dict()
        expect_info: dict[str, str | int | bool] = {
            "Id": 0,
            "Name": "bootloader",
            "Offset": 0,
            "Size": 0x1000,
            "Execute": False,
            "Read": True,
            "Write": True,
        }
        self.assertEqual(expect_info, info)

    def test_partition_string(self):
        partition = bk_partition(0, "bootloader", 0, 0x1000, False, True, True)
        info = partition.get_format_info()
        expect_info = "bootloader,0x00000000,4K,False,True,True"
        self.assertEqual(expect_info, info)
