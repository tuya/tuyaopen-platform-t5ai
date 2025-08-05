#!/bin/bash

init_bk_libs_dir()
{
	mkdir -p ${s_bk_libs_dir}/${s_soc}
	mkdir -p ${s_bk_libs_dir}/${s_soc}/libs
	mkdir -p ${s_bk_libs_dir}/${s_soc}/config
	mkdir -p ${s_bk_libs_dir}/${s_soc}/hash
	mkdir -p ${s_bk_libs_dir}/${s_soc}/bootloader/ab_bootloader
	mkdir -p ${s_bk_libs_dir}/${s_soc}/bootloader/normal_bootloader
	rm -rf ${s_bk_libs_dir}/${s_soc}/libs/*
	rm -rf ${s_bk_libs_dir}/${s_soc}/config/*
	rm -rf ${s_bk_libs_dir}/${s_soc}/config/*
	rm -rf ${s_bk_libs_dir}/${s_soc}/bootloader/ab_bootloader/*
	rm -rf ${s_bk_libs_dir}/${s_soc}/bootloader/normal_bootloader/*
}

copy_libs()
{
	echo "Copy armino properties libs ${s_armino_dir}/components/bk_libs"

	for dir in ${s_armino_build_dir}/armino/*
	do
		if [ -d ${dir} ] ; then
			for file in ${dir}/*.a
			do
				if [ -f ${file} ]; then
						#echo "cp -rf ${file} ${s_bk_libs_dir}/${s_soc}/libs/"
						cp -rf ${file} ${s_bk_libs_dir}/${s_soc}/libs/
				fi
			done
		fi
	done
	rm -rf ${s_bk_libs_dir}/${s_soc}/libs/libmain.a
	#find ${s_bk_libs_dir}/${s_soc}/libs -name *.a | xargs md5sum > ${s_bk_libs_dir}/${s_soc}/hash/libs_hash.txt
	compute_hash_tool=${s_armino_tools_dir}/build_tools/compute_files_hash.py
	compute_hash_src_dir=${s_bk_libs_dir}/${s_soc}/libs
	compute_hash_dest_file=${s_bk_libs_dir}/${s_soc}/hash/libs_hash.txt
	python3 ${compute_hash_tool} ${compute_hash_src_dir} > ${compute_hash_dest_file}
	echo "Saved manifest to ${s_bk_libs_dir}/${s_soc}/hash/libs_version_manifest.xml"
	# repo manifest -r -o ${s_bk_libs_dir}/${s_soc}/hash/libs_version_manifest.xml > /dev/null
}

copy_bootloader()
{
	normal_bl_build_path=${s_armino_dir}/properties/modules/bootloader/aboot/arm_bootloader/output/bootloader.bin
	normal_bl_archive_path=${s_bk_libs_dir}/${s_soc}/bootloader/normal_bootloader/bootloader.bin
	ab_bl_build_path=${s_armino_dir}/properties/modules/bootloader/aboot/arm_bootloader_ab/output/bootloader.bin
	ab_bl_archive_path=${s_bk_libs_dir}/${s_soc}/bootloader/ab_bootloader/bootloader.bin
	cp ${normal_bl_build_path} ${normal_bl_archive_path}
	cp ${ab_bl_build_path} ${ab_bl_archive_path}
}

copy_sdkconfig()
{
	cp ${s_armino_build_dir}/sdkconfig ${s_bk_libs_dir}/${s_soc}/config/
	cp ${s_armino_build_dir}/config/sdkconfig.h ${s_bk_libs_dir}/${s_soc}/config
}

s_soc=$1
s_armino_dir=$2
s_armino_build_dir=$3
s_project=$4
s_armino_tools_dir=$5
s_bk_libs_dir="${s_armino_dir}/components/bk_libs"

init_bk_libs_dir
copy_libs
copy_bootloader
copy_sdkconfig 
