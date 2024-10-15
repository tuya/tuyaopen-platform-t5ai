#!/usr/bin/env python3

import logging
import subprocess
import os
import shutil

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if (ret):
        logging.error(f'failed to run "{cmd}"')
        exit(1)

def build_server_process(project):
    run_cmd(f'make clean')
    #run_cmd(f'make bk7236 PROJECT={project} APP_VERSION="vvvvvvvvvvvvvvvvvvvvvvvv1.0"')
    run_cmd(f'make bk7236 PROJECT={project} APP_VERSION="v1.0"')
