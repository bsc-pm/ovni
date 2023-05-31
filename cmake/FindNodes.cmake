include(GNUInstallDirs)

if(DEFINED ENV{NODES_HOME})
  set(NODES_HOME "$ENV{NODES_HOME}")
else()
  message(STATUS "NODES_HOME not set, refusing to search")
endif()

find_library(NODES_LIBRARY NAMES nodes PATHS "${NODES_HOME}/lib" NO_DEFAULT_PATH)
find_file(NODES_WRAPPER NAMES nodes-main-wrapper.o PATHS "${NODES_HOME}/lib" NO_DEFAULT_PATH)
find_path(NODES_INCLUDE_DIR nodes.h PATHS "${NODES_HOME}/include" NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Nodes DEFAULT_MSG
  NODES_LIBRARY NODES_INCLUDE_DIR NODES_WRAPPER)

if(NOT NODES_FOUND)
  return()
endif()

if(NOT TARGET Nodes::nodes)
  add_library(Nodes::nodes SHARED IMPORTED)
  set_target_properties(Nodes::nodes PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${NODES_INCLUDE_DIR}"
    IMPORTED_LOCATION ${NODES_LIBRARY})
endif()

if(NOT TARGET Nodes::wrapper)
  add_library(Nodes::wrapper STATIC IMPORTED)
  set_target_properties(Nodes::wrapper PROPERTIES
    IMPORTED_LOCATION ${NODES_WRAPPER})
endif()
