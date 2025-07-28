from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path

import bk_packager


@dataclass
class app_info:
    app_name: str  # eg. bk7258 bk7258_ap
    app_name_in_sdk: str  # eg. cp ap
    build_bin: Path
    pack_bin_name: str


class bk_project(ABC):
    def __init__(self) -> None:
        self._summary = ""

    @property
    @abstractmethod
    def project_name(self) -> str: ...

    @property
    @abstractmethod
    def soc_name(self) -> str: ...

    @property
    @abstractmethod
    def sdk_path(self) -> Path: ...

    @property
    @abstractmethod
    def project_path(self) -> Path: ...

    @property
    @abstractmethod
    def partitions_dir(self) -> Path: ...

    @property
    @abstractmethod
    def project_build_dir(self) -> Path: ...

    @property
    @abstractmethod
    def project_build_parititons_dir(self) -> Path: ...

    @property
    @abstractmethod
    def project_build_package_dir(self) -> Path: ...

    @property
    @abstractmethod
    def auto_partitions_table(self) -> Path: ...

    @property
    @abstractmethod
    def ram_regions_table(self) -> Path: ...

    @property
    @abstractmethod
    def bootloader_archive_path(self) -> Path: ...

    @property
    @abstractmethod
    def bootloader_build_path(self) -> Path: ...

    @property
    @abstractmethod
    def bootloader_backup_dir(self) -> Path: ...

    @property
    @abstractmethod
    def app0_name(self) -> str: ...

    @property
    @abstractmethod
    def app0_src_root_path(self) -> Path: ...

    @property
    def build_summary(self) -> str:
        return self._summary

    @build_summary.setter
    def build_summary(self, value: str):
        self._summary = value

    @property
    @abstractmethod
    def tools_path(self) -> Path: ...

    @property
    @abstractmethod
    def is_ab_project(self) -> bool: ...

    @property
    @abstractmethod
    def flash_crc_enable(self) -> bool: ...

    @property
    @abstractmethod
    def ram_regions_setting(self) -> Path: ...

    @property
    @abstractmethod
    def flash_partitions_setting(self) -> Path: ...

    @property
    @abstractmethod
    def apps_info(self) -> list[app_info]: ...

    @abstractmethod
    def get_middleware_soc_config_path(self, app_name: str) -> Path: ...

    @abstractmethod
    def pre_auto_partition(self) -> None: ...

    @abstractmethod
    def post_auto_partition(self) -> None: ...

    @abstractmethod
    def pre_package(self) -> None: ...

    @abstractmethod
    def post_package(self) -> None: ...

    def get_packager(
        self, pack_dir: Path, pack_json: Path, output_bin: Path
    ) -> bk_packager.bk_packager:
        if self.flash_crc_enable:
            packager = bk_packager.bk_packager_linear_crc(
                pack_dir, pack_json, output_bin
            )
        else:
            packager = bk_packager.bk_packager_linear(pack_dir, pack_json, output_bin)
        return packager

    def _copy_bootloader_to_pack_dir(self, pack_dir: Path):
        origin_path = self.bootloader_archive_path
        pack_path = pack_dir / self.bootloader_archive_path.name
        self._copy_binaries(origin_path, pack_path)

    def copy_binaries_to_pack_dir(self, pack_dir: Path) -> None:
        self._copy_bootloader_to_pack_dir(pack_dir)
        app_list = self.apps_info
        for app in app_list:
            app_build_bin = app.build_bin
            app_pack_bin = pack_dir / app.pack_bin_name
            self._copy_binaries(app_build_bin, app_pack_bin)

    @staticmethod
    def _copy_binaries(origin_path: Path, pack_path: Path):
        import shutil

        if not origin_path.exists():
            raise FileNotFoundError(f"{origin_path} not found.")
        shutil.copy(origin_path, pack_path)
