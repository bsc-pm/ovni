# Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

if(EXISTS "/proc/sys/kernel/perf_event_paranoid")
  file(READ "/proc/sys/kernel/perf_event_paranoid" paranoid_raw)

  string(REPLACE "\n" "" paranoid_value "${paranoid_raw}")
  message(STATUS "Value of /proc/sys/kernel/perf_event_paranoid is ${paranoid_value}")

  if(paranoid_value LESS_EQUAL 1)
    message(STATUS "Value of perf_event_paranoid suitable for Kernel tests")
    set(PERF_PARANOID_KERNEL ON)
  else()
    message(STATUS "Value of perf_event_paranoid NOT suitable for Kernel tests")
    set(PERF_PARANOID_KERNEL OFF)
  endif()
else()
  message(STATUS "Missing /proc/sys/kernel/perf_event_paranoid")
  set(PERF_PARANOID_KERNEL OFF)
endif()
