from __future__ import annotations

from pathlib import Path
import argparse
import sys
import os
import shutil


def get_project():
    from bk_sdk_project import bk_project_info, bk_sdk_project
    parser = argparse.ArgumentParser()
    parser.add_argument("project_dir", type=Path)
    parser.add_argument("build_dir", type=Path)
    parser.add_argument("soc_name", type=str)
    args = parser.parse_args()

    project_dir: Path = args.project_dir
    build_dir: Path = args.build_dir
    soc_name: str = args.soc_name

    project_build_dir = build_dir / soc_name / project_dir.name
    project_build_dir.mkdir(parents=True, exist_ok=True)
    assert project_dir.exists()
    assert build_dir.exists()

    project_name = project_dir.name
    project_build_dir = build_dir / soc_name / project_name

    project_info = bk_project_info(
        project_name,
        project_dir,
        project_build_dir,
        soc_name,
        True,
        [soc_name, f"{soc_name}_ap"],
    )
    return bk_sdk_project(project_info)


def main():
    project = get_project()
    bootloader_path = project.bootloader_build_path.parent.parent
    os.system(f"make build_prepare ARMINO_SOC_NAME={project.soc_name} PROJECT_DIR={project.project_path} -C {project.sdk_path}")
    ota_json_src = project.project_build_parititons_dir / "bk_ota_partitions.json"
    ota_json_dst = bootloader_path / "tools/partition_ota.json"
    shutil.copy(ota_json_src, ota_json_dst)
    os.system(f"make attach_part_table -C {bootloader_path}")


if __name__ == "__main__":
    sys.path.append(Path(__file__).parent.parent.as_posix())
    main()
