import logging

from .bk_flash_partition import (
    bk_flash_partition,
    bk_flash_partition_content_generator,
    partition_info,
)

__version__ = "0.0.1"

__all__ = [
    "bk_flash_partition",
    "bk_flash_partition_content_generator",
    "partition_info",
]


def set_debug_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)
