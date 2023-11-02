# Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig)

if(NOT PKG_CONFIG_FOUND)
  message(STATUS "pkg-config not found, required to locate nOSV-V")
  return()
endif()

# Use PKG_CONFIG_LIBDIR=/path/to/nosv/install/lib/pkgconfig to use a custom
# installation.
pkg_search_module(NOSV IMPORTED_TARGET nos-v)

if(NOT NOSV_FOUND)
  message(STATUS "nOS-V not found")
else()
  message(STATUS "Found nOS-V ${NOSV_VERSION} at ${NOSV_LINK_LIBRARIES}")
endif()
