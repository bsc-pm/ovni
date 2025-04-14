# Copyright (c) 2022-2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT OMPSS2_COMPILER_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "Compiler doesn't support -fompss-2 flag, cannot enable Nanos6 RT tests")
  else()
    message(STATUS "Compiler doesn't support -fompss-2 flag, disabling Nanos6 RT tests")
  endif()
  return()
endif()

if(NOT NANOS6_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "Nanos6 not found, cannot enable Nanos6 RT tests")
  else()
    message(STATUS "Nanos6 not found, disabling Nanos6 RT tests")
  endif()
  return()
else()
  message(STATUS "Enabling Nanos6 RT tests")
endif()

function(nanos6_rt_test)
  set(switches "")
  set(single LEVEL)
  set(multi "")
  cmake_parse_arguments(
    NANOS6_TEST "${switches}" "${single}" "${multi}" ${ARGN})
  ovni_test(${ARGN})
  target_compile_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2=libnanos6")
  target_link_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2=libnanos6")
  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NANOS6_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nanos6/nanos6.toml")
  if(NANOS6_TEST_LEVEL)
    set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
      ENVIRONMENT "NANOS6_CONFIG_OVERRIDE=instrument.ovni.level=${NANOS6_TEST_LEVEL}")
  endif()
endfunction()

nanos6_rt_test(simple-task.c)
nanos6_rt_test(nested-task.c)
nanos6_rt_test(several-tasks.c)
nanos6_rt_test(sched-add.c)
nanos6_rt_test(if0.c)
#nanos6_rt_test(taskfor.c) #Taskfor no longer supported
nanos6_rt_test(taskloop.c)
nanos6_rt_test(spawn-task.c)
nanos6_rt_test(spawn-task-external.c)
nanos6_rt_test(spawn-task-external-bad.c SHOULD_FAIL
  REGEX "ovni_ev_add: thread is not initialized")

# Test multiple instrumentation levels
nanos6_rt_test(simple-task.c NAME simple-task-level-1 LEVEL 1)
nanos6_rt_test(simple-task.c NAME simple-task-level-2 LEVEL 2)
nanos6_rt_test(simple-task.c NAME simple-task-level-3 LEVEL 3)

# Same but with breakdown enabled
nanos6_rt_test(simple-task.c NAME simple-task-breakdown-level-1 LEVEL 1 BREAKDOWN)
nanos6_rt_test(simple-task.c NAME simple-task-breakdown-level-2 LEVEL 2 BREAKDOWN)
nanos6_rt_test(simple-task.c NAME simple-task-breakdown-level-3 LEVEL 3 BREAKDOWN)
