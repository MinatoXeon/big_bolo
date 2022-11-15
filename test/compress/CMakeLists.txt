add_executable(compress_test test.cc)

target_link_libraries(compress_test compress ${GNU_FS_LIB})
