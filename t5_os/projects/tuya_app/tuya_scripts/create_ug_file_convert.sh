#! /bin/bash
#
# py2elf.sh
# Copyright (C) 2025 cc <cc@tuya>
#
# Distributed under terms of the TUYA license.
#

docker pull ubuntu:18.04

docker run -it -v $(pwd):/home/workplace ubuntu:18.04 bash -c "cat > /etc/apt/sources.list << 'EOF'
deb http://mirrors.aliyun.com/ubuntu/ bionic main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ bionic-security main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ bionic-updates main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ bionic-backports main restricted universe multiverse
EOF
apt update && apt install -y python3 python3-pip gcc zlib1g-dev python3-dev build-essential && pip3 install --upgrade pip setuptools wheel && pip3 install pyinstaller==4.5.1 && pip3 install click && cd /home/workplace && ls && pyinstaller --onefile --collect-all click --clean create_ug_file.py"
