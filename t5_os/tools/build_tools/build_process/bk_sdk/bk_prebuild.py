import logging
import os
import shutil
import sys
from pathlib import Path


logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def copy_file(src: str, dst: str):
    logger.debug(f"copy {src} to {dst}")
    logger.debug(f"to {dst}")
    if os.path.exists(f"{src}"):
        shutil.copy(f"{src}", f"{dst}")


def copy_files(postfix: str, src_dir: str, dst_dir: str):
    logger.debug(f"copy *{postfix} from {src_dir}")
    logger.debug(f"to {dst_dir}")
    for f in os.listdir(src_dir):
        if f.endswith(postfix):
            if os.path.isfile(f"{src_dir}/{f}"):
                shutil.copy(f"{src_dir}/{f}", f"{dst_dir}/{f}")


def install_configs(cfg_dir: Path, install_dir: Path):
    logger.debug(f"install configs from: {cfg_dir}")
    logger.debug(f"to: {install_dir}")
    if not cfg_dir.exists():
        return

    if os.path.exists(f"{cfg_dir}/partitions.csv") and os.path.exists(
        f"{install_dir}/partitions.csv"
    ):
        logger.debug(f"exists {cfg_dir}/partitions.csv")
        logger.debug(f"remove {install_dir}/partitions.csv")
        os.remove(f"{install_dir}/partitions.csv")

    if os.path.exists(f"{cfg_dir}/csv/partitions.csv") and os.path.exists(
        f"{install_dir}/csv/partitions.csv"
    ):
        logger.debug(f"exists {cfg_dir}/csv/partitions.csv")
        logger.debug(f"remove {install_dir}/partitions.csv")
        os.remove(f"{install_dir}/csv/partitions.csv")

    for f in os.listdir(f"{cfg_dir}"):
        if (
            f.endswith(".csv")
            or f.endswith(".bin")
            or f.endswith(".json")
            or f.endswith(".pem")
        ):
            if f == "partitions.csv":
                continue

            if os.path.isfile(f"{cfg_dir}/{f}"):
                shutil.copy(f"{cfg_dir}/{f}", f"{install_dir}/{f}")

    if os.path.exists(f"{cfg_dir}/key"):
        copy_files(".pem", f"{cfg_dir}/key", str(install_dir))
    if os.path.exists(f"{cfg_dir}/csv"):
        copy_files(".csv", f"{cfg_dir}/csv", str(install_dir))
    if os.path.exists(f"{cfg_dir}/regs"):
        copy_files(".csv", f"{cfg_dir}/regs", str(install_dir))


def prebuild():
    from bk_curr_project import curr_project

    soc_name: str = curr_project.soc_name
    tools_dir = curr_project.tools_path
    cpu0_armino_soc = curr_project.app0_name

    app_name = sys.argv[1]
    cmake_partition_bin_dir = (
        curr_project.project_build_dir / app_name / "armino/partitions/_build"
    )

    middleware_soc_cfg_dir = curr_project.get_middleware_soc_config_path(app_name)
    project_partitions_dir = curr_project.partitions_dir

    logger.debug(f"tools_dir={tools_dir}")
    logger.debug(f"base_cfg_dir={middleware_soc_cfg_dir}")
    logger.debug(f"prefered_cfg_dir={project_partitions_dir}")
    logger.debug(f"soc={soc_name}")

    bk_utils_script = tools_dir / "env_tools/beken_utils/main.py"
    logger.debug("Create temporary _build")
    os.makedirs(cmake_partition_bin_dir, exist_ok=True)
    os.chdir(cmake_partition_bin_dir)
    logger.debug(f"cd {cmake_partition_bin_dir}")
    copy_file(
        f"{middleware_soc_cfg_dir}/partitions/bl1_control.json",
        f"{cmake_partition_bin_dir}/bl1_control.json",
    )

    install_configs(middleware_soc_cfg_dir, cmake_partition_bin_dir)
    install_configs(project_partitions_dir, cmake_partition_bin_dir)
    install_configs(project_partitions_dir / "common", cmake_partition_bin_dir)
    install_configs(project_partitions_dir / cpu0_armino_soc, cmake_partition_bin_dir)
    logger.debug("partition pre-processing")
    ret = os.system(f"python3 {bk_utils_script} gen all --debug")
    if ret != 0:
        raise RuntimeError(f"run {bk_utils_script} fail")


if __name__ == "__main__":
    set_logging()
    sys.path.append(str(Path(__file__).parent.parent))
    prebuild()
