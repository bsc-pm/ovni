# Copyright (c) 2022-2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later 

test_emu(flush-overhead.c DISABLED)
test_emu(flush.c)
test_emu(sort.c SORT)
test_emu(sort-flush.c SORT)
test_emu(empty-sort.c SORT)
test_emu(sort-first-and-full-ring.c SORT
  SHOULD_FAIL REGEX "cannot find a event previous to clock")
test_emu(burst-stats.c REGEX "burst stats: median/avg/max =  33/ 33/ 33 ns")
test_emu(mp-simple.c MP)
test_emu(version-good.c)
test_emu(version-bad.c SHOULD_FAIL REGEX "incompatible .* version")
test_emu(clockgate.c MP SHOULD_FAIL REGEX "detected large clock gate")
test_emu(no-cpus.c SHOULD_FAIL REGEX "loom .* has no physical CPUs")
test_emu(sort-cpus-by-loom.c MP)
test_emu(sort-cpus-by-rank.c MP)
