How to build different bootrom bin
============================================

Build BootROM for FPGA Verification
------------------------------------------------------

Enable FPGA and run "./b1.sh fpga [deploy dir]" to build FPGA version.

Build BootROM for MPW1
------------------------------------------------------

Disable FPGA and run "./b1.sh mpw1 [deploy dir]" to build MPW1 version.

Build BootROM for MPW2
------------------------------------------------------

Disable FPGA and run "./b1.sh mpw2 [deploy dir]" to build MPW2 version.

If you want to enable buf dump, you can enable DUMP_BUF and disable LEGACY_BOOT options.

