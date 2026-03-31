cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(farmtech_final C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(farmtech_final
    farmtech_core.c
)

# Habilita suporte a Wi-Fi (CYW43439)
target_link_libraries(farmtech_final
    pico_stdlib
    hardware_adc
    hardware_pwm
    hardware_i2c
    hardware_timer
    pico_cyw43_arch_none
)

pico_enable_stdio_usb(farmtech_final 1)
pico_enable_stdio_uart(farmtech_final 1)

pico_add_extra_outputs(farmtech_final)
