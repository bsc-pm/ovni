include(CheckCCompilerFlag)

set(CMAKE_REQUIRED_LINK_OPTIONS "-fompss-2")
check_c_compiler_flag("-fompss-2" OMPSS2_COMPILER_FOUND)
