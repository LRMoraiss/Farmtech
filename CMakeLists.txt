cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(farmtech C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(farmtech src/farmtech.c)

# Headers locais + PIO gerado (build/)
target_include_directories(farmtech PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

# PIO — Geração automática do header ws2812.pio.h
pico_generate_pio_header(farmtech ${CMAKE_CURRENT_LIST_DIR}/pio/ws2812.pio)

# Bibliotecas
target_link_libraries(farmtech
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_adc
    hardware_i2c
    hardware_pio
    hardware_pwm
    hardware_timer
    hardware_clocks
)

pico_enable_stdio_usb(farmtech 1)
pico_enable_stdio_uart(farmtech 0)
pico_add_extra_outputs(farmtech)
