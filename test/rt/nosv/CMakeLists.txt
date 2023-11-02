# Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Nosv)

if(NOT NOSV_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "nOS-V not found, cannot enable nOS-V RT tests")
  else()
    message(STATUS "nOS-V not found, disabling nOS-V RT tests")
  endif()
  return()
else()
  message(STATUS "Enabling nOS-V RT tests")
endif()

function(nosv_test)
  ovni_test(${ARGN})
  target_link_libraries("${OVNI_TEST_NAME}" PRIVATE PkgConfig::NOSV)
  set_property(TEST "${OVNI_TEST_NAME}" APPEND
    PROPERTY
      ENVIRONMENT "NOSV_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nosv/nosv.toml")
endfunction()

nosv_test(attach.c SORT)
nosv_test(waitfor.c SORT)
nosv_test(several-tasks.c SORT)
