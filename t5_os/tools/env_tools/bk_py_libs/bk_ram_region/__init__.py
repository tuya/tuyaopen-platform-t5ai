import logging

from .bk_ram_region import (
    bk_ram_region,
    mem_region,
)

__version__ = "0.0.1"

__all__ = ["bk_ram_region", "mem_region"]


def set_debug_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)
