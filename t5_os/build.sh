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

export TUYA_APP_PATH=$APP_PATH
export TUYA_APP_NAME=$APP_BIN_NAME

p=$(pwd);p1=${p%%/vendor*};echo $p1
export TUYA_PROJECT_DIR=$p1

USER_SW_VER=`echo $APP_VERSION | cut -d'-' -f1`

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

if [ x$USER_CMD = "xclean" ];then
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
ori_value=f8f45b0779a8269fa089ac84ebd9c149
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
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TODO app_resource_config.json"
# app_config_file=${TUYA_PROJECT_DIR}/apps/$APP_BIN_NAME/app_resource_config.json
# out_file_path=${TUYA_APP_DEMO_PATH}/config/
# vendor_config_file=${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_default_config.json
# vendor_convert_script=${TUYA_PROJECT_DIR}/vendor/T5/t5_os/${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_default_config.py
# echo "python3 ${vendor_convert_script} ${out_file_path} ${vendor_config_file} ${app_config_file}"
# if [ -f $app_config_file ]; then
#     if ! python3 ${vendor_convert_script} ${out_file_path} ${vendor_config_file} ${app_config_file}; then
#         echo "update config error"
#         exit -1
#     fi
# else
#     echo "no app config, used default"
# fi
# 
# if [ -f ${out_file_path}/usr_gpio_cfg0.h ]; then
#     mv ${out_file_path}/usr_gpio_cfg0.h ${out_file_path}/bk7258/usr_gpio_cfg.h
# fi
#
# if [ -f ${out_file_path}/usr_gpio_cfg1.h ]; then
#     mv ${out_file_path}/usr_gpio_cfg1.h ${out_file_path}/bk7258_ap/usr_gpio_cfg.h
# fi
# 
# modified_config=${TUYA_PROJECT_DIR}/vendor/T5/t5_os/${TUYA_APP_DEMO_PATH}/tuya_scripts/tuya_modified_config.sh
# bash ${modified_config} ${app_config_file} ${out_file_path}

echo "Start Compile"

echo "make ${TARGET_PLATFORM} PROJECT=tuya_app BUILD_DIR=../build APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j"
make ${TARGET_PLATFORM} PROJECT=tuya_app APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j
res=$(echo $?)

if [ "x$res" != "x0" ]; then
    echo "make failed, $res"
    exit -1
fi

echo "Start Combined"

OUTPUT_PATH=""
if [ "x${CI_PACKAGE_PATH}" != "x" ]; then
    OUTPUT_PATH=${CI_PACKAGE_PATH}
else
    OUTPUT_PATH=../../../apps/$APP_BIN_NAME/output/$USER_SW_VER
fi

if [ ! -d "$OUTPUT_PATH" ]; then
    mkdir -p $OUTPUT_PATH
fi

DEBUG_FILE_PATH=${OUTPUT_PATH}/debug
if [ ! -d "$DEBUG_FILE_PATH" ]; then
    mkdir -p $DEBUG_FILE_PATH
fi

if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}" ]; then
    mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}
fi

if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_ap" ]; then
    mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_ap
fi


if [ -e "build/bk7258/tuya_app/package/all-app.bin" ]; then
    set -e

    TUYA_DIFF_OTA_BIN_TOOL=${TUYA_APP_DEMO_PATH}/tuya_scripts/diff2ya
    TUYA_FIRMWARE_CHECK=${TUYA_APP_DEMO_PATH}/tuya_scripts/firmware_check.py
    TUYA_FORMAT_BIN_TOOL=${TUYA_APP_DEMO_PATH}/tuya_scripts/format_up_bin.py
    TUYA_CREATE_UA_FILE_TOOL=${TUYA_APP_DEMO_PATH}/tuya_scripts/create_ua_file.py
    TUYA_GET_PARTITION_INFO=${TUYA_APP_DEMO_PATH}/tuya_scripts/get_partition_info.py
    TUYA_GET_SECTION_OFFSET=${TUYA_APP_DEMO_PATH}/tuya_scripts/get_map_section.py

    cp_bin_file=build/bk7258/tuya_app/bk7258/app.bin
    ap_bin_file=build/bk7258/tuya_app/bk7258_ap/app.bin
    bk_all_bin_file=build/bk7258/tuya_app/package/all-app.bin
    ty_final_bin_file=build/bk7258/tuya_app/package/prod.bin
    ua_bin_file=build/bk7258/tuya_app/package/ua_file.bin
    ug_bin_file=build/bk7258/tuya_app/package/ug_file.bin
    ty_ota_file=build/bk7258/tuya_app/package/ty_ug_file.bin
    ap_map_file=build/bk7258/tuya_app/bk7258_ap/app.map
    partiton_file=${TUYA_APP_DEMO_PATH}/partitions/bk7258/auto_partitions.csv

    ap_ty_section_addr=$(python3 ${TUYA_GET_SECTION_OFFSET} ${ap_map_file} _ty_section_start)
    ap_start_section_addr=$(python3 ${TUYA_GET_SECTION_OFFSET} ${ap_map_file} __vector_core0_table)

    split_point=$(printf "%d" "$((${ap_ty_section_addr}-${ap_start_section_addr}+1048576))") # cp + ap_without_ts2

    echo "ap_start_section_addr: ${ap_start_section_addr}"
    echo "ap_ty_section_addr: ${ap_ty_section_addr}"
    echo "split_point: ${split_point}"

    python3 ${TUYA_CREATE_UA_FILE_TOOL} ${partiton_file} ${cp_bin_file} ${ap_bin_file} --ua_file=${ua_bin_file}

    # python3 ${TUYA_FORMAT_BIN_TOOL} ${ua_bin_file} ${ug_bin_file} 360000 1000 0 1000 10D0 $split_point -v
    python3 ${TUYA_FORMAT_BIN_TOOL} ${ua_bin_file} ${ug_bin_file} 6b8000 1000 0 1000 10D0 $split_point -v

    ./${TUYA_DIFF_OTA_BIN_TOOL} ${ug_bin_file} ${ug_bin_file} ${ty_ota_file} 0 > /dev/null

    user_fs_bin=${TUYA_PROJECT_DIR}/apps/$APP_BIN_NAME/fs.bin
    if [ -f ${user_fs_bin} ]; then
        # 获取分区表中文件系统起始地址及长度
        fs_address=$(python $TUYA_GET_PARTITION_INFO $partiton_file "usr_config" "address")
        fs_limit_size=$(python $TUYA_GET_PARTITION_INFO $partiton_file "usr_config" "size")
        # 获取用户文件系统及长度，判断如果与分区表定义长度不一致，则报错
        fs_actual_size=$(stat -c%s "${user_fs_bin}")
        if [ $fs_actual_size -ne $fs_limit_size ]; then
            echo "fs.bin size is $fs_actual_size not equal to $fs_limit_size"
            exit -3
        fi

        # 振金平台需求，在烧录文件的末尾添加固件信息，长度128字节内，因此需要截断用户生成的文件系统末尾128字节
        # !!! 此处需注意，用户文件系统默认1M长度，但是最后128字节不可用
        fs_trunc_size=$(($fs_limit_size - 128))
        dd if="$user_fs_bin" of=trunc_fs bs=1 skip=0 count=$fs_trunc_size > /dev/null 2>&1

        # 拼接qio与文件系统，前置判断已经明确app-all.bin符合分区表限制，即all-app.bin不会与文件系统分区重叠
        all_app_size=$(stat -c%s $bk_all_bin_file)
        padding_size=$(($fs_address - $all_app_size))
        echo "merge fs.bin to $fs_address, padding size: $padding_size"
        dd if=/dev/zero bs=1 count=$padding_size 2>/dev/null | tr '\000' '\377' > padding
        cat $bk_all_bin_file padding trunc_fs > $ty_final_bin_file   # qio_with_fs.bin

        # 删除临时文件
        rm padding trunc_fs
    else
        cp $bk_all_bin_file $ty_final_bin_file
    fi

    # 在固件尾部追加固件校验信息
    script_file=${TUYA_PROJECT_DIR}/scripts/write_verid_to_bin.py
    if [ "x" != "x$TUYAOS_VERSION_ID" ] && [ -f ${script_file} ]; then
        cp $bk_all_bin_file                         $DEBUG_FILE_PATH/${TARGET_PLATFORM}/ori-all-app.bin
        cp $ty_ota_file                             $DEBUG_FILE_PATH/${TARGET_PLATFORM}/ori-app_ota_ug.bin

        cd ./build/${TARGET_PLATFORM}
        python3 ${script_file} $ty_final_bin_file
        python3 ${script_file} $ty_ota_file
        cd -
    fi

    cp $ty_final_bin_file                                   $OUTPUT_PATH/$APP_BIN_NAME"_QIO_"$USER_SW_VER.bin
    cp $ua_bin_file                                         $OUTPUT_PATH/$APP_BIN_NAME"_UA_"$USER_SW_VER.bin
    cp $ty_ota_file                                         $OUTPUT_PATH/$APP_BIN_NAME"_UG_"$USER_SW_VER.bin

    cp build/bk7258/tuya_app/bk7258/app*                    $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp build/bk7258/tuya_app/bk7258/size_map*               $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp build/bk7258/tuya_app/bk7258/sdkconfig               $DEBUG_FILE_PATH/${TARGET_PLATFORM}


    cp build/bk7258/tuya_app/bk7258_ap/app*                 $DEBUG_FILE_PATH/${TARGET_PLATFORM}_ap
    cp build/bk7258/tuya_app/bk7258_ap/size_map*            $DEBUG_FILE_PATH/${TARGET_PLATFORM}_ap
    cp build/bk7258/tuya_app/bk7258_ap/sdkconfig            $DEBUG_FILE_PATH/${TARGET_PLATFORM}_ap

    cp "projects/tuya_app/cp/config/bk7258/usr_gpio_cfg.h"  $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp projects/tuya_app/ap/config/bk7258_ap/usr_gpio_cfg.h $DEBUG_FILE_PATH/${TARGET_PLATFORM}_ap

    echo "*************************************************************************"
    echo "**********************COMPILE SUCCESS************************************"
    echo "*************************************************************************"
fi

disable_python_env "tuya_build_env"

