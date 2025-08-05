#!/usr/bin/env python3
import os
import logging
import csv

from .parse_csv import *

otp_keys = [
    'id'
    'name'
    'size'
    'offset'
    'end'
    'privilege'
    'security'
]

def str_to_security(s):
    s = s.upper()
    if s == "TRUE":
        return "OTP_SECURITY"
    elif s == "FALSE":
        return "OTP_NON_SECURITY"
    else:
        logging.error("otp map security error.")
class OTP:
    is_write_title = False

    def __init__(self, csv_file, number):
        self.data = self.init_from_csv(csv_file)
        self.index = number

    def init_from_csv(self, csv_file):
        data = {}
        with open(csv_file,newline='') as csvfile:
            reader = csv.DictReader(csvfile, skipinitialspace=True)
            end = 0
            for row in reader:
                item_id = int(row['id'])
                size = int(row['size'])
                offset = int(row['offset'], 16)
                if(offset < end):
                    logging.error(f"{csv_file} offset error,id = {item_id}.")
                    exit(0)
                end = int(row['end'], 16)
                if(size != (end - offset)):
                    logging.error(f"{csv_file} size error,id = {item_id}.")
                    exit(0)
                data[item_id] = {
                    'name':row['name'],
                    'size':size,
                    'offset':offset,
                    'privilege':row['privilege'],
                    'security':str_to_security(row['security'])
                }
        return data

