# Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later 

# Only run performance sensitive tests on Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  ovni_test(flush-overhead.c)
endif()

ovni_test(flush.c)
ovni_test(sort.c SORT)
ovni_test(empty-sort.c SORT)
ovni_test(sort-first-and-full-ring.c SORT
  SHOULD_FAIL REGEX "cannot find a event previous to clock")
ovni_test(burst-stats.c REGEX "burst stats: median 33 ns, avg 33\.0 ns, max 33 ns")
ovni_test(mp-simple.c MP)
ovni_test(version-good.c)
ovni_test(version-bad.c SHOULD_FAIL REGEX "version mismatch")
ovni_test(clockgate.c MP SHOULD_FAIL REGEX "detected large clock gate")
