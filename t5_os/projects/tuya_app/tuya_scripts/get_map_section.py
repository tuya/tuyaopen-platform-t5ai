#!/usr/bin/env python

import sys
import re

# Define the function to extract address from a line
def extract_address(line):
    # Split the line by whitespace, assuming the first part is the address
    parts = line.split()
    if len(parts) > 0:
        # The address is assumed to be the first part
        address = parts[0]
        # Convert the hexadecimal string to an integer
        return int(address, 16)
    else:
        raise ValueError("No address found in line")
    
def find_address_in_map_file(file_path, search_string):
    address = None
    with open(file_path, 'r') as file:
        for line in file:
            if search_string in line:
                try:
                    address = extract_address(line)
                    break
                except ValueError as e:
                    print(f"Error processing line: {e}")
    return address

def do_main(file_path, search_string):
    address = find_address_in_map_file(file_path, search_string)
    if address == None:
        print("No address found in the file.")
        sys.exit(1)

    return address

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python get_map_section.py <file_path> <search_string>")
        sys.exit(1)
    else:
        file_path = sys.argv[1]
        search_string = sys.argv[2]

        result = do_main(file_path, search_string)
        if result != None:
            print(f"{result}")