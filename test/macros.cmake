# Copyright (c) 2022-2024 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

include(CMakeParseArguments)

function(ovni_test source)
  set(switches MP SHOULD_FAIL NOEMU SORT BREAKDOWN DISABLED)
  set(single NPROC REGEX NAME DRIVER)
  set(multi ENV)

  cmake_parse_arguments(
    OVNI_TEST "${switches}" "${single}" "${multi}" ${ARGN})

  if(OVNI_TEST_NAME)
    set(test_name "${OVNI_TEST_NAME}")
  else()
    set(test_name "${source}")
  endif()

  # Compute the test name from the source and path
  cmake_path(RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${OVNI_TEST_SOURCE_DIR}"
          OUTPUT_VARIABLE name_prefix)
  set(full_path "${name_prefix}/${test_name}")
  string(REGEX REPLACE "\.c$" "" full_path_noext "${full_path}")
  string(REPLACE "/" "-" name "${full_path_noext}")

  set(OVNI_TEST_NAME ${name})
  set(OVNI_TEST_NAME ${OVNI_TEST_NAME} PARENT_SCOPE)
  set(OVNI_TEST_SOURCE ${source})

  if(NOT OVNI_TEST_NPROC)
    if(NOT OVNI_TEST_MP)
      set(OVNI_TEST_NPROC 1)
    else()
      set(OVNI_TEST_NPROC 4)
    endif()
  endif()

  list(APPEND OVNI_TEST_ENV
    "OVNI_NPROCS=${OVNI_TEST_NPROC}")

  list(APPEND OVNI_TEST_ENV
    "OVNI_BUILD_DIR=${CMAKE_BINARY_DIR}/src/emu")

  list(APPEND OVNI_TEST_ENV
    "OVNI_CURRENT_DIR=${CMAKE_CURRENT_BINARY_DIR}")

  list(APPEND OVNI_TEST_ENV
    "OVNI_CONFIG_DIR=${CMAKE_SOURCE_DIR}/cfg")

  list(APPEND OVNI_TEST_ENV
    "OVNI_SOURCE_DIR=${CMAKE_SOURCE_DIR}")

  if(OVNI_TEST_SORT)
    list(APPEND OVNI_TEST_ENV "OVNI_DO_SORT=1")
  endif()

  if(OVNI_TEST_NOEMU)
    list(APPEND OVNI_TEST_ENV "OVNI_NOEMU=1")
  endif()

  if(OVNI_TEST_BREAKDOWN)
    list(APPEND OVNI_TEST_ENV "OVNI_EMU_ARGS=-b")
  endif()

  if(OVNI_TEST_DRIVER)
    list(APPEND OVNI_TEST_ENV
      "OVNI_DRIVER=${CMAKE_CURRENT_SOURCE_DIR}/${OVNI_TEST_DRIVER}")
  endif()

  include_directories(
    "${CMAKE_SOURCE_DIR}/src/include"
    "${CMAKE_SOURCE_DIR}/src/emu"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/include"
  )

  add_executable("${OVNI_TEST_NAME}" "${OVNI_TEST_SOURCE}")

  target_link_libraries("${OVNI_TEST_NAME}" PRIVATE ovni emu)

  set(driver "${OVNI_TEST_SOURCE_DIR}/ovni-driver.sh")

  if(OVNI_TEST_SHOULD_FAIL)
    if(NOT OVNI_TEST_REGEX)
      message(FATAL_ERROR "You must provide a REGEX for a failing test")
    endif()
    # Custom error handler, as ctest doesn't behave as one would expect.
    add_test(NAME "${OVNI_TEST_NAME}"
      COMMAND
        "${OVNI_TEST_SOURCE_DIR}/match-error.sh"
        "${OVNI_TEST_REGEX}"
        "${driver}"
        "${OVNI_TEST_NAME}"
      WORKING_DIRECTORY "${OVNI_TEST_BUILD_DIR}")
  else()
    if(OVNI_TEST_REGEX)
      # Custom error handler for ok too
      add_test(NAME "${OVNI_TEST_NAME}"
        COMMAND
          "${OVNI_TEST_SOURCE_DIR}/match-ok.sh"
          "${OVNI_TEST_REGEX}"
          "${driver}"
          "${OVNI_TEST_NAME}"
        WORKING_DIRECTORY "${OVNI_TEST_BUILD_DIR}")
    else()
      add_test(NAME "${OVNI_TEST_NAME}"
        COMMAND
          "${driver}"
          "${OVNI_TEST_NAME}"
        WORKING_DIRECTORY "${OVNI_TEST_BUILD_DIR}")
    endif()
  endif()

  set_tests_properties("${OVNI_TEST_NAME}"
    PROPERTIES
      TIMEOUT 60
      RUN_SERIAL TRUE
      ENVIRONMENT "${OVNI_TEST_ENV}"
      WORKING_DIRECTORY "${OVNI_TEST_BUILD_DIR}")

  if(OVNI_TEST_DISABLED)
    set_property(TEST "${OVNI_TEST_NAME}" PROPERTY DISABLED TRUE)
  endif()
endfunction(ovni_test)
