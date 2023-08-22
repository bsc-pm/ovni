# Copyright (c) 2022-2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

function(test_emu)
  ovni_test(${ARGN})
  target_link_libraries("${OVNI_TEST_NAME}"
    PRIVATE instr-static)
endfunction()

add_subdirectory(common)
add_subdirectory(ovni)
add_subdirectory(nosv)
add_subdirectory(nanos6)
add_subdirectory(tampi)
