cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(my_blink C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Include directories
include_directories(include/directory)
include_directories(include/FAT)
include_directories(include/filesystem)
include_directories(include/flash)

add_executable(my_blink
    src/main.c
    src/flash/flash_ops.c
    src/filesystem/filesystem_helper.c
    src/filesystem/filesystem.c
    src/FAT/fat_fs.c
    src/directory/directories.c
    src/directory/directory_helpers.c
    
)
pico_enable_stdio_usb(my_blink 1)
pico_enable_stdio_uart(my_blink 0)

pico_add_extra_outputs(my_blink)

target_link_libraries(my_blink pico_stdlib )
