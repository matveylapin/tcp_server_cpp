add_library(ThreadPool
ThreadPool.cpp
)

target_link_libraries(ThreadPool PRIVATE)

target_include_directories(ThreadPool
    PUBLIC
    "${PROJECT_SOURCE_DIR}/src/include"
    PRIVATE
    private
)