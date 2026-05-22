cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(picocalc_gb C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(picocalc_gb_kaeru
    src/main.c
)

target_link_libraries(picocalc_gb_kaeru
    pico_stdlib
)

pico_add_extra_outputs(picocalc_gb_kaeru)

pico_enable_stdio_usb(picocalc_gb_kaeru 1)
pico_enable_stdio_uart(picocalc_gb_kaeru 0)

