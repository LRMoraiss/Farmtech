cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(tarefa1_unidade4 C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

# Questões 1 e 2
add_executable(q1_q2 q1_q2_timer_botoes.c)
target_link_libraries(q1_q2 pico_stdlib hardware_timer)
pico_enable_stdio_usb(q1_q2 1)
pico_add_extra_outputs(q1_q2)

# Questão 3
add_executable(q3 q3_pwm_rgb.c)
target_link_libraries(q3 pico_stdlib hardware_pwm hardware_timer)
pico_enable_stdio_usb(q3 1)
pico_add_extra_outputs(q3)

# Questão 4
add_executable(q4 q4_adc_temperatura.c)
target_link_libraries(q4 pico_stdlib hardware_adc)
pico_enable_stdio_usb(q4 1)
pico_add_extra_outputs(q4)
