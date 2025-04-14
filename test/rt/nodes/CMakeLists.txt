# Copyright (c) 2022-2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

include(CheckOmpSs2Compiler)

if(NOT OMPSS2_COMPILER_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "Compiler doesn't support -fompss-2 flag, cannot enable NODES RT tests")
  else()
    message(STATUS "Compiler doesn't support -fompss-2 flag, disabling NODES RT tests")
  endif()
  return()
endif()

if(NOT NODES_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "NODES not found, cannot enable NODES RT tests")
  else()
    message(STATUS "NODES not found, disabling NODES RT tests")
  endif()
  return()
else()
  message(STATUS "Enabling NODES RT tests")
endif()

function(nodes_rt_test)
  set(switches "")
  set(single LEVEL)
  set(multi "")
  cmake_parse_arguments(
    NODES_TEST "${switches}" "${single}" "${multi}" ${ARGN})
  ovni_test(${ARGN})

  target_compile_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2=libnodes")
  target_link_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2=libnodes")
  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NANOS6_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nodes/nanos6.toml")

  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NOSV_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nodes/nosv.toml")

  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NODES_OVNI=1")

  if(DEFINED NODES_TEST_LEVEL)
    set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
      ENVIRONMENT "NOSV_CONFIG_OVERRIDE=ovni.level=${NODES_TEST_LEVEL}")
  endif()
endfunction()

nodes_rt_test(../nanos6/simple-task.c NAME simple-task SORT)
nodes_rt_test(../nanos6/nested-task.c NAME nested-task SORT)
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks SORT)
nodes_rt_test(../nanos6/if0.c NAME if0 SORT)
nodes_rt_test(../nanos6/sched-add.c NAME sched-add SORT)
nodes_rt_test(../nanos6/taskloop.c NAME taskloop SORT)
nodes_rt_test(taskiter.c SORT)

# Test multiple instrumentation levels
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks-level-1 LEVEL 1 SORT)
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks-level-2 LEVEL 2 SORT)
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks-level-3 LEVEL 3 SORT)
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks-level-4 LEVEL 4 SORT)

# Same but with breakdown enabled
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks-breakdown-level-1 LEVEL 1 SORT BREAKDOWN
  REGEX "cannot enable breakdown for nOS-V model")
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks-breakdown-level-2 LEVEL 2 SORT BREAKDOWN
  REGEX "cannot enable breakdown for nOS-V model")
# From level 3 up the breakdown can be enabled
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks-breakdown-level-3 LEVEL 3 SORT BREAKDOWN)
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks-breakdown-level-4 LEVEL 4 SORT BREAKDOWN)

if (PERF_PARANOID_KERNEL)
  message(STATUS "Enabling perf paranoid tests for NODES")
  nodes_rt_test(fibonacci.c SORT LEVEL 3 DRIVER "fibonacci.driver.sh")
elseif(ENABLE_ALL_TESTS)
  message(FATAL_ERROR "Cannot enable perf paranoid tests for NODES")
else()
  message(STATUS "Disabling perf paranoid tests for NODES")
endif()

nodes_rt_test(fibonacci-mark.c SORT)
nodes_rt_test(heat-mark.c SORT)
