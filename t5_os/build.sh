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

tmp_gen_files_list=bk_idk/tools/build_tools/part_table_tools/config/gen_files_list.txt
if [ -f $tmp_gen_files_list ]; then
    rm $tmp_gen_files_list
fi
touch $tmp_gen_files_list

if [ -z $CI_PACKAGE_PATH ]; then
    echo "not is ci build"
else
    make clean
	make clean -C ./bk_idk/
fi

if [ x$USER_CMD = "xclean" ];then
	make clean
	make clean -C ./bk_idk/
    git checkout build/*
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

echo "------ enable uac ------"
TUYA_APP_DEMO_PATH=projects/tuya_app

ty_lwip=$(grep -wcE "#define *ENABLE_LWIP" ${sdk_config_file})
if [ "x$ty_lwip" = "x1" ]; then
    echo "------ use tuya lwip ------"
    export TUYA_LWIP_STACK_USED="lwip_tuya"
    make tuya_lwip -f ${TUYA_APP_DEMO_PATH}/tuya_scripts/env.mk
else
    echo "------ use bk lwip ------"
    export TUYA_LWIP_STACK_USED="lwip_bk"
    cp ${TUYA_APP_DEMO_PATH}/config/${TARGET_PLATFORM}/config .${TARGET_PLATFORM}_config.save
    sed -i "s/CONFIG_LWIP=n/CONFIG_LWIP=y/g" ${TUYA_APP_DEMO_PATH}/config/${TARGET_PLATFORM}/config
    sed -i "s/CONFIG_LWIP_V2_1=n/CONFIG_LWIP_V2_1=y/g" ${TUYA_APP_DEMO_PATH}/config/${TARGET_PLATFORM}/config
fi

echo "CHECK COMPONENTS"
is_componenst_file_exist=0
if [ -f "${TUYA_PROJECT_DIR}/apps/$APP_BIN_NAME/components.mk" ]; then
    is_componenst_file_exist=1
    comp_dir=""
    if [ -d "${TUYA_PROJECT_DIR}/application_components" ]; then
        comp_dir=application_components
    elif [ -d "${TUYA_PROJECT_DIR}/components" ]; then
        comp_dir=components
    fi # comp_dir
    echo "components path: $comp_dir"

    make tuya_components -f ${TUYA_APP_DEMO_PATH}/tuya_scripts/env.mk COMPONENTS_PATH=$comp_dir APP=$APP_BIN_NAME

fi # components.mk

# export PYTHONPATH=${TUYA_PROJECT_DIR}/vendor/${TARGET_PLATFORM}/toolchain/site-packages:${PYTHONPATH}
export PYTHONPATH=${TUYA_PROJECT_DIR}/vendor/T5/toolchain/site-packages:${PYTHONPATH}

echo "APP_DIR:"$APP_DIR

echo "check bootloader.bin"
boot_file=bk_idk/components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin
check_value=$(md5sum ${boot_file} | awk '{print $1}')
ori_value=349ed5c2be62376f843cae86dc913713
if [ "x${check_value}" != "x${ori_value}" ]; then
    echo -e "\033[1;31m bootloader.bin check failed, the file had been changed, please update md5 value in build.sh \033[0m"
    exit
else
    echo "bootloader check ok"
fi

echo "Start Compile"

echo "make ${TARGET_PLATFORM} PROJECT_DIR=../${TUYA_APP_DEMO_PATH} BUILD_DIR=../build APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j"
make ${TARGET_PLATFORM} PROJECT_DIR=../${TUYA_APP_DEMO_PATH} BUILD_DIR=../build APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j
res=$(echo $?)
rm $tmp_gen_files_list

# delete file whether it exists
rm -f ${TUYA_APP_DEMO_PATH}/.tmp_component_desc
if [ "x${TUYA_LWIP_STACK_USED}" = "xlwip_bk" ]; then
    mv .${TARGET_PLATFORM}_config.save ${TUYA_APP_DEMO_PATH}/config/${TARGET_PLATFORM}/config
else
    rm -rf ../tuyaos/tuya_os_adapter/lwip_intf_v2_1
fi

if [ $res -ne 0 ]; then
    echo "make failed"
    exit -1
fi

echo "Start Combined"

OUTPUT_PATH=""
if [ "x${CI_PACKAGE_PATH}" != "x" ]; then
    echo "ci build"
    OUTPUT_PATH=${CI_PACKAGE_PATH}
else
    echo "local build"
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

if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp1" ]; then
    mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp1
fi

#if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp2" ]; then
#    mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp2
#fi


if [ -e "./build/${TARGET_PLATFORM}/all-app.bin" ]; then
    set -x
    set -e

    ofs=$(stat -c %s ./build/${TARGET_PLATFORM}/app.bin)
    # TODO 1920k = 1966080 bytes
    app0_max_size=1966080
    if [ $ofs -gt $app0_max_size ]; then
        echo "app0 file is too big, limit $app0_max_size, act $ofs"
        exit -1
    fi

    pad_bytes_size=$(expr $app0_max_size - $ofs)
    dd if=/dev/zero bs=1 count=${pad_bytes_size} | tr "\000" "\377" > pad_bin_file

    cat ./build/${TARGET_PLATFORM}/app.bin pad_bin_file ./build/${TARGET_PLATFORM}/app1.bin > ./build/${TARGET_PLATFORM}/ua_file.bin
    total_size=$(stat -c %s ./build/${TARGET_PLATFORM}/ua_file.bin)

    echo "ofs: ${ofs}"
    echo "pad_bytes_size: ${pad_bytes_size}"
    echo "total_size: ${total_size}"

    TUYA_FORMAT_BIN_TOOL=${TUYA_APP_DEMO_PATH}/tuya_scripts/format_up_bin.py
    TUYA_DIFF_OTA_BIN_TOOL=${TUYA_APP_DEMO_PATH}/tuya_scripts/diff2ya

    python3 ${TUYA_FORMAT_BIN_TOOL} ./build/${TARGET_PLATFORM}/ua_file.bin ./build/${TARGET_PLATFORM}/app_ug.bin 500000 1000 0 1000 18D0 $app0_max_size -v
    ./${TUYA_DIFF_OTA_BIN_TOOL} ./build/${TARGET_PLATFORM}/app_ug.bin ./build/${TARGET_PLATFORM}/app_ug.bin ./build/${TARGET_PLATFORM}/app_ota_ug.bin 0

    rm pad_bin_file

    cp ./build/${TARGET_PLATFORM}/all-app.bin       $OUTPUT_PATH/$APP_BIN_NAME"_QIO_"$USER_SW_VER.bin
    cp ./build/${TARGET_PLATFORM}/ua_file.bin       $OUTPUT_PATH/$APP_BIN_NAME"_UA_"$USER_SW_VER.bin
    cp ./build/${TARGET_PLATFORM}/app_ota_ug.bin    $OUTPUT_PATH/$APP_BIN_NAME"_UG_"$USER_SW_VER.bin

    cp ./build/${TARGET_PLATFORM}/app*              $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp ./build/${TARGET_PLATFORM}/size_map*         $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp ./build/${TARGET_PLATFORM}/sdkconfig         $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp ./build/${TARGET_PLATFORM}/bootloader.bin    $DEBUG_FILE_PATH/${TARGET_PLATFORM}

    cp ./build/${TARGET_PLATFORM}_cp1/app*          $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp1
    cp ./build/${TARGET_PLATFORM}_cp1/size_map*     $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp1
    cp ./build/${TARGET_PLATFORM}_cp1/sdkconfig     $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp1

#    cp ./build/${TARGET_PLATFORM}_cp2/app*          $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp2
#    cp ./build/${TARGET_PLATFORM}_cp2/size_map*     $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp2
#    cp ./build/${TARGET_PLATFORM}_cp2/sdkconfig     $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp2
fi

echo "*************************************************************************"
echo "*************************************************************************"
echo "*************************************************************************"
echo "*********************${APP_BIN_NAME}_$APP_VERSION.bin********************"
echo "*************************************************************************"
echo "**********************COMPILE SUCCESS************************************"
echo "*************************************************************************"

# Modified by TUYA End

