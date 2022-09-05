add_executable(main "main.cpp")
target_link_libraries(main PRIVATE Server)
target_link_libraries(main PRIVATE ThreadPool)