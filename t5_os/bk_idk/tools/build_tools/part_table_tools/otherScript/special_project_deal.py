#!/usr/bin/env python3

#TODO - optimize it, should finally remove this packer

import os
import sys
import json
import shutil
import argparse
import copy
import errno
import re

def parse_sdkconfig(sdkconfig_path):
    d = list()
    f = open(sdkconfig_path, 'r')
    for l in f:
        l = ' '.join(l.split())
        line_words = l.split(' ')
        if len(line_words) < 1:
            continue
        if line_words[0] == '#':
            if (len(line_words) != 5):
                continue
            if (line_words[2] == 'is') and (line_words[3] == 'not') and (line_words[4] == 'set'):
                if line_words[1][0:7] != 'CONFIG_':
                    continue
                k = line_words[1]
                v = 'is not set'
                d.append((k,v))
        else:
            line_key_value = l.split('=')
            if len(line_key_value) != 2:
                continue
            k = line_key_value[0]
            v = line_key_value[1]
            if k[0:7] != "CONFIG_":
                continue
            d.append((k,v))
    f.close()
    return (d)

def size_format(size, include_size):
    if include_size:
        for (val, suffix) in [(0x100000, "M"), (0x400, "K")]:
            if size % val == 0:
                return "%d%s" % (size // val, suffix)
    size_str = '%x'%size
    lead_zero = '0'*(8 - len(size_str))
    return "0x%s%s" % (lead_zero, size_str)

def shrink_val(val, need_shrink):
    return int(val*32/34) if need_shrink else val

def parse_int(v):
    for letter, multiplier in [("k", 1024), ("m", 1024 * 1024)]:
        if v.lower().endswith(letter):
            return parse_int(v[:-1]) * multiplier
        return int(v, 0)


def project_general_deal(project_dir, project, target, json_src, config, cpu1_base_addr):
    need_deal = False
    start_addr = 0

    config_list = parse_sdkconfig(config)
    config_keys = [ck[0] for ck in config_list]
    dup_config_keys = set(ck for ck in config_keys if config_keys.count(ck) > 1)
    if len(dup_config_keys) != 0:
        print("Duplicate config items:\n")
        for dck in dup_config_keys:
            print("\t%s appeared times: %d\n"%(dck, config_keys.count(dck)))
        print("Please check %s to remove duplicate config items\n"%(config))
        raise

    with open(json_src, 'r') as local_json:
        config_json = json.load(local_json)
    for sec in config_json['section']:
        if sec['partition'] == 'app1':
            need_deal = True
            start_addr = shrink_val(parse_int(sec["start_addr"]), True)

    if need_deal:
        config_temp_list = list()

        depends_config_keys = ['CONFIG_SYS_CPU1_OFFSET']
        for ck,cv in config_list:
            key_searched = False
            for dck in depends_config_keys:
                special_config_key = dck
                if special_config_key == ck:
                    temp_item = '%s=y\n'%(ck)
                    key_searched = True
                    break
                else:
                    if cv == 'is not set':
                        temp_item = '# %s %s\n'%(ck, cv)
                    else:
                        temp_item = '%s=%s\n'%(ck, cv)
            if not key_searched:
                config_temp_list.append(temp_item)

        for dck in depends_config_keys:
            temp_item = '%s=y\n'%(dck)
            config_temp_list.append(temp_item)

        for index in range(len(config_temp_list)):
            if config_temp_list[index] == 'CONFIG_SYS_CPU1_OFFSET=y\n':
                config_temp_list[index] = 'CONFIG_SYS_CPU1_OFFSET=0x%x\n'%(start_addr + cpu1_base_addr)

        with open(config, 'w', encoding='utf-8') as f:
            for config_line in config_temp_list:
                f.write(config_line)
        f.close()

def project_bk7256_configa_deal(project_dir, project, target, json_src, config, cpu1_base_addr):
    if project != "bk7256_configa":
        return
    print("============>Project %s deal...\n"%(project))
    with open(json_src, 'r') as local_json:
        config_json = json.load(local_json)
    config_json['magic'] = config_json['magic']
    config_json['version'] = config_json['version']
    config_json['count'] = 2
    app1_start_addr = None
    app1_size = None
    configa_deal_sec = list()
    for sec in config_json['section']:
        if sec['partition'] == 'bootloader':
            configa_deal_sec.append(sec)
        if sec['partition'] == 'app':
            configa_deal_sec.append(sec)
        if sec['partition'] == 'app1':
            app1_start_addr = sec["start_addr"]
            app1_size = sec["size"]
    for sec in configa_deal_sec:
        sec['firmware'] = sec['firmware']
        sec['version'] = sec['version']
        sec['partition'] = sec['partition']
        sec['start_addr'] = size_format(shrink_val(parse_int(sec["start_addr"]), True), False)
        if sec['partition'] == 'app':
            sec['firmware'] = 'app_ab.bin'
            sec['partition'] = 'app'
            sec['size'] = size_format(parse_int(sec["size"]) + parse_int(app1_size), True)
            sec['size'] = size_format(shrink_val(parse_int(sec["size"]), True), True)
        else:
            sec['size'] = size_format(shrink_val(parse_int(sec["size"]), True), True)
    config_json['section'] = configa_deal_sec

    json_dest = '%s/config/%s/configurationab.json'%(project_dir, target)
    config_json = json.dumps(config_json, sort_keys=False, indent=4)
    with open(json_dest, 'w') as f:
        f.write(config_json)

    project_general_deal(project_dir, project, target, json_src, config, cpu1_base_addr)

def project_bk7256_configb_deal(project_dir, project, target, json_src, config, cpu1_base_addr):
    if project != "bk7256_configb":
        return
    print("============>Project %s deal...\n"%(project))
    with open(json_src, 'r') as local_json:
        config_json = json.load(local_json)
    config_json['magic'] = config_json['magic']
    config_json['version'] = config_json['version']
    config_json['count'] = 2
    app1_start_addr = None
    app1_size = None
    configb_deal_sec = list()
    for sec in config_json['section']:
        if sec['partition'] == 'bootloader':
            configb_deal_sec.append(sec)
        if sec['partition'] == 'app':
            configb_deal_sec.append(sec)
        if sec['partition'] == 'app1':
            app1_start_addr = sec["start_addr"]
            app1_size = sec["size"]
    for sec in configb_deal_sec:
        sec['firmware'] = sec['firmware']
        sec['version'] = sec['version']
        sec['partition'] = sec['partition']
        sec['start_addr'] = size_format(shrink_val(parse_int(sec["start_addr"]), True), False)
        if sec['partition'] == 'app':
            sec['firmware'] = 'app_ab.bin'
            sec['partition'] = 'app'
            sec['size'] = size_format(parse_int(sec["size"]) + parse_int(app1_size), True)
            sec['size'] = size_format(shrink_val(parse_int(sec["size"]), True), True)
        else:
            sec['size'] = size_format(shrink_val(parse_int(sec["size"]), True), False)
    config_json['section'] = configb_deal_sec

    json_dest = '%s/config/%s/configurationab.json'%(project_dir, target)
    config_json = json.dumps(config_json, sort_keys=False, indent=4)
    with open(json_dest, 'w') as f:
        f.write(config_json)

    project_general_deal(project_dir, project, target, json_src, config, cpu1_base_addr)

#for deal bk7236-bk7258 position independent project (generate json for making rbl firmware)
def project_config_ab_deal(project_dir, project, target, json_src, config, cpu1_base_addr):
    if project != "config_ab":
        return
    print("============>Project %s deal...\n"%(project))
    with open(json_src, 'r') as local_json:
        config_json = json.load(local_json)
    config_json['magic'] = config_json['magic']
    config_json['version'] = config_json['version']
    config_json['count'] = config_json['count']
    app1_start_addr = None
    app2_start_addr = None
    app_total_size = None
    app0_index = None
    configa_deal_sec = list()
    for tmp_index, sec in enumerate(config_json['section']):
        if sec['partition'] == 'bootloader':
            configa_deal_sec.append(sec)
        elif sec['partition'] == 'app':
            print("============>>>>>app...\n")
            configa_deal_sec.append(sec)
            app_total_size = int((parse_int(sec["size"]))/1024)
            app0_index = tmp_index
        elif sec['partition'] == 'app1':
            print("============>>>>>app1...\n")
            app_total_size += int((parse_int(sec["size"]))/1024)
        elif sec['partition'] == 'app2':
            print("============>>>>>app2...\n")
            app_total_size += int((parse_int(sec["size"]))/1024)
        else:
            raise 'error:not deal this situation!'
    app_total_size = str(app_total_size) + 'K'
    config_json['section'][app0_index]["size"] = app_total_size
    print("============>>>>>app_total_size :%s \n"%(app_total_size))

    for sec in configa_deal_sec:
        sec['firmware'] = sec['firmware']
        sec['version'] = sec['version']
        sec['partition'] = sec['partition']
        sec['start_addr'] = size_format(shrink_val(parse_int(sec["start_addr"]), True), False)
        if sec['partition'] == 'app':
            sec['firmware'] = 'app_ab.bin'
            sec['partition'] = 'app'
            sec['size'] = size_format(shrink_val(parse_int(sec["size"]), True), True)
        else:
           sec['size'] = size_format(shrink_val(parse_int(sec["size"]), True), True)
    config_json['section'] = configa_deal_sec
    config_json['count'] = 2
    json_dest = '%s/config/%s/configurationab.json'%(project_dir, target)
    config_json = json.dumps(config_json, sort_keys=False, indent=4)
    with open(json_dest, 'w') as f:
        f.write(config_json)

    project_general_deal(project_dir, project, target, json_src, config, cpu1_base_addr)
def main():
    special_project_deal_funcs = {
        "bk7256_configa": project_bk7256_configa_deal,
        "bk7256_configb": project_bk7256_configb_deal,
        "config_ab": project_config_ab_deal,
    }

    parser = argparse.ArgumentParser(description='Deal with special json files from special project')

    parser.add_argument(
        '--project-dir',
        help='Special project directory',
        default=None)

    parser.add_argument(
        '--project',
        help='Special project name',
        default=None)

    parser.add_argument(
        '--target',
        help='Special target name',
        default=None)

    parser.add_argument(
        '--json-src',
        help='Json file need deal with special project',
        default=None)

    parser.add_argument(
        '--config',
        help='Config file need deal with all project',
        default=None)

    parser.add_argument(
        '--cpu1_base_addr',
        help='The base addr add to the CONFIG_SYS_CPU1_OFFSET',
        default=None)

    args = parser.parse_args()

    project_dir = args.project_dir
    project = args.project
    target = args.target
    json_src = args.json_src
    config = args.config
    cpu1_base_addr = int(args.cpu1_base_addr, 0)

    if project in special_project_deal_funcs:
        special_project_deal_func = special_project_deal_funcs[project]
        special_project_deal_func(project_dir, project, target, json_src, config, cpu1_base_addr)
    else:
        print("============>Project %s need not to special deal..."%(project))
        project_general_deal(project_dir, project, target, json_src, config, cpu1_base_addr)

if __name__ == "__main__":
    main()