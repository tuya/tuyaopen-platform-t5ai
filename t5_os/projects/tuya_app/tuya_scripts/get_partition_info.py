#! /usr/bin/env python3
# vim:fenc=utf-8
#
# Copyright © 2025 cc <cc@tuya>
#
# Distributed under terms of the TUYA license.

import csv
import sys

def parse_size(size_str):
    s = size_str.strip().upper()
    if s.endswith('K'):
        return int(s[:-1]) * 1024
    elif s.endswith('M'):
        return int(s[:-1]) * 1024 * 1024
    else:
        return int(s)

def main():
    if len(sys.argv) != 4:
        print("0")
        sys.exit(1)

    file_path = sys.argv[1]
    target_partition = sys.argv[2]
    info_type = sys.argv[3]

    partitions = []
    current_offset = 0

    with open(file_path, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            if not row:
                continue
            if row[0].strip().startswith('#'):
                continue
            if len(row) < 4:
                continue
            name = row[0].strip()
            size_str = row[2].strip()
            try:
                size_bytes = parse_size(size_str)
            except ValueError:
                print("0")
                sys.exit(1)
            partitions.append({
                'name': name,
                'size_str': size_str,
                'offset': current_offset
            })
            current_offset += size_bytes

    for part in partitions:
        if part['name'] == target_partition:
            if info_type == 'address':
                print(part['offset'])
                sys.exit(0)
            elif info_type == 'size':
                print(parse_size(part['size_str']))
                sys.exit(0)
            else:
                print("0")
                sys.exit(1)

    print("0")
    sys.exit(1)

if __name__ == '__main__':
    main()


