# Copyright (c) 2021-2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

set(OVNI_TEST_SOURCE_DIR "${CMAKE_SOURCE_DIR}/test")
set(OVNI_TEST_BUILD_DIR "${CMAKE_BINARY_DIR}/test")

include(macros.cmake)

add_subdirectory(unit)
add_subdirectory(emu)
add_subdirectory(rt)
add_subdirectory(bench)
