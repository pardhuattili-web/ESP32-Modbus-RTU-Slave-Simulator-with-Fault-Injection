cmake_minimum_required(VERSION 3.16)

option(BUILD_HOST_TESTS "Build protocol core unit tests on the host" OFF)

if(BUILD_HOST_TESTS)
    project(esp32_modbus_host_tests C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
    enable_testing()

    add_executable(test_modbus
        main/modbus_crc.c
        main/modbus_rtu.c
        main/register_map.c
        tests/test_modbus.c
    )
    target_include_directories(test_modbus PRIVATE main)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_modbus PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME modbus_core COMMAND test_modbus)
    return()
endif()

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "IDF_PATH is not set. Activate the ESP-IDF environment first.")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_modbus_rtu_fault_simulator)
