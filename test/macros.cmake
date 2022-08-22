include(CMakeParseArguments)

function(ovni_test)
  set(switches MP SHOULD_FAIL)
  set(single NPROC NAME REGEX)
  set(multi SOURCE ENV)

  cmake_parse_arguments(
    OVNI_TEST "${switches}" "${single}" "${multi}" ${ARGN})

  if(NOT OVNI_TEST_NAME)
    message(FATAL_ERROR "You must provide a test NAME")
  endif(NOT OVNI_TEST_NAME)

  set(OVNI_TEST_NAME ${OVNI_TEST_NAME} PARENT_SCOPE)

  # Set default source if not given
  if(NOT OVNI_TEST_SOURCE)
    set(OVNI_TEST_SOURCE "${OVNI_TEST_NAME}.c")
    #message("Setting default source to ${OVNI_TEST_SOURCE}")
  endif()

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
    "OVNI_BUILD_DIR=${CMAKE_BINARY_DIR}")

  list(APPEND OVNI_TEST_ENV
    "OVNI_CURRENT_DIR=${CMAKE_CURRENT_BINARY_DIR}")

  add_executable("${OVNI_TEST_NAME}" "${OVNI_TEST_SOURCE}")
  target_link_libraries("${OVNI_TEST_NAME}" ovni)

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
    add_test(NAME "${OVNI_TEST_NAME}"
      COMMAND
        "${driver}"
        "${OVNI_TEST_NAME}"
      WORKING_DIRECTORY "${OVNI_TEST_BUILD_DIR}")
  endif()

  set_tests_properties("${OVNI_TEST_NAME}"
    PROPERTIES
      RUN_SERIAL TRUE
      ENVIRONMENT "${OVNI_TEST_ENV}"
      WORKING_DIRECTORY "${OVNI_TEST_BUILD_DIR}")
endfunction(ovni_test)
