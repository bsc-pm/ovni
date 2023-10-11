# Copyright (c) 2022-2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

include(GNUInstallDirs)

find_library(NANOS6_LIBRARY NAMES nanos6)
find_path(NANOS6_INCLUDE_DIR nanos6.h)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Nanos6 DEFAULT_MSG
  NANOS6_LIBRARY NANOS6_INCLUDE_DIR)

if(NOT NANOS6_FOUND)
  return()
endif()

if(NOT TARGET Nanos6::nanos6)
  add_library(Nanos6::nanos6 SHARED IMPORTED)
  set_target_properties(Nanos6::nanos6 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${NANOS6_INCLUDE_DIR}"
    IMPORTED_LOCATION ${NANOS6_LIBRARY})
endif()
