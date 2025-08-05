#!/bin/bash
ENABLE_CHINESE_PYTHON_SOURCE=0
LANG=en
ARMINO_TOOLS_INSTALL_PATH=/opt
INTERACTIVE_ENABLE=1
TARGET_SOC=
CURRENT_DIR=$(dirname "$(readlink -f "$0")")
LINUX_DISTRIBUTION=
ENABLE_CHINESE_YUM_SOURCE=0

function unrecognized_linux_version(){
    echo "Unable to recognize Linux version, please install manually."
    exit 1
}

function unsuported_linux_version(){
    echo "current $1 version $2 is too low, please use version $3 or above."
    exit 1
}

function check_ubuntu_version(){
    current_version=$2
    lowest_version=$3

    current_ver=$(echo $current_version | cut -d '.' -f 1)
    lowest_ver=$(echo $lowest_version | cut -d '.' -f 1)

    if [ $current_ver -lt $lowest_ver ];then
        unsuported_linux_version $1 $2 $3
    fi
}

function check_debian_version(){
    current_ver=$2
    lowest_ver=$3
    if [ $current_ver -lt $lowest_ver ];then
        unsuported_linux_version $1 $2 $3
    fi
}

function check_centos_version(){
    current_ver=$2
    lowest_ver=$3
    if [ $current_ver -lt $lowest_ver ];then
        unsuported_linux_version $1 $2 $3
    fi
}

# check linux distribution version
function check_linux_version(){
    if [ -f /etc/os-release ];then
        . /etc/os-release
        case "$ID" in
            ubuntu)
                UBUNTU_LOWEST_VER=20.04
                check_ubuntu_version $ID $VERSION_ID $UBUNTU_LOWEST_VER
                ;;
            debian)
                DEBIAN_LOWEST_VER=11
                check_debian_version $ID $VERSION_ID $DEBIAN_LOWEST_VER
                ;;
            arch)
                # TODO
                ;;
            centos)
                CENTOS_LOWEST_VER='7'
                check_centos_version $ID $VERSION_ID $CENTOS_LOWEST_VER
                ;;
            *)
                unrecognized_linux_version
                ;;
        esac
        LINUX_DISTRIBUTION=$ID
    else
        unrecognized_linux_version
    fi
}

