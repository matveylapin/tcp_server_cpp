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

struct keepAliveConfig
{
    uint16_t idle = 120, intvl = 10, cnt = 5;
};

class TCPServerClient
{
public:

    class CounterStruct
    {
    private:
        uint64_t start_ = 0, now_ = 0, step_ = 0;
    public:
        CounterStruct() = default;
        CounterStruct(uint64_t start, uint64_t step) : start_(start), step_(step), now_(start){};

        uint64_t getNext()
        {
            uint64_t tmp = now_;
            now_ = ((now_ + step_) >= start_) ? (now_ + step_) : start_;
            return tmp;
        };

        bool isDataSet() const
        {
            return (start_ != 0) || (step_ != 0);
        };

    };

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

    void lock();
    void unlock();

    void setCountersState(bool run_counter) const;
    void setCounter(const CounterStruct& counter, int i) const;
    void setCounter(uint64_t start, uint64_t step, int i) const;

    int getId() const;
    uint32_t getHost() const;
    uint16_t getPort() const;
    status_t getStatus() const;
    CounterStruct getCounter(int i) const;
    bool isCountersRun() const;
    uint64_t getNextCounterValue(int i) const;

    status_t disconnect();

    std::string getData();

    bool sendData(const void *buffer, const size_t size) const;
    bool sendData(const std::string& str) const;

    bool operator==(const TCPServerClient& client) const;

private:

    int socket_;
    std::mutex access_mutex_;
    sockaddr_in address_;
    status_t status_ = status_t::CONNECTED;
    std::vector<CounterStruct> counters_;

    bool run_counter_ = false;
};

class TCPServer
{
public:
    typedef std::function<void(const TCPServerClient &)> action_handler_function_t;
    typedef std::function<void(const TCPServerClient &, std::string&)> handler_function_t;

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
              action_handler_function_t each_loop_handler = {},
              action_handler_function_t disconnect_handler = {},
              handler_function_t handler = {});

    ~TCPServer();

    status_t start();
    status_t stop();
    void disconnectClient(const TCPServerClient& client);
    void disconnectAll();

    uint16_t getPort() const { return port_; };

    void joinServer();

private:
    status_t status_ = status_t::STOPED;

    uint16_t port_;
    int master_socket_;
    keepAliveConfig keepAliveConfig_;
    sockaddr_in server_addr_;

    std::mutex client_mutex_;

    action_handler_function_t connect_handler_ = {};
    action_handler_function_t each_loop_handler_ = {};
    action_handler_function_t disconnect_handler_ = {};
    handler_function_t handler_ = {};

    ThreadPool thread_pool_;

    std::list<std::unique_ptr<TCPServerClient>> clients_;

    void clientManagerLoop();
    void requestWaitLoop();
    bool keepAliveForClientSetup(int socket);
};
