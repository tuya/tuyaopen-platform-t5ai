set(OVERRIDE_COMPILE_OPTIONS 
    "-mcpu=cortex-m33+nodsp"
    "-mfpu=fpv5-sp-d16"
    "-mfloat-abi=hard"
    "-mcmse"
    "-fstack-protector"
)

set(OVERRIDE_LINK_OPTIONS
    "-fno-builtin-printf"
    "-Os"
    "--specs=nano.specs"
)
