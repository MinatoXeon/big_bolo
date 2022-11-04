add_executable(copy_test test.cc)

target_link_libraries(copy_test copy ${GNU_FS_LIB})
