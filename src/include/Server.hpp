#pragma once

#include <list>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ThreadPool.hpp"

#define CLIENT_COUNTERS_COUNT 3

#define DEFAULT_SERVER_PORT (uint16_t)(9999)
#define SERVER_MAX_CLIENTS 30

namespace server
{

    struct keepAliveConfig
    {
        uint16_t idle = 120, intvl = 10, cnt = 5;
    };

    class TCPServerClient
    {
    public:
        enum status_t : uint8_t
        {
            CONNECTED = 0,
            ERR_SOCKET_INIT,
            ERR_SOCKET_BIND,
            ERR_SOCKET_LISTENING,
            ERR_WAITDATA,
            DISCONNECTED
        };

        TCPServerClient(int socket, sockaddr_in address);
        ~TCPServerClient();

        int getId() const;
        uint32_t getHost() const;
        uint16_t getPort() const;
        status_t getStatus() const;

        status_t disconnect();

        status_t waitData();
        bool sendDataBuffer(const void *buffer, const size_t size);

    private:
        typedef struct
        {
            uint64_t start = 0, now = 0, step = 0;
            uint64_t getNext()
            {
                now = (now + step >= start) ? (now + step) : start;
                return now;
            };
            bool isDataSet()
            {
                return (start != 0) || (step != 0);
            };
        } counter_t;

        int socket_;
        std::mutex access_mutex_;
        sockaddr_in address_;
        status_t status_ = status_t::CONNECTED;
        std::vector<counter_t> counters_;

        void processData(std::string s);
    };

    class TCPServer
    {
    public:
        typedef std::function<void(const TCPServerClient &)> action_handler_function_t;
        typedef std::list<std::unique_ptr<TCPServerClient>>::iterator ClientsIterator_t;

        enum status_t : uint8_t
        {
            UP = 0,
            ERR_SOCKET_INIT,
            ERR_SOCKET_BIND,
            ERR_SOCKET_SETOPTION,
            ERR_SOCKET_LISTENING,
            STOPED
        };

        TCPServer(uint16_t port = DEFAULT_SERVER_PORT,
                  keepAliveConfig KAConfig = {},
                  action_handler_function_t connect_handler = {},
                  action_handler_function_t disconnect_handler = {},
                  action_handler_function_t handler = {});

        ~TCPServer();

        status_t start();
        status_t stop();
        void disconnectAll();

        uint16_t getPort() const { return port_; };

        void joinServer();

    private:
        status_t status_ = status_t::STOPED;

        uint16_t port_;
        int master_socket_;
        keepAliveConfig keepAliveConfig_;

        std::mutex client_mutex_;

        action_handler_function_t connect_handler_ = {};
        action_handler_function_t disconnect_handler_ = {};
        action_handler_function_t handler_ = {};

        ThreadPool thread_pool_;

        std::list<std::unique_ptr<TCPServerClient>> clients_;

        void clientManagerLoop();
        void requestWaitLoop();
        bool keepAliveForClientSetup(int socket);
    };

}