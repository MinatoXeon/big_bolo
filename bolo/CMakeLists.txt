add_library(bolo STATIC bolo.cc)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(bolo copy Threads::Threads)
