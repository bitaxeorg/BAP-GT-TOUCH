# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_compile_options("-Wno-attributes")

# Set custom partition table
set(PARTITION_TABLE_CSV_PATH ${CMAKE_CURRENT_SOURCE_DIR}/partitions.csv)

# Default version (overridden by CI with -DPROJECT_VER)
if(NOT DEFINED PROJECT_VER)
    set(PROJECT_VER "v1.0.1")
endif()

project(lvgl_porting)
