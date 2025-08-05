import logging

from .bk_build_summary import bk_build_summary

__version__ = "0.0.1"

__all__ = ["bk_build_summary"]


def set_debug_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)
