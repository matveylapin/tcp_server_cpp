add_library(Server
TCPServer.cpp
TCPClient.cpp
)

target_link_libraries(Server PRIVATE)

target_include_directories(Server
    PUBLIC
    "${PROJECT_SOURCE_DIR}/src/include"
    PRIVATE
    private
)