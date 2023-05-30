# Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

find_library(nosv libnosv)
find_path(NOSV_INCLUDE_DIR nosv.h)

check_library_exists(nosv nosv_init "nosv.h" HAVE_NOSV)

if(NOT HAVE_NOSV)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "nOS-V library not found, cannot enable nOS-V RT tests")
  else()
    message(STATUS "nOS-V library not found, disabling nOS-V RT tests")
  endif()
  return()
else()
  message(STATUS "Enabling nOS-V RT tests")
endif()

function(nosv_test)
  ovni_test(${ARGN})
  target_link_libraries("${OVNI_TEST_NAME}" PRIVATE nosv)
  target_include_directories("${OVNI_TEST_NAME}"
    PUBLIC ${NOSV_INCLUDE_DIR})
  set_property(TEST "${OVNI_TEST_NAME}" APPEND
    PROPERTY
      ENVIRONMENT "NOSV_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nosv/nosv.toml")
endfunction()

nosv_test(attach.c SORT)
nosv_test(waitfor.c SORT)
nosv_test(several-tasks.c SORT)
