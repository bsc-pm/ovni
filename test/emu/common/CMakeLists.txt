# Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(instr-static STATIC instr.c)
target_include_directories(instr-static
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
  PRIVATE "${CMAKE_SOURCE_DIR}/src"
  PRIVATE "${CMAKE_SOURCE_DIR}/src/include"
  PRIVATE "${CMAKE_SOURCE_DIR}/include"
)
target_link_libraries(instr-static ovni-static parson-static common-static)
