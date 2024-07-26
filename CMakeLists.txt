# Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)

project(OVNI LANGUAGES C VERSION 1.10.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

add_compile_options(-Wall -Wextra -Wformat
  -Wmissing-prototypes -Wstrict-prototypes
  #-Wconversion -Wsign-conversion
  -Wold-style-definition -pedantic
  -Werror
)

set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS FALSE)

option(ENABLE_DEBUG_LOG "Enable debug messages (very verbose)")
option(ENABLE_ALL_TESTS "Forces the execution of all tests")
set(OVNI_GIT_COMMIT "unknown" CACHE STRING "Set the git commit")

if("${OVNI_GIT_COMMIT}" STREQUAL "unknown")
  message(WARNING "OVNI_GIT_COMMIT is unknown, please specify the git commit")
endif()

if(ENABLE_DEBUG_LOG)
  add_definitions(-DENABLE_DEBUG)
endif()

add_definitions(-D_POSIX_C_SOURCE=200809L)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Asan UBsan." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

include(GNUInstallDirs)
set(OVNI_CONFIG_RELDIR "${CMAKE_INSTALL_DATADIR}/ovni")
set(OVNI_CONFIG_DIR "${CMAKE_INSTALL_PREFIX}/${OVNI_CONFIG_RELDIR}")

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_available OUTPUT error LANGUAGES C)

# Enable IPO by default, if available
if(NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  if(ipo_available)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO is not supported, disabling")
  endif()
endif()

if(NOT CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  message(WARNING "IPO is not enabled, expect performance penalty")
endif()

# Extra build type for AddressSanitizer (Asan)
set(CMAKE_C_FLAGS_ASAN "${CMAKE_C_FLAGS_DEBUG} \
  -fsanitize=address \
  -fno-optimize-sibling-calls \
  -fsanitize-address-use-after-scope \
  -fno-omit-frame-pointer"
  CACHE STRING "Flags used by the C compiler during AddressSanitizer builds." FORCE)

set(CMAKE_C_FLAGS_UBSAN "${CMAKE_C_FLAGS_DEBUG} -fsanitize=undefined"
  CACHE STRING "Flags used by the C compiler during UndefinedBehaviorSanitizer builds." FORCE)

find_program(IWYU NAMES include-what-you-use iwyu)

if(IWYU)
  set(IWYU_CMD ${IWYU} -Xiwyu --no_comments)
  set(CMAKE_C_INCLUDE_WHAT_YOU_USE ${IWYU_CMD})
  message(STATUS "IWYU found")
else()
  message(WARNING "IWYU not found, skipping")
endif()

# Required for clock_gettime() in glibc <= 2.17
include(CheckLibraryExists)
check_library_exists(c clock_gettime "" HAVE_CLOCK_GETTIME)

if (NOT HAVE_CLOCK_GETTIME)
  check_library_exists(rt clock_gettime "time.h" HAVE_CLOCK_GETTIME_RT)
  if (HAVE_CLOCK_GETTIME_RT)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lrt")
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lrt")
  else()
    message(FATAL_ERROR "cannot find clock_gettime()")
  endif()
endif()

add_subdirectory(include)
add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

install(DIRECTORY cfg/ DESTINATION "${OVNI_CONFIG_RELDIR}")

include(FeatureSummary)
feature_summary(WHAT ALL)