function switch_centos_chinese_source(){
    . /etc/os-release
    cp -r /etc/yum.repos.d /etc/yum.repos.d.bak
    rm -rf /etc/yum.repos.d/*
    if [[ $VERSION_ID == "7" ]];then
        curl -o /etc/yum.repos.d/CentOS-Base.repo https://mirrors.aliyun.com/repo/Centos-7.repo
    elif [[ $VERSION_ID == "8" ]];then
        curl -o /etc/yum.repos.d/CentOS-Base.repo https://mirrors.aliyun.com/repo/Centos-vault-8.5.2111.repo
    fi
    curl -o /etc/yum.repos.d/epel.repo https://mirrors.aliyun.com/repo/epel-7.repo
    yum makecache
}

function restore_centos_source(){
    rm -rf /etc/yum.repos.d
    mv /etc/yum.repos.d.bak /etc/yum.repos.d
}

function set_default_timezone(){
    if [ ! -f /etc/timezone ];then
        TZ=Asia/Shanghai
        ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo "$TZ" > /etc/timezone
    fi
}

function get_pip_options(){
    python_install_options=""
    if [ ${ENABLE_CHINESE_PYTHON_SOURCE} -ne 0 ];then
        python_install_options="${python_install_options} -i https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple"
    fi

    # check python's external management
    python_external_managed_path=`find /usr/lib -name EXTERNALLY-MANAGED`
    if [ ! -z $python_external_managed_path ];then
        python_install_options="${python_install_options} --break-system-packages"
    fi
    echo $python_install_options
}

function centos7_python_env_patch(){
    . /etc/os-release

    if [ "$LINUX_DISTRIBUTION" != "centos" ];then
        return
    fi

    if [ "$VERSION_ID" != "7" ];then
        return
    fi

    pip3 install "urllib3<2.0"
    ln -s /usr/local/python38/bin/sphinx-build /usr/local/bin/sphinx-build
}

function install_python_env(){
    python_install_options="$(get_pip_options)"
    pip3 install pycryptodome click future click_option_group cryptography jinja2 PyYAML cbor2 intelhex $python_install_options
    pip3 install Sphinx sphinx-rtd-theme breathe blockdiag \
         sphinxcontrib-blockdiag sphinxcontrib-seqdiag sphinxcontrib-actdiag \
         sphinxcontrib-nwdiag Pillow $python_install_options

    pip3 install "cryptography>=40" $python_install_options

    # ensure /usr/bin/python is python3
    if [ -f /usr/bin/python ];then
        python_ver=`python --version`
        python_main_ver=${python_ver:7:1}
        if [[ "$python_main_ver" -ne 3 ]];then
            rm -f /usr/bin/python
            ln -s /usr/bin/python3 /usr/bin/python
        fi
    else
        ln -s /usr/bin/python3 /usr/bin/python
    fi
    
    centos7_python_env_patch
}

function install_essential_software_ubuntu(){
    apt update
    apt install make wget git cmake python3 python3-pip ninja-build doxygen -y
}

function install_essential_software_debian(){
    apt update
    apt install make wget git cmake python3 python3-pip ninja-build doxygen -y
}

function install_essential_software_arch(){
    pacman -Syyu --noconfirm
    pacman -S --noconfirm make wget git cmake python3 python-pip python-setuptools ninja doxygen
}

function centos_install_python3(){
    . /etc/os-release
    if [ -f /usr/bin/python3 ];then
        python_ver=`python3 --version`
        python_minor_version=$(echo $python_ver | cut -d. -f2)
        if [ $python_minor_version -ge 8 ];then
            return
        else
            rm -f /usr/bin/python3
            if [ -f /usr/bin/pip3 ];then
                rm -f /usr/bin/pip3
            fi
        fi
    fi
    if [ -f /usr/bin/python3 ];then
        rm -f /usr/bin/python3
    fi
    if [ -f /usr/bin/pip3 ];then
        rm -f /usr/bin/pip3
    fi
    if [[ $VERSION_ID == "7" ]];then
        wget -O ./python38-centos7.tar.gz https://dl.bekencorp.com/d/tools/arminosdk/pythonenv/centos7/python38-centos7_v.1.0.1.tar.gz?sign=eYVNaVkjBxdt5IZk3BXQc47dWA2vDZTsK4KkyQeYqb0=:0
        tar -zxvf ./python38-centos7.tar.gz -C /usr/local
        rm -f ./python38-centos7.tar.gz
        ln -s /usr/local/python38/bin/python3 /usr/bin/python3
        ln -s /usr/local/python38/bin/pip3 /usr/bin/pip3

        yum_script_path='/usr/bin/yum'
        if ! head -n 1 $yum_script_path | grep -q python2 ;then
            sed -i '1s/python/python2/' $yum_script_path
        fi
    elif [[ $VERSION_ID == "8" ]];then
        if [ ! -f /usr/bin/python3 ];then
            ln -s /usr/bin/python3.8 /usr/bin/python3
        fi
        if [ ! -f /usr/bin/pip3 ];then
            ln -s /usr/bin/pip3.8 /usr/bin/pip3
        fi
    fi
}

function install_essential_software_centos(){
    if [ $ENABLE_CHINESE_YUM_SOURCE -ne 0 ];then
        switch_centos_chinese_source
    fi
    . /etc/os-release

    if [[ $VERSION_ID == "7" ]];then
        yum install -y make git cmake3 ninja-build bzip2 wget doxygen
    elif [[ $VERSION_ID == "8" ]];then
        yum install -y 'dnf-command(config-manager)'
        yum config-manager --set-enabled PowerTools
        yum install -y python38 make git cmake3 ninja-build bzip2 wget doxygen glibc-langpack-en
    fi
    centos_install_python3
    
    if [[ ! -f /usr/bin/cmake && -f /usr/bin/cmake3 ]];then
        ln -s /usr/bin/cmake3 /usr/bin/cmake
    fi

    if [ $ENABLE_CHINESE_YUM_SOURCE -ne 0 ];then
        restore_centos_source
    fi
}

function install_essential_software(){
    install_essential_software_$LINUX_DISTRIBUTION
}

function install_armino_common_depandency(){
    install_essential_software
    install_python_env
}

function setup_armino_build_env(){
    check_soc_valid $TARGET_SOC
    install_armino_common_depandency
    setup_soc_depandency $TARGET_SOC
}

function set_prompt_language(){
    read -p "choose Language EN or ZH, default is EN.[Y/n]:" user_input
    user_input=${user_input:-Y}
    if [[ $user_input == "N" || $user_input == "n" ]];then
        LANG=zh
    else
        LANG=en
    fi
}

function set_tools_install_path(){
    prompt_en='set tools install path[/opt]:'
    prompt_zh='设置工具安装路径[/opt]:'
    prompt=$(eval echo \${prompt_${LANG}})
    read -p "$prompt" user_input
    ARMINO_TOOLS_INSTALL_PATH=${user_input:-'/opt'}
    ARMINO_TOOLS_INSTALL_PATH=$(eval echo $ARMINO_TOOLS_INSTALL_PATH)
    ARMINO_TOOLS_INSTALL_PATH=$(realpath $ARMINO_TOOLS_INSTALL_PATH)
    echo "set armino tools install path is $ARMINO_TOOLS_INSTALL_PATH"
    if [ ! -d $ARMINO_TOOLS_INSTALL_PATH ];then
        mkdir -p $ARMINO_TOOLS_INSTALL_PATH
    fi
}

function set_pip_source(){
    prompt_en='Whether need to switch python pip to chinese source.[N/y]:'
    prompt_zh='是否需要将python切换为国内源[N/y]:'
    prompt=$(eval echo \${prompt_${LANG}})
    read -p "$prompt" user_input
    user_input=${user_input:-N}
    if [[ $user_input == "Y" || $user_input == "y" ]];then
        ENABLE_CHINESE_PYTHON_SOURCE=1
        echo "Switched python pip to chinese source."
    else
        ENABLE_CHINESE_PYTHON_SOURCE=0
    fi
}

function print_tips_info(){
    prompt_en='Tip: Pressing the ENTER key directly will use the default option.'
    prompt_zh='提示：直接按下回车键，将使用默认配置。'
    prompt=$(eval echo \${prompt_${LANG}})
    echo $prompt
}

function set_soc_target(){
    prompt_en='Please set SOC[bk7258]:'
    prompt_zh='请设置需要使用的soc[bk7258]:'
    prompt=$(eval echo \${prompt_${LANG}})
    read -p "$prompt" user_input
    TARGET_SOC=${user_input:-bk7258}
    check_soc_valid $TARGET_SOC
}

function set_centos_source(){
    if [[ ! $LINUX_DISTRIBUTION == "centos" ]];then
        return
    fi
    prompt_en='Whether need to switch centos yum to chinese source.[N/y]:'
    prompt_zh='是否需要将centos yum切换为国内源[N/y]:'
    prompt=$(eval echo \${prompt_${LANG}})
    read -p "$prompt" user_input
    user_input=${user_input:-N}
    if [[ $user_input == "Y" || $user_input == "y" ]];then
        ENABLE_CHINESE_YUM_SOURCE=1
        echo "Switched centos yum to chinese source."
    else
        ENABLE_CHINESE_YUM_SOURCE=0
    fi
}

function interactive_set_options(){
    set_prompt_language
    print_tips_info
    set_soc_target
    set_tools_install_path
    set_centos_source
    set_pip_source
}

function check_srcipt_options(){
    while getopts "c:qst" arg
    do
        case $arg in
            c)
                # set target soc.
                TARGET_SOC=$OPTARG
                check_soc_valid $TARGET_SOC
                ;;
            s)
                # chinese python source, tsinghua source.
                ENABLE_CHINESE_PYTHON_SOURCE=1
                ;;
            q)
                # set quiet mode, without interactive action.
                INTERACTIVE_ENABLE=0
                ;;
            t)
                # set default timezone, Asia/Shanghai.
                set_default_timezone
                ;;
            *)
                # TODO
                ;;
        esac
    done
}

set -e

bk7236_soc_name="bk7236"
bk7236_soc_func="install_cross_compiler_cm"

bk7258_soc_name="bk7258"
bk7258_soc_func="install_cross_compiler_cm"

armono_soc=([0]=bk7236_soc [1]=bk7258_soc)

function install_cross_compiler_cm(){
    # download and install cross-build-chain

    if [ -f $ARMINO_TOOLS_INSTALL_PATH/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc ];then
        echo "cross compiler gcc-arm-none-eabi-10.3-2021.10 already installed."
        return 0
    fi

    wget -O ./gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 https://dl.bekencorp.com/d/tools/toolchain/arm/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2?sign=RXdqFf5PRB5upxFDS7UXTUtTsStCbrQgoAfcUDJe9M8=:0
    tar -xvjf ./gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C $ARMINO_TOOLS_INSTALL_PATH
    rm -f ./gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
}

function print_valid_soc(){
    all_soc=""

    for i in "${!armono_soc[@]}"; do
        soc="${armono_soc[$i]}"
        soc_name=$(eval echo \${${soc}_name})
        all_soc+=" ${soc_name}"
    done
    echo "valid soc are:$all_soc"
}

function check_soc_valid(){
    soc_name=

    for i in "${!armono_soc[@]}"; do
        soc="${armono_soc[$i]}"
        cur_soc_name=$(eval echo \${${soc}_name})
        if [ "${cur_soc_name}" == "$1" ];then
            soc_name=${cur_soc_name}
            break
        fi
    done
    if [ -z "$soc_name" ];then
        echo "soc [$1] is invalid"
        print_valid_soc
        exit -1
    fi
}

function setup_soc_depandency(){
    check_soc_valid $1
    soc_name=
    soc_setup_func=
    for i in "${!armono_soc[@]}"; do
        soc="${armono_soc[$i]}"
        cur_soc_name=$(eval echo \${${soc}_name})
        cur_soc_setup_func=$(eval echo \${${soc}_func})

        if [ "${cur_soc_name}" == "$1" ];then
            soc_name=${cur_soc_name}
            soc_setup_func=${cur_soc_setup_func}
            break
        fi
    done
    if [ -n "$soc_setup_func" ];then
        $soc_setup_func
    fi
}

function check_software_setup(){
    if type $1 > /dev/null 2>&1;then
        echo 1
    else
        echo 0
    fi
}

function check_dependency(){
    dependecy_tools=(cmake make git ninja python3 doxygen)
    install_fail_tools=()
    for item in "${dependecy_tools[@]}"; do
        if [ $(check_software_setup $item) -ne 1 ];then
            install_fail_tools+=($item)
        fi
    done

    if [ ${#install_fail_tools[@]} -ne 0 ];then
        echo ${install_fail_tools[@]} "not installed, please check software source and retry intall software."
    else
        echo "armino build environment installed successfully."
    fi
}

function main(){
    check_linux_version
    check_srcipt_options $@
    if [ $INTERACTIVE_ENABLE -ne 0 ];then
        interactive_set_options
    fi
    setup_armino_build_env

    check_dependency
}

main $@
