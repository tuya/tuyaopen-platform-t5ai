import importlib.util
import logging
import sys
from pathlib import Path

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def test_py_libs() -> bool:
    curr_dir = Path(__file__).parent.absolute()
    test_script = curr_dir.parent / "bk_py_libs/ut/ut_all.py"
    spec = importlib.util.spec_from_file_location("test_py_libs", test_script)
    if spec is None or spec.loader is None:
        raise RuntimeError("import error")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    ret = module.test_py_libs()
    if not ret:
        logger.error("bk_py_libs test fail")
    return ret


def main():
    test_results: list[bool] = []
    test_results.append(test_py_libs())
    if not all(test_results):
        sys.exit(-1)


if __name__ == "__main__":
    set_logging()
    main()
