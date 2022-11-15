add_executable(bolo_test test.cc)

target_link_libraries(bolo_test bolo tar crypto compress ${CMAKE_DL_LIBS} ${GNU_FS_LIB})
