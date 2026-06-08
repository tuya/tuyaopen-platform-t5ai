#!/bin/sh

# Modified by TUYA Start
APP_BIN_NAME=$1
APP_VERSION=$2
TARGET_PLATFORM=$3
APP_PATH=../../../$4
USER_CMD=$5

TARGET_PLATFORM=bk7258

echo APP_BIN_NAME=$APP_BIN_NAME
echo APP_VERSION=$APP_VERSION
echo TARGET_PLATFORM=$TARGET_PLATFORM
echo APP_PATH=$APP_PATH
echo USER_CMD=$USER_CMD

USER_SW_VER=`echo $APP_VERSION | cut -d'-' -f1`

p=$(pwd);p1=${p%%/vendor*};echo $p1

export TUYA_PROJECT_DIR=$p1
export TUYA_APP_PATH=$APP_PATH
export TUYA_APP_NAME=$APP_BIN_NAME
export TUYA_APP_VERSION=$USER_SW_VER
export TUYA_APP_PLATFORM=$TARGET_PLATFORM

current_env_inifo=$(uname -a)
echo "env info: ${current_env_inifo}"

APP_DIR_temp=$(echo $APP_PROJ_PATH)
if [ "x$APP_DIR_temp" != "x" ];then
    last_character=$(echo -n $APP_PROJ_PATH | tail -c 1)
    if [ "x$last_character" = 'x/' ];then
        APP_DIR_temp=${APP_PROJ_PATH%?}
    else
        APP_DIR_temp=$APP_PROJ_PATH
    fi
    APP_DIR=${APP_DIR_temp%/*}
else
    APP_DIR=apps
fi

APP_PATH=../../$APP_DIR

# Remove TUYA APP OBJs first
if [ -e "${APP_OBJ_PATH}/$APP_BIN_NAME/src" ]; then
    for i in `find ${APP_OBJ_PATH}/$APP_BIN_NAME/src -type d`; do
        echo "Deleting $i"
        rm -rf $i/*.o
    done
fi

if [ -z $CI_PACKAGE_PATH ]; then
    echo "not is ci build"
else
    make clean
fi

function_clean() {
    # save sdkconfig.h
    mkdir -p .tmp_build/bk7258/tuya_app/bk7258/armino_as_lib/bk7258/config/
    mkdir -p .tmp_build/bk7258/tuya_app/bk7258/config/
    mkdir -p .tmp_build/bk7258/tuya_app/bk7258_ap/armino_as_lib/bk7258_ap/config/
    mkdir -p .tmp_build/bk7258/tuya_app/bk7258_ap/config/

    cp build/bk7258/tuya_app/bk7258/armino_as_lib/bk7258/config/sdkconfig.h         .tmp_build/bk7258/tuya_app/bk7258/armino_as_lib/bk7258/config/
    cp build/bk7258/tuya_app/bk7258/config/sdkconfig.h                              .tmp_build/bk7258/tuya_app/bk7258/config/
    cp build/bk7258/tuya_app/bk7258_ap/armino_as_lib/bk7258_ap/config/sdkconfig.h   .tmp_build/bk7258/tuya_app/bk7258_ap/armino_as_lib/bk7258_ap/config/
    cp build/bk7258/tuya_app/bk7258_ap/config/sdkconfig.h                           .tmp_build/bk7258/tuya_app/bk7258_ap/config/

    make clean
    mv .tmp_build build
}

if [ x$USER_CMD = "xclean" ];then
    function_clean
    echo "*************************************************************************"
    echo "************************CLEAN SUCCESS************************************"
    echo "*************************************************************************"
    exit 0
fi

# lwip check
sdk_config_file=${TUYA_PROJECT_DIR}/include/base/include/tuya_iot_config.h
if [ ! -f ${sdk_config_file} ]; then
    echo "${sdk_config_file} is not exist"
    exit -1;
fi

TUYA_APP_DEMO_PATH=projects/tuya_app

echo "APP_DIR:"$APP_DIR

boot_file=cp/components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin
check_value=$(md5sum ${boot_file} | awk '{print $1}')
ori_value=04071779f81970998f2b7c0a00fe5d76
if [ "x${check_value}" != "x${ori_value}" ]; then
    echo -e "\033[1;31m bootloader.bin check failed, the file had been changed, please update md5 value in build.sh \033[0m"
    exit
else
    echo "bootloader check ok"
fi

# python 虚拟环境
PYTHON_CMD="python3"
check_python_install() {

    if command -v python3 >/dev/null; then
        PYTHON_CMD=python3
    elif command -v python >/dev/null && python --version | grep -q '^Python 3'; then
        PYTHON_CMD=python
    else
        echo "Python 3 is not installed. Please run: "
        echo ""
        echo "$ sudo apt-get install python3 -y"
        echo ""
        if [ -d "projects/tuya_app/tuya_build_env" ]; then
            rm -rf projects/tuya_app/tuya_build_env
        fi
        exit 1
    fi

    python_version=$(${PYTHON_CMD} --version 2>&1 | cut -d' ' -f2 | cut -d. -f1-2)
    formatted_version="python${python_version}-venv"
    echo "Python version: ${python_version}"

    major=$(echo "$python_version" | cut -d. -f1)
    minor=$(echo "$python_version" | cut -d. -f2)

    if [ "$major" -lt 3 ] || { [ "$major" -eq 3 ] && [ "$minor" -lt 8 ]; }; then
        echo "Error: Current Python version (${python_version}) is less than 3.8, please upgrade"
        if [ -d "projects/tuya_app/tuya_build_env" ]; then
            rm -rf projects/tuya_app/tuya_build_env
        fi
        exit 1
    fi

    if apt list --installed | grep -q "^${formatted_version}/"; then
        echo "${formatted_version} is installed, continuing with the script..."
    else
        if [ -d "projects/tuya_app/tuya_build_env" ]; then
            rm -rf projects/tuya_app/tuya_build_env
        fi
        echo "python3-venv is not installed. Please run:"
        echo ""
        echo "$ sudo apt-get install python3-venv -y"
        echo ""
        exit 1
    fi 
}

enable_python_env() {
    if [ -z $1 ]; then
        echo "Please input virtual environment name."
        exit 1
    fi

    VIRTUAL_NAME=$1
    SCRIPT_DIR=$PWD/${TUYA_APP_DEMO_PATH}
    VIRTUAL_ENV=$SCRIPT_DIR/$VIRTUAL_NAME

    if [ ! -d "${VIRTUAL_ENV}" ]; then
        echo "Virtual environment not found. Creating one..."
        $PYTHON_CMD -m venv "${VIRTUAL_ENV}" || { echo "Failed to create virtual environment."; exit 1; }
        echo "Virtual environment created at ${VIRTUAL_ENV}"
    else
        echo "Virtual environment already exists."
    fi

    ACTIVATE_SCRIPT=${VIRTUAL_ENV}/bin/activate
    PIP_CMD=${VIRTUAL_ENV}/bin/pip3
    if [ -f "$ACTIVATE_SCRIPT" ] && [ -f ${PIP_CMD} ]; then
        echo "Activate python virtual environment."
        . ${ACTIVATE_SCRIPT} || { echo "Failed to activate virtual environment."; exit 1; }
        ${PIP_CMD} install -r "projects/tuya_app/tuya_scripts/requirements.txt" || { echo "Failed to install required Python packages."; deactivate; exit 1; }
    else
        echo "Activate script not found."
        rm -rf "${VIRTUAL_ENV}"
        $PYTHON_CMD -m venv "${VIRTUAL_ENV}" || { echo "Failed to create virtual environment."; exit 1; }
        . ${ACTIVATE_SCRIPT} || { echo "Failed to activate virtual environment."; exit 1; }
        ${PIP_CMD} install -r "requirements.txt" || { echo "Failed to install required Python packages."; deactivate; exit 1; }
    fi
}

disable_python_env() {
    if [ -z $1 ]; then
        echo "Please input virtual environment name."
        exit 1
    fi

    VIRTUAL_NAME=$1
    SCRIPT_DIR=$PWD/${TUYA_APP_DEMO_PATH}
    VIRTUAL_ENV=$SCRIPT_DIR/$VIRTUAL_NAME

    # echo "SCRIPT_DIR $VIRTUAL_ENV"
    if [ -n "$VIRTUAL_ENV" ]; then
        # echo "Deactivate python virtual environment."
        deactivate
    else
        echo "No virtual environment is active."
    fi
}

check_python_install ||  { echo "Failed to check python environment."; exit 1; }

if [ -z $CI_PACKAGE_PATH ]; then
    enable_python_env "tuya_build_env" || { echo "Failed to enable python virtual environment."; exit 1; }
else
    export PYTHONPATH=${TUYA_PROJECT_DIR}/vendor/T5/toolchain/site-packages:${PYTHONPATH}
fi

# 业务需求：提前初始化gpio
app_config_file=${TUYA_PROJECT_DIR}/apps/$APP_BIN_NAME/app_resource_config.json
export TUYA_APP_CONFIG_FILE=${app_config_file}
default_gpio_config_file=${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_default_gpio_config.json
update_gpio_config_script=${TUYA_PROJECT_DIR}/vendor/T5/t5_os/${TUYA_APP_DEMO_PATH}/tuya_scripts/user_gpio_config.py
gpio_config_file=${TUYA_APP_DEMO_PATH}/ap/config/${TARGET_PLATFORM}_ap/usr_gpio_cfg.h
echo "python3 ${update_gpio_config_script} ${app_config_file} ${default_gpio_config_file} -o ${gpio_config_file}"
if ! python3 ${update_gpio_config_script} ${app_config_file} ${default_gpio_config_file} -o ${gpio_config_file}; then
    echo "update config error"
    exit -1
fi

# compress_mode_file=${TUYA_APP_DEMO_PATH}/tuya_scripts/files/tuya_ota_compress_table.csv
# partition_mode_file=${TUYA_APP_DEMO_PATH}/tuya_scripts/files/tuya_ota_partition_table.csv
# 解析 app_resource_config.json 文件，获取ota模式
# 目标文件: vendor/T5/t5_os/projects/tuya_app/partitions/bk7258/auto_partitions.csv
# 判断文件是否相投，不相同则需要覆盖替换文件，并在t5_os目录下执行make clean操作
# 如果是纯压缩模式，把 compress_mode_file文件覆盖auto_partitions.csv
# 如果是分段模式，把 partition_mode_file文件覆盖auto_partitions.csv
tuya_partition_table_select=${TUYA_PROJECT_DIR}/vendor/T5/t5_os/${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_partition_table_select.py
if ! python3 ${tuya_partition_table_select} ${app_config_file}; then
    echo "select partition table error"
    exit -1
fi

echo "Start Compile"

echo "make ${TARGET_PLATFORM} PROJECT=tuya_app BUILD_DIR=../build APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j"
make ${TARGET_PLATFORM} PROJECT=tuya_app APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j
res=$(echo $?)

if [ "x$res" != "x0" ]; then
    echo "make failed, $res"
    exit -1
fi

# 分析并输出内存使用信息
echo "CP Build Info"
CP_ELF_FILE="build/bk7258/tuya_app/bk7258/app.elf"
CP_MEMORY_FILE="build/bk7258/tuya_app/bk7258/app_memory.txt"
if [ -f "${CP_ELF_FILE}" ]; then
    if [ -f "${CP_MEMORY_FILE}" ]; then
        # 模式1: 使用 --toolchain_memory_usage_file 参数（链接器生成的内存使用信息）
        # 需要工具链配置 --print-memory-usage 选项: -Wl,--print-memory-usage > app_memory.txt
        python3 ${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_analyze_elf_memory.py "${CP_ELF_FILE}" --toolchain_memory_usage_file "${CP_MEMORY_FILE}"
    else
        # 模式2: 直接分析 ELF 文件
        python3 ${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_analyze_elf_memory.py "${CP_ELF_FILE}"
    fi
else
    echo "Warning: CP ELF file not found: ${CP_ELF_FILE}"
fi

echo ""
echo "AP Build Info"
AP_ELF_FILE="build/bk7258/tuya_app/bk7258_ap/app.elf"
AP_MEMORY_FILE="build/bk7258/tuya_app/bk7258_ap/app_memory.txt"
if [ -f "${AP_ELF_FILE}" ]; then
    if [ -f "${AP_MEMORY_FILE}" ]; then
        # 模式1: 使用 --toolchain_memory_usage_file 参数（链接器生成的内存使用信息）
        python3 ${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_analyze_elf_memory.py "${AP_ELF_FILE}" --toolchain_memory_usage_file "${AP_MEMORY_FILE}"
    else
        # 模式2: 直接分析 ELF 文件
        python3 ${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_analyze_elf_memory.py "${AP_ELF_FILE}"
    fi
else
    echo "Warning: AP ELF file not found: ${AP_ELF_FILE}"
fi

disable_python_env "tuya_build_env"

echo "\n*************************************************************************"
echo "    $APP_BIN_NAME"_"$USER_SW_VER.bin compile success"
echo "*************************************************************************"
