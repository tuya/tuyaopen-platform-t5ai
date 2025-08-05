set(OVERRIDE_COMPILE_OPTIONS
    "-mcpu=cortex-m33+nodsp"
    "-mfpu=fpv5-sp-d16"
    "-mfloat-abi=hard"
    "-mcmse"
    "-fstack-protector"
    "-ffunction-sections"
    "-fdata-sections"
)

set(OVERRIDE_LINK_OPTIONS
    "-Os"

)
