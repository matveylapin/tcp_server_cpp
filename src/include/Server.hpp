#pragma once

#include <list>
#include <thread>
#include <vector>
#include <queue>

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#define SERVER_PORT 8888
#define SERVER_MAX_CLIENTS 30

#define SERVER_OK 0
#define SERVER_ERROR 1

namespace server
{

    class DataBuffer
    {
    public:
        size_t size = 0;
        void *data_ptr = nullptr;

        DataBuffer() = default;
        DataBuffer(void *data_ptr, int size) : data_ptr(data_ptr), size(size) {}
        DataBuffer(const DataBuffer &data) : data_ptr(malloc(data.size)), size(data.size) { memcpy(data.data_ptr, data_ptr, size); }
        DataBuffer(DataBuffer &&data) : data_ptr(data.data_ptr), size(data.size) { data.data_ptr = nullptr; }

        ~DataBuffer()
        {
            if (data_ptr)
                free(data_ptr);
        }

        bool isEmpty() { return data_ptr || size; }

        operator bool() { return data_ptr && size; }
    };

    class TCPServerClient
    {
        enum status_t : uint8_t
        {
            CONNECTED = 0,
            ERR_SOCKET_INIT,
            ERR_SOCKET_BIND,
            ERR_SOCKET_LISTENING,
            DISCONNECTED
        };

    private:
        int socket_;
        std::mutex access_mutex_;
        sockaddr_in address_;
        status_t status_ = status_t::DISCONNECTED;

        void *client_data_ = nullptr;
        std::function<void()> handler_;

    public:
        TCPServerClient(int socket, sockaddr_in address) : socket_(socket), address_(address){};
        TCPServerClient(int socket, sockaddr_in address, void *client_data, std::function<void()> func) : socket_(socket), address_(address), client_data_(client_data), handler_(func){};
        ~TCPServerClient();

        int getId() const;
        uint32_t getHost() const;
        uint16_t getPort() const;
        status_t getStatus() const;

        void setClientData(void *client_data);
        void setClientHandler(std::function<void()> func);

        status_t disconnect();

        DataBuffer waitData();

        bool sendData(const void *buffer, const size_t size) const;
        bool sendData(const DataBuffer &data) const;
    };

    class TCPServer
    {
        typedef std::list<std::unique_ptr<TCPServerClient>>::iterator ClientsIterator_t;
        typedef std::function<void(TCPServerClient &, DataBuffer)> handler_function_t;
        typedef std::function<void(TCPServerClient &)> action_handler_function_t;

        enum status_t : uint8_t
        {
            UP = 0,
            ERR_SOCKET_INIT,
            ERR_SOCKET_BIND,
            ERR_SOCKET_LISTENING,
            DOWN
        };

    private:
        uint16_t port_;
        int master_socket_;

        std::list<std::unique_ptr<TCPServerClient>> clients_;
        std::vector<std::thread> thread_pool_;
        std::queue<std::function<void()>> job_queue;

        class ThreadPool
        {
            template <typename F>
            void addJob(F job);
            template <typename F, typename... Arg>
            void addJob(const F &job, const Arg &...args);
            void join();
            void dropAll();
        } threadPool;

    public:
        TCPServer() = default;

        uint16_t getPort() const { return port_; };
    };

}