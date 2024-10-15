#!/bin/sh

TARGET_PLATFORM=$1
APP_NAME=$2
APP_VERSION=$3
echo TARGET_PLATFORM=$TARGET_PLATFORM
echo APP_NAME=$APP_NAME
echo APP_VERSION=$APP_VERSION

fatal() {
    echo -e "\033[0;31merror: $1\033[0m"
    exit 1
}

[ -z $TARGET_PLATFORM ] && fatal "please set target platform, like bk7256!"
[ -z $APP_NAME ] && fatal "please set project name, like avdk!"
[ -z $APP_VERSION ] && fatal "please set app version!"

cd `dirname $0`

export ROOT_DIR=$(pwd)

export BUILD_PATH=${ROOT_DIR}
export APPS_BUILD_PATH=${BUILD_PATH}/
export APPS_BUILD_CMD=${BUILD_PATH}/build.sh


if [ -f $APPS_BUILD_CMD ]; then
    cd $APPS_BUILD_PATH
    sh $APPS_BUILD_CMD $APP_NAME $APP_VERSION $TARGET_PLATFORM
else
    echo "No Build Command!"
    exit 1
fi

