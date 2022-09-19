# Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later 

# Only run performance sensitive tests on Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  ovni_test(flush-overhead.c)
endif()

ovni_test(flush.c)
ovni_test(mp-simple.c MP)
ovni_test(mp-rank.c MP)
