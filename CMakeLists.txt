# Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)

project(OVNI LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

add_compile_options(-Wall -Wextra -Wformat
  -Wmissing-prototypes -Wstrict-prototypes
  #-Wconversion -Wsign-conversion
  -Wold-style-definition -pedantic
  -Werror
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS FALSE)

option(ENABLE_DEBUG_LOG "Enable debug messages (very verbose)")

if(ENABLE_DEBUG_LOG)
  add_definitions(-DENABLE_DEBUG)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Asan UBsan." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_available OUTPUT error LANGUAGES C)

# Enable IPO by default, if available
if(ipo_available)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(WARNING "IPO is not supported, expect performance penalty")
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

add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

install(DIRECTORY cfg/ DESTINATION share/ovni)
