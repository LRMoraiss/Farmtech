cmake_minimum_required(VERSION 3.13)

# Inicializa o SDK do Pico
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(farmtech_sim C CXX ASM)

# Define o padrão C
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Inicializa o Pico SDK
pico_sdk_init()

# Cria o executável principal
add_executable(farmtech_sim
    farmtech_sim.c
)

# Link com as bibliotecas necessárias
target_link_libraries(farmtech_sim
    pico_stdlib
    hardware_adc
    hardware_i2c
    hardware_pio
    hardware_clocks
)

# Habilita saída USB para debug
pico_enable_stdio_usb(farmtech_sim 1)
pico_enable_stdio_uart(farmtech_sim 0)

# Gera arquivos UF2 para upload
pico_add_uf2_output(farmtech_sim)
