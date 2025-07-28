#!/bin/bash


set -e
set -u

ARMINO_TARGET="build"
ARMINO_DIR=$1
PROJECT_DIR=$2
BUILD_DIR=$3
ARMINO_TOOLS_DIR=$4
BUILD_TARGET=$5



python_isabs_cmd="import os;print(os.path.isabs(\"$BUILD_DIR\"));"
if [ `python3 -c "$python_isabs_cmd"` == "False" ];then
	BUILD_DIR=${ARMINO_DIR}/${BUILD_DIR}
fi

SDK_BUILD_DIR=$(dirname ${BUILD_DIR})

ARMINO_TOOL=${ARMINO_TOOLS_DIR}/build_tools/armino
BUILD_TARGET_PREFIX=${BUILD_TARGET:0:5}

need_build_properties_lib=0
need_build_soc=0
need_clean=0
need_doc=0

if [ "${BUILD_TARGET_PREFIX}" == "libbk" ]; then
	ARMINO_SOC=${BUILD_TARGET#lib}
	need_build_properties_lib=1
	BUILD_DIR=build/properties_libs/${ARMINO_SOC}
	PROJECT_DIR=projects/properties_libs
	echo "build armino ${ARMINO_SOC} properties libs only"
	export LIB_HASH="NULL"
elif [ "${BUILD_TARGET_PREFIX}" == "relbk" ]; then
	ARMINO_SOC=${BUILD_TARGET#rel}
	BUILD_DIR=build/${BUILD_DIR}/${ARMINO_SOC}
	need_build_properties_lib=0
	need_build_soc=1
	echo "build armino ${ARMINO_SOC} only"
	export LIB_HASH="NULL"
elif [ "${BUILD_TARGET_PREFIX}" == "clean" ]; then
	ARMINO_SOC=${BUILD_TARGET#clean}
	BUILD_DIR=build/${BUILD_DIR}/${ARMINO_SOC}
	need_clean=1
elif [ "${BUILD_TARGET_PREFIX}" == "docbk" ]; then
	ARMINO_SOC=${BUILD_TARGET#doc}
	BUILD_DIR=build/${BUILD_DIR}/${ARMINO_SOC}
	echo "build armino ${ARMINO_SOC} doc only"
	need_doc=1
elif [ "${APP_VERSION}" == "verify" ]; then
	ARMINO_SOC=${BUILD_TARGET}
	BUILD_DIR=${BUILD_DIR}/${ARMINO_SOC}
	need_build_soc=1
	need_build_properties_lib=0
	compute_hash_tool=${ARMINO_TOOLS_DIR}/build_tools/compute_files_hash.py
	export LIB_HASH=$(python3 ${compute_hash_tool} ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}/libs)
else
	ARMINO_SOC=${BUILD_TARGET}
	BUILD_DIR=${BUILD_DIR}/${ARMINO_SOC}
	need_build_soc=1
	has_properties_lib_src=$(${ARMINO_TOOLS_DIR}/build_tools/detect_internal_lib_src.py)
	if [ ${has_properties_lib_src} == "1" ]; then
		echo "build armino properties lib first, then ${ARMINO_SOC}"
		need_build_properties_lib=1
		export LIB_HASH="NULL"
	else
		need_build_properties_lib=0
		compute_hash_tool=${ARMINO_TOOLS_DIR}/build_tools/compute_files_hash.py
		export LIB_HASH=$(python3 ${compute_hash_tool} ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}/libs)
	fi
fi

PROPERTIES_LIB_BUILD_DIR=${SDK_BUILD_DIR}/properties_libs/${ARMINO_SOC}
PROPERTIES_LIB_DIR=${PROPERTIES_PROJECT_DIR}

# not to build properties repeatedly when verify.
if [ -n "${BK_JENKINS_ID:-}" ]; then
	if [ -d "$PROPERTIES_LIB_BUILD_DIR" ]; then
		need_build_properties_lib=0
	fi
fi

if [ "${need_clean}" == "1" ]; then
	echo "remove ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}"
	rm -rf ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}
	echo "remove ${BUILD_DIR}"
	rm -rf ${BUILD_DIR}
	echo "remove ${PROPERTIES_LIB_BUILD_DIR}"
	rm -rf ${PROPERTIES_LIB_BUILD_DIR}
	python ${ARMINO_TOOLS_DIR}/build_tools/armino_doc.py clean ${ARMINO_SOC}
	exit 0
fi

if [ "${need_doc}" == "1" ]; then
	${ARMINO_TOOL} -B ${BUILD_DIR} -P ${PROJECT_DIR} doc ${ARMINO_SOC}
	exit 0
fi

if [ "${need_build_properties_lib}" == "1" ]; then
	echo "build properties lib for ${ARMINO_SOC}"
	rm -rf ${PROPERTIES_LIB_BUILD_DIR}/sdkconfig
	# echo "${ARMINO_TOOL} -B ${PROPERTIES_LIB_BUILD_DIR} -P ${PROPERTIES_LIB_DIR} set-target ${ARMINO_SOC}"
	${ARMINO_TOOL} -B ${PROPERTIES_LIB_BUILD_DIR} -P ${PROPERTIES_LIB_DIR} set-target ${ARMINO_SOC}
	# echo "${ARMINO_TOOL} -B ${PROPERTIES_LIB_BUILD_DIR} -P ${PROPERTIES_LIB_DIR} ${ARMINO_TARGET}"
	${ARMINO_TOOL} -B ${PROPERTIES_LIB_BUILD_DIR} -P ${PROPERTIES_LIB_DIR} ${ARMINO_TARGET}
	# echo "${ARMINO_TOOLS_DIR}/build_tools/copy_internal_libs.sh ${ARMINO_SOC} ${ARMINO_DIR} ${PROPERTIES_LIB_BUILD_DIR} ${PROJECT}"
	${ARMINO_TOOLS_DIR}/build_tools/copy_internal_libs.sh ${ARMINO_SOC} ${ARMINO_DIR} ${PROPERTIES_LIB_BUILD_DIR} ${PROJECT} ${ARMINO_TOOLS_DIR}
fi

if [ "${need_build_soc}" == "1" ]; then
	echo "build ${ARMINO_SOC}"
	rm -rf ${BUILD_DIR}/sdkconfig
	${ARMINO_TOOL} -B ${BUILD_DIR} -P ${PROJECT_DIR} set-target ${ARMINO_SOC}
	${ARMINO_TOOL} -B ${BUILD_DIR} -P ${PROJECT_DIR} ${ARMINO_TARGET}
	${ARMINO_TOOLS_DIR}/build_tools/armino_as_lib.sh ${ARMINO_SOC} ${ARMINO_DIR} ${BUILD_DIR} ${PROJECT}
fi

