# find static openssl lib
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

add_library(crypto STATIC crypto.cc)
target_link_libraries(crypto OpenSSL::SSL OpenSSL::Crypto)
