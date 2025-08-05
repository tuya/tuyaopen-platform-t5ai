import logging
import sys
import unittest
from pathlib import Path


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    # 设置默认打印等级
    logging.basicConfig(format=log_format, level=logging.INFO)


def TestMain():
    """通过默认加载器"""
    suite = unittest.defaultTestLoader.discover(str(Path(__file__).resolve().parent))
    runner = unittest.TextTestRunner(verbosity=1)
    runner.run(suite)


if __name__ == "__main__":
    set_logging()
    currPath = Path(__file__).resolve().parent
    print(currPath)
    packagePath = currPath.parent
    sys.path.append(str(packagePath))
    packagePath = currPath.parent.parent
    sys.path.append(str(packagePath))
    TestMain()
