cmake_minimum_required(VERSION 3.13)

include(cmake/pico_sdk_import.cmake)

project(pico_rcm_injector C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_rcm_injector
    src/main.c
    src/rcm_injector.c
)

target_include_directories(pico_rcm_injector PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(pico_rcm_injector
    pico_stdlib
    tinyusb_host
    tinyusb_board
)

# The RP2040 USB peripheral is being used in host mode, so stdio over USB is not available.
pico_enable_stdio_usb(pico_rcm_injector 0)
pico_enable_stdio_uart(pico_rcm_injector 1)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(TARGET pico_rcm_injector POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:pico_rcm_injector> ${CMAKE_CURRENT_BINARY_DIR}/pico_rcm_injector.bin
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/tools/bin2uf2.py
            ${CMAKE_CURRENT_BINARY_DIR}/pico_rcm_injector.bin
            ${CMAKE_CURRENT_BINARY_DIR}/pico_rcm_injector.uf2
    BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/pico_rcm_injector.bin
            ${CMAKE_CURRENT_BINARY_DIR}/pico_rcm_injector.uf2
    COMMENT "Generating pico_rcm_injector.bin and pico_rcm_injector.uf2"
    VERBATIM
)
