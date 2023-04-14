# Copyright (c) 2022-2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Nodes)

include(CheckCCompilerFlag)
check_c_compiler_flag("-fompss-2" HAVE_FOMPSS2_FLAG)
if(NOT HAVE_FOMPSS2_FLAG)
  message(STATUS "Compiler doesn't support -fompss-2 flag, disabling NODES RT tests")
  return()
endif()

if(NOT NODES_FOUND)
  message(STATUS "NODES not found, disabling NODES RT tests")
  return()
else()
  message(STATUS "Enabling NODES RT tests")
endif()

function(nodes_rt_test)
  ovni_test(${ARGN})

  target_compile_options("${OVNI_TEST_NAME}" PUBLIC
    "-fompss-2" "-fdo-not-use-nanos6")

  target_link_options("${OVNI_TEST_NAME}" PUBLIC
    "-fompss-2" "-fdo-not-use-nanos6" "-z" "lazy"
    "-rpath" "${NODES_HOME}/lib")

  target_link_libraries("${OVNI_TEST_NAME}" PRIVATE Nodes::nodes Nodes::wrapper)
  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NANOS6_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nodes/nanos6.toml")

  set_property(TEST "${OVNI_TEST_NAME}" APPEND
    PROPERTY
      ENVIRONMENT "NOSV_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nodes/nosv.toml")
endfunction()

nodes_rt_test(../nanos6/simple-task.c NAME simple-task SORT)
nodes_rt_test(../nanos6/nested-task.c NAME nested-task SORT)
nodes_rt_test(../nanos6/several-tasks.c NAME several-tasks SORT)
nodes_rt_test(../nanos6/if0.c NAME if0 SORT)
nodes_rt_test(../nanos6/sched-add.c NAME sched-add SORT)
nodes_rt_test(taskiter.c SORT)
