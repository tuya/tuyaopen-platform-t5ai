
__version__ = '0.0.1'

import logging

def setup_logging():
    root_logger = logging.getLogger()
    logger = logging.getLogger(__name__)

    if len(root_logger.handlers) == 0:
        console_handler = logging.StreamHandler()
        formatter = logging.Formatter('[%(name)s|%(levelname)s] %(message)s')
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)
        logger.setLevel(logging.INFO)

    return logger

def set_debug_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

def set_info_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)

from .file_hash import get_file_md5sum
