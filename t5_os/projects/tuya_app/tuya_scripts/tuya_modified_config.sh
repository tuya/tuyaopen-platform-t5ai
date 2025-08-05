#! /bin/bash
#
# tuya_modified_config.sh
# Copyright (C) 2025 cc <cc@tuya>
#
# Distributed under terms of the MIT license.
#

if [ ! -f $1 ]; then
    echo "no config file"
fi

if [ "x$2" == "x" ]; then
    echo "no config path"
fi

set -x
set -e

config_file=$1
config_path=$2

config_list=("CONFIG_LV_COLOR_16_SWAP" "CONFIG_LCD_SPI_DISPLAY")
conf0_file=${config_path}/bk7258/config
conf1_file=${config_path}/bk7258_cp1/config
conf2_file=${config_path}/bk7258_cp2/config

for i in "${config_list[@]}"; do
    # 1, match user config file, and get value
    user_value=$(grep $i ${config_file} | awk -F'"' '{print $4}')
    if [ "x${user_value}" != "xn" ] && [ "x${user_value}" != "xy" ]; then
        echo "not found $i in user config"
        continue
    fi

    # 2, match item in config, and get value
    default0_value=$(grep $i $conf0_file | awk -F'=' '{print $2}')
    if [ "x${default0_value}" != "xn" ] && [ "x${default0_value}" != "xy" ] && [ "x${default0_value}" != "x" ]; then
        continue
    fi
    # compare value, if not equal, modified
    if [ "x${user_value}" != "x${default0_value}" ]; then
        # Modify
        sed -i "s/${i}=[ny]$/${i}=${user_value}/" ${conf0_file}
    fi

    default1_value=$(grep $i ${conf1_file} | awk -F'=' '{print $2}')
    if [ "x${default1_value}" != "xn" ] && [ "x${default1_value}" != "xy" ]; then
        echo "xxx not found ${i} xxxx ${default1_value} "
        continue
    fi
    # compare value, if not equal, modified
    if [ "x${user_value}" != "x${default0_value}" ]; then
        # Modify
        sed -i "s/${i}=[ny]$/${i}=${user_value}/" ${conf1_file}
    fi


    default2_value=$(grep $i ${conf2_file} | awk -F'=' '{print $2}')
    if [ "x${default2_value}" != "xn" ] && [ "x${default2_value}" != "xy" ]; then
        continue
    fi
    # compare value, if not equal, modified
    if [ "x${user_value}" != "x${default0_value}" ]; then
        # Modify
        sed -i "s/${i}=[ny]$/${i}=${user_value}/" ${conf2_file}
    fi

done

