
import struct
import argparse
import os

sections = []
map_t_format = 'IIII'
section_hdr_format = 'IIII'
map_t_size = struct.calcsize(map_t_format)
section_hdr_len = struct.calcsize(section_hdr_format)
section_magic_word = 0xA5A5AA55

class EarlyExitException(Exception):
    pass

def parse_map_t_from_bin_file(file_path, start_position):
    with open(file_path, 'rb') as f:
        f.seek(start_position)
        while True:
            # read data of map_t struct
            data = f.read(map_t_size)
            if len(data) < map_t_size:
                break
            
            # parse data
            id, start, end, length = struct.unpack(map_t_format, data)
            
            # chekc id if is 0xFFFFFFFF
            if id == 0xFFFFFFFF:
                break
            
            sections.append({
                'id': id,
                'start': start,
                'end': end,
                'length': length
            })
    
    return sections

def save_partial_bin(source_file, target_file, offset, target_len):
    dest_len = 0
    try:
        with open(source_file, 'rb') as src:
            src.seek(offset)
            data = src.read(target_len)

        with open(target_file, 'wb') as dest:
            dest.write(data)
        
        dest_len = target_len
        print(f"save {dest_len} bytes from {source_file} to {target_file}")
    except FileNotFoundError:
        print("File Not Found, Please check your file paths")

    return dest_len

def bin_file_to_text_file(bin_file, txt_file, bytes_one_line):
    with open(bin_file, 'rb') as src, open(txt_file, 'w') as dest:
        while True:
            data = src.read(bytes_one_line)
            if len(data) < bytes_one_line:
                break
            if 4 == bytes_one_line:
                (number,) = struct.unpack('I', data)
                dest.write(f'{hex(number)}\n')
            else:
                hex_string = ''.join(f'{byte:02x}' for byte in data)
                dest.write(hex_string + '\n')
    pass

def get_section_hdr(source_file, offset):
    section_hdr = []

    with open(source_file, 'rb') as file_handle:
        file_handle.seek(offset)
        content = file_handle.read(section_hdr_len)
        if len(content) < section_hdr_len:
            raise EarlyExitException("condition not matched")
        
        # parse data
        magic, start, end, length = struct.unpack(section_hdr_format, content)
        if (section_magic_word == magic):
            section_hdr.append(start)
            section_hdr.append(end)
            section_hdr.append(length)
    return section_hdr

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Parse map_t structures from a binary file.')
    parser.add_argument('file', help='The path to the binary file.')
    parser.add_argument('position', type=lambda x: int(x, 0), help='The start position in the file (e.g., 0x253000).')
    args = parser.parse_args()

    source_bin_access_offset = args.position
    sections = parse_map_t_from_bin_file(args.file, source_bin_access_offset)
    for section in sections:
        print(section)

    source_bin_access_offset += (len(sections) + 1) * map_t_size
    save_partial_bin(args.file, "cpu_context.bin", source_bin_access_offset, sections[0]['length']);
    bin_file_to_text_file('cpu_context.bin', 'cpu_context.txt', 4)

    source_bin_access_offset += sections[0]['length']
    for section in sections[1:]:
        hdr = get_section_hdr(args.file, source_bin_access_offset)
        source_bin_access_offset += section_hdr_len

        print(hdr)
        section_file = f"{hex(hdr[0])}_{hex(hdr[1])}_{hex(hdr[2])}.bin"
        save_partial_bin(args.file, section_file, source_bin_access_offset, hdr[2])
        source_bin_access_offset += hdr[2]

