add_executable(tar_test test.cc)

target_link_libraries(tar_test tar ${GNU_FS_LIB})
