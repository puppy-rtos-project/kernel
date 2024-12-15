
target("puppy")

if is_config("build_board", "qemu-virt-riscv") then
    add_defines(
        "__riscv_float_abi_soft",
        "_POSIX_C_SOURCE=199309L",
        "PUP_CPU_NR=1"
    )
    add_files(
        "**.c",
        "**.S"
    )
    add_includedirs(
        "drivers",
        "libraries"
    )
    set_toolchains("cross")
    set_extension(".elf")
    set_arch("riscv32")
    add_cxflags(' -march=rv32imac -mabi=ilp32 -mcmodel=medany -Wall -fmessage-length=0 -ffunction-sections -fdata-sections -fno-builtin-printf')
    add_asflags(' -c -march=rv32imac -mabi=ilp32 -mcmodel=medany -Wall -fmessage-length=0 -ffunction-sections -fdata-sections -fno-builtin-printf')
    add_ldflags(' -march=rv32imac -mabi=ilp32 -mcmodel=medany -nostartfiles -Xlinker --gc-sections -Xlinker --defsym=__stack_size=1024 -Xlinker -Map=puppy.map')
    add_ldflags(' -T ' .. os.scriptdir() .. '/link.lds')
    after_build(function (package)
        if is_mode('debug') then
            os.exec("$(sdk)/bin/riscv64-unknown-elf-objcopy -O binary $(buildir)/cross/riscv32/debug/puppy.elf puppy.bin")
        else
            os.exec("$(sdk)/bin/riscv64-unknown-elf-objcopy -O binary $(buildir)/cross/riscv32/release/puppy.elf puppy.bin")
        end
    end)
end
