# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(growver2020)

set(EXTRA_COMPONENT_DIRS "components/other")
set(EXTRA_COMPONENT_DIRS "components/motor")
set(EXTRA_COMPONENT_DIRS "components/ota")
set(EXTRA_COMPONENT_DIRS "components/prov")
set(EXTRA_COMPONENT_DIRS "components/ws2812")

spiffs_create_partition_image(storage main/WebFiles/fs FLASH_IN_PROJECT)
