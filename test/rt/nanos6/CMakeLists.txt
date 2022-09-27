find_library(nanos6 libnanos6)
find_path(NANOS6_INCLUDE_DIR nanos6.h)

function(nanos6_rt_test)
  set(switches "")
  set(single LEVEL)
  set(multi "")
  cmake_parse_arguments(
    NANOS6_TEST "${switches}" "${single}" "${multi}" ${ARGN})
  ovni_test(${ARGN})
  target_compile_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2")
  target_link_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2")
  target_link_libraries("${OVNI_TEST_NAME}" nanos6)
  target_include_directories("${OVNI_TEST_NAME}"
    PUBLIC ${NANOS6_INCLUDE_DIR})
  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NANOS6_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nanos6/nanos6.toml")
  if(NANOS6_TEST_LEVEL)
    set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
      ENVIRONMENT "NANOS6_CONFIG_OVERRIDE=instrument.ovni.level=${NANOS6_TEST_LEVEL}")
  endif()
endfunction()

nanos6_rt_test(simple-task.c)
nanos6_rt_test(nested-task.c)
nanos6_rt_test(several-tasks.c)
nanos6_rt_test(sched-add.c)
nanos6_rt_test(if0.c)
nanos6_rt_test(taskfor.c)
nanos6_rt_test(simple-task.c NAME simple-task-level-1 LEVEL 1)
nanos6_rt_test(simple-task.c NAME simple-task-level-2 LEVEL 2)
nanos6_rt_test(simple-task.c NAME simple-task-level-3 LEVEL 3)
