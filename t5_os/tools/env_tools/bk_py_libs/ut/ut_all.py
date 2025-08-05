import logging
import sys
import unittest
from pathlib import Path


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def TestMain() -> bool:
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    currPath = Path(__file__).resolve().parent
    for item in currPath.iterdir():
        if item.is_dir() and (item / "ut_main.py").exists():
            sys.path.append(str(item))
            discovered_tests = loader.discover(str(item), top_level_dir=str(currPath))
            suite.addTests(discovered_tests)

    runner = unittest.TextTestRunner(verbosity=1)
    ret = runner.run(suite)
    return ret.wasSuccessful()


def test_py_libs() -> bool:
    currPath = Path(__file__).resolve().parent
    packagePath = currPath
    sys.path.append(str(packagePath))
    packagePath = currPath.parent
    sys.path.append(str(packagePath))
    return TestMain()


if __name__ == "__main__":
    set_logging()
    test_py_libs()
