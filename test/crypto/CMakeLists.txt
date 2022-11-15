add_executable(crypto_test test.cc)

target_link_libraries(crypto_test crypto ${GNU_FS_LIB} ${CMAKE_DL_LIBS})
