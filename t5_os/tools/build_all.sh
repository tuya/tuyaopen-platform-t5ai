#!/bin/sh

BUILD_VERSION=$1

rm -rf ./build/*

./build_avdk.sh bk7256 avdk 1.0.0
