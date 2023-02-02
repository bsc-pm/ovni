# Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

add_subdirectory(include)

add_library(parson STATIC parson.c)
target_include_directories(parson PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_property(TARGET parson PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(common STATIC common.c)
target_include_directories(common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(parson-static STATIC parson.c)
target_include_directories(parson-static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(common-static STATIC common.c)
target_include_directories(common-static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

configure_file("config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(rt)
add_subdirectory(emu)
