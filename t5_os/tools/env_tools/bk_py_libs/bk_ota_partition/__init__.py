import logging

from .bk_ota_partition import bk_ota_partition

__version__ = "0.0.3"

__all__ = ["bk_ota_partition"]


def set_debug_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)
