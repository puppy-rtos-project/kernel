
option("build_target")
    set_default("stm32-f407-fkm1")
    set_values("stm32-f407-fkm1", "stm32-f412-nucleo", "raspberry-pico")
    set_description("Build target")

option("build_toolchian")
    set_default("arm-none-eabi-gcc")
    set_values("arm-none-eabi-gcc", "armclang")
    set_description("Build Toolchian")

option("nr_micro_shell")
    set_default(true)
    set_category("SubSystem")
    set_description("Enable or disable nr_micro_shell")

option("tlsf")
    set_default(true)
    set_category("SubSystem")
    set_description("Enable or disable tlsf")

option("p_stdlib_h")
    set_default(true)
    set_category("SubSystem")
    set_description("Enable or disable stdlib.h(malloc,free...)")

option("kernel_test")
        set_default(true)
        set_category("Tests")
        set_description("Enable or disable kernrl-tests")
option_end()
