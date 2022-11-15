cmake_minimum_required(VERSION 3.10)
# set the project name
project(bolo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(GNU_FS_LIB stdc++fs)
endif()

# set include_directories
include_directories("include")

add_subdirectory(copy)
add_subdirectory(compress)
add_subdirectory(tar)
add_subdirectory(crypto)
add_subdirectory(bolo)
add_subdirectory(libfswatch)
add_subdirectory(app)
add_subdirectory(test)
