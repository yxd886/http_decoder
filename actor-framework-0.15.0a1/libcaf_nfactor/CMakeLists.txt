cmake_minimum_required(VERSION 2.8)
project(caf_nfactor C CXX)

# get header files; only needed by CMake generators,
# e.g., for creating proper Xcode projects
file(GLOB_RECURSE LIBCAF_NFACTOR_HDRS "caf/*.hpp")

# list cpp files excluding platform-dependent files
set (LIBCAF_NFACTOR_SRCS
     src/foo.cpp)

add_custom_target(libcaf_nfactor)

# build shared library if not compiling static only
if (NOT CAF_BUILD_STATIC_ONLY)
  add_library(libcaf_nfactor_shared SHARED ${LIBCAF_NFACTOR_SRCS} ${LIBCAF_NFACTOR_HDRS})
  target_link_libraries(libcaf_nfactor_shared ${LD_FLAGS} ${CAF_LIBRARY_CORE} ${CAF_LIBRARY_IO})
  set_target_properties(libcaf_nfactor_shared
                        PROPERTIES
                        SOVERSION ${CAF_VERSION}
                        VERSION ${CAF_VERSION}
                        OUTPUT_NAME caf_nfactor)
  if(NOT WIN32)
    install(TARGETS libcaf_nfactor_shared LIBRARY DESTINATION lib)
  endif()
  add_dependencies(libcaf_nfactor_shared libcaf_nfactor)
endif ()

# build static library only if --build-static or --build-static-only was set
if (CAF_BUILD_STATIC_ONLY OR CAF_BUILD_STATIC)
  add_library(libcaf_nfactor_static STATIC ${LIBCAF_NFACTOR_HDRS} ${LIBCAF_NFACTOR_SRCS})
  target_link_libraries(libcaf_nfactor_static ${LD_FLAGS} ${CAF_LIBRARY_CORE_STATIC} ${CAF_LIBRARY_IO_STATIC})
  set_target_properties(libcaf_nfactor_static PROPERTIES OUTPUT_NAME caf_nfactor_static)
  if(NOT WIN32)
    install(TARGETS libcaf_nfactor_static ARCHIVE DESTINATION lib)
  endif()
  add_dependencies(libcaf_nfactor_static libcaf_nfactor)
endif ()

link_directories(${LD_DIRS})
include_directories(. ${INCLUDE_DIRS})

# install includes
if(NOT WIN32)
  install(DIRECTORY caf/ DESTINATION include/caf FILES_MATCHING PATTERN "*.hpp")
endif()
