cmake_minimum_required(VERSION 2.8)

project(BURNITNOW)

set(CMAKE_MODULE_PATH "${BURNITNOW_SOURCE_DIR}/CMakeModules")

include(UseHaiku)

add_subdirectory(Source)
