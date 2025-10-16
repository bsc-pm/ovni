# Copyright (c) 2021-2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT NOSV_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "nOS-V not found, cannot enable nOS-V RT tests")
  else()
    message(STATUS "nOS-V not found, disabling nOS-V RT tests")
  endif()
  return()
endif()

# API breakage
if("${NOSV_VERSION}" VERSION_LESS "4.0.0")
  message(FATAL_ERROR "nOS-V version ${NOSV_VERSION} too old, required at least 4.0.0")
endif()

message(STATUS "Enabling nOS-V RT tests")

function(nosv_test)
  set(switches "")
  set(single LEVEL)
  set(multi "")
  cmake_parse_arguments(
    NOSV_TEST "${switches}" "${single}" "${multi}" ${ARGN})
  ovni_test(${ARGN})
  target_link_libraries("${OVNI_TEST_NAME}" PRIVATE PkgConfig::NOSV m)
  set_property(TEST "${OVNI_TEST_NAME}" APPEND
    PROPERTY
      ENVIRONMENT "NOSV_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nosv/nosv.toml")
  if(DEFINED NOSV_TEST_LEVEL)
    set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
      ENVIRONMENT "NOSV_CONFIG_OVERRIDE=ovni.level=${NOSV_TEST_LEVEL}")
  endif()
endfunction()

nosv_test(attach.c SORT)
nosv_test(attach-instr.c SORT)
nosv_test(nosv-pthread.c SORT)
nosv_test(waitfor.c SORT)
nosv_test(several-tasks.c SORT)
nosv_test(init-nested.c SORT)
nosv_test(parallel-tasks.c SORT)
nosv_test(inline.c SORT)
nosv_test(mutex.c SORT LEVEL 3)
nosv_test(barrier.c SORT LEVEL 3)
nosv_test(cond.c SORT LEVEL 3)
nosv_test(yield-noisy.c NAME "yield-noisy-l3" DRIVER "yield-noisy-l3.driver.sh")
nosv_test(yield-noisy.c NAME "yield-noisy-l4" DRIVER "yield-noisy-l4.driver.sh")
nosv_test(resubmit-race.c SORT)

# Test multiple instrumentation levels
nosv_test(several-tasks.c SORT NAME several-tasks-level-0 LEVEL 0)
nosv_test(several-tasks.c SORT NAME several-tasks-level-1 LEVEL 1)
nosv_test(several-tasks.c SORT NAME several-tasks-level-2 LEVEL 2)
nosv_test(several-tasks.c SORT NAME several-tasks-level-3 LEVEL 3)
nosv_test(several-tasks.c SORT NAME several-tasks-level-4 LEVEL 4)

# Same but with breakdown enabled
nosv_test(several-tasks.c SORT NAME several-tasks-breakdown-level-0 LEVEL 0 BREAKDOWN
  REGEX "cannot enable breakdown for nOS-V model")
nosv_test(several-tasks.c SORT NAME several-tasks-breakdown-level-1 LEVEL 1 BREAKDOWN
  REGEX "cannot enable breakdown for nOS-V model")
nosv_test(several-tasks.c SORT NAME several-tasks-breakdown-level-2 LEVEL 2 BREAKDOWN
  REGEX "cannot enable breakdown for nOS-V model")
# From level 3 up the breakdown can be enabled
nosv_test(several-tasks.c SORT NAME several-tasks-breakdown-level-3 LEVEL 3 BREAKDOWN)
nosv_test(several-tasks.c SORT NAME several-tasks-breakdown-level-4 LEVEL 4 BREAKDOWN)

nosv_test(several-tasks.c NAME hwc-tasks DRIVER "hwc.driver.sh")
nosv_test(hwc-stride.c DRIVER "hwc-stride.driver.sh")

if (PERF_PARANOID_KERNEL)
  message(STATUS "Enabling perf paranoid tests for nOS-V")
  nosv_test(kernel.c NAME kernel-overflow DRIVER "kernel-overflow.driver.sh")
  nosv_test(kernel.c NAME kernel-normal   DRIVER "kernel-normal.driver.sh")
elseif(ENABLE_ALL_TESTS)
  message(FATAL_ERROR "Cannot enable perf paranoid tests for nOS-V")
else()
  message(STATUS "Disabling perf paranoid tests for nOS-V")
endif()
