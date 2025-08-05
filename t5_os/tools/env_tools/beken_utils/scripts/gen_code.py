#!/usr/bin/env python3

from .gen_mpc import *
from .gen_ota import *
from .gen_otp_map import *
from .gen_partition import *
from .gen_ppc import *
from .gen_security import *
from .partition import *


def gen_code():
    gen_ppc_config_file("ppc.csv", "gpio_dev.csv", "_ppc.h")
    gen_mpc_config_file("mpc.csv", "_mpc.h")
    gen_security_config_file("security.csv", "security.h")
    gen_ota_config_file("ota.csv", "_ota.h")
    gen_otp_map_file()

    OTA("ota.csv")
