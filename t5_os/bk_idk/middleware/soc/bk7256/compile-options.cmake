set(OVERRIDE_COMPILE_OPTIONS
    "-mstrict-align"
    "-fstack-protector"
    "-matomic"
	"-mno-riscv-iprintf"
)

set(OVERRIDE_LINK_OPTIONS
    "-Wl,--defsym,memcpy=memcpy_ss"
    "-fno-builtin-printf"
    "-Os"
    "-Wl,-wrap,iprintf"
    "-Wl,-wrap,viprintf"
    "-Wl,-wrap,vprintf"
)
