import os
from pathlib import Path

from bk_sdk.bk_sdk_project import bk_project_info, bk_sdk_project


def get_curr_project():
    if not os.getenv("PROJECT_DIR"):
        raise RuntimeError("get PROJECT_DIR error")
    project_dir = Path(os.getenv("PROJECT_DIR", ""))
    soc_name = os.getenv("ARMINO_SOC_NAME", "")
    project_name = os.getenv("PROJECT_NAME", "")
    project_build_dir = Path(os.getenv("PROJECT_BUILD_DIR", ""))

    if not project_dir:
        raise RuntimeError("get project dir error")
    if not project_build_dir:
        raise RuntimeError("get build dir error")

    project_info = bk_project_info(
        project_name,
        project_dir,
        project_build_dir,
        soc_name,
        True,
        [soc_name, f"{soc_name}_ap"],
    )
    return bk_sdk_project(project_info)


curr_project = get_curr_project()
