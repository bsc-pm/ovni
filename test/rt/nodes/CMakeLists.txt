# Copyright (c) 2022-2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Nodes)
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
  ovni_test(${ARGN})

  target_compile_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2=libnodes")
  target_link_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2=libnodes")
  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NANOS6_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nodes/nanos6.toml")

  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NOSV_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nodes/nosv.toml")

  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NODES_OVNI=1")
endfunction()

nodes_rt_test(../nanos6/simple-task.c NAME simple-task SORT)
nodes_rt_test(../nanos6/nested-task.c NAME nested-task SORT)
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks SORT)
nodes_rt_test(../nanos6/if0.c NAME if0 SORT)
nodes_rt_test(../nanos6/sched-add.c NAME sched-add SORT)
nodes_rt_test(../nanos6/taskloop.c NAME taskloop SORT)
nodes_rt_test(taskiter.c SORT)
