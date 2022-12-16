# Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

include_directories("${CMAKE_SOURCE_DIR}/src/include")

add_library(ovni SHARED ovni.c)
target_link_libraries(ovni parson)
target_include_directories(ovni PUBLIC "${CMAKE_SOURCE_DIR}/include")
set_target_properties(ovni PROPERTIES
  VERSION 1.0.0
  SOVERSION 1)

add_library(ovni-static STATIC ovni.c)
target_link_libraries(ovni-static parson)
target_include_directories(ovni-static PUBLIC "${CMAKE_SOURCE_DIR}/include")

install(TARGETS ovni LIBRARY DESTINATION lib)
