#! /usr/bin/env python3
# vim:fenc=utf-8
#
# Copyright © 2025 cc <cc@tuya>
#
# Distributed under terms of the MIT license.

"""

"""

import csv
import sys
import re

def parse_size(size_str):
    size_str = size_str.strip()
    if not size_str:
        return 0
    match = re.match(r'^(\d+)([kK])?$', size_str)
    if not match:
        raise ValueError(f"invalid size: {size_str}")
    num, unit = match.groups()
    return int(num) * 1024 if unit else int(num)

def main():
    if len(sys.argv) < 4:
        print("fail: argv error")
        sys.exit(1)

    csv_file = sys.argv[1]
    actual_app_size_str = sys.argv[2]
    actual_app1_size_str = sys.argv[3]

    try:
        actual_app_size = parse_size(actual_app_size_str)
    except ValueError as e:
        print(f"fail: can't parse primary_cp_app size: {e}")
        sys.exit(1)

    try:
        actual_app1_size = parse_size(actual_app1_size_str)
    except ValueError as e:
        print(f"fail: can't parse primary_ap_app size: {e}")
        sys.exit(1)

    partitions = []
    with open(csv_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            is_commented = line.startswith('#')
            if is_commented:
                line = line[1:].strip()

            reader = csv.reader([line])
            try:
                row = next(reader)
            except csv.Error:
                continue

            if len(row) < 4:
                continue

            name = row[0].strip()
            size_str = row[2].strip()
            if not name or not size_str:
                continue

            try:
                size_bytes = parse_size(size_str)
            except ValueError:
                continue

            partitions.append({
                'name': name,
                'size_bytes': size_bytes,
                'size_str': size_str
            })

    app_partition = next((p for p in partitions if p['name'] == 'primary_cp_app'), None)
    app1_partition = next((p for p in partitions if p['name'] == 'primary_ap_app'), None)
    if not app_partition or not app1_partition:
        print("fail: CSV does not include 'primary_cp_app' or 'primary_ap_app'")
        sys.exit(1)

    max_original_app = (app_partition['size_bytes'] * 32) // 34
    max_original_app1 = (app1_partition['size_bytes'] * 32) // 34

    error_messages = []
    if actual_app_size > max_original_app:
        error_messages.append(f"primary_cp_app actual size {actual_app_size_str}/{actual_app_size//1024}K, limited {max_original_app//1024}K")
    if actual_app1_size > max_original_app1:
        error_messages.append(f"primary_ap_app actual size {actual_app1_size_str}/{actual_app1_size//1024}K, limited {max_original_app1//1024}K")

    if error_messages:
        print("fail: " + "; ".join(error_messages))
        sys.exit(1)

    target_names = ['primary_cp_app', 'primary_ap_app', 'ota']
    total_storage = 0
    for p in partitions:
        if p['name'] in target_names:
            total_storage += p['size_bytes']

    print(f"success:{total_storage:x}:{max_original_app}")

if __name__ == '__main__':
    main()
