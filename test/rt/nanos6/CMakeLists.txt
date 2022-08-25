function(nanos6_rt_test)
  ovni_test(${ARGN})
  target_compile_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2")
  target_link_options("${OVNI_TEST_NAME}" PUBLIC "-fompss-2")
  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NANOS6_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nanos6/nanos6.toml")
endfunction()

nanos6_rt_test(NAME rt-nanos6-simple-task SOURCE simple-task.c)
nanos6_rt_test(NAME rt-nanos6-nested-task SOURCE nested-task.c)
