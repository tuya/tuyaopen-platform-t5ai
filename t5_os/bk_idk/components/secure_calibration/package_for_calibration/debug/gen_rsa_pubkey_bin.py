#!/usr/bin/env python3
import sys
import os
import struct
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization

#read_public_key from .pem
def read_public_key(file):
    with open(file,'rb') as pem_file:
        public_key = serialization.load_pem_public_key(
            pem_file.read(),
            backend=default_backend()
        )

    der_data = public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    )

    with open ('rsa_pubkey_gen.bin','wb+') as src:
        size = os.path.getsize(file)        #get the length of file
        size = hex(size).replace('0x', '')  #translate to hex format is str format
        print(f'size = {size} size ={type(size)}')
        size = int(size,16)
        size_hex = struct.pack('I', size)
        src.write(size_hex)
        src.seek(4)
        src.write(der_data)

if __name__ == '__main__':
    read_public_key(sys.argv[1])