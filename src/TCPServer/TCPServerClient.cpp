#include "Server.hpp"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "Macros.hpp"

#define BUFFER_SIZE 128

TCPServerClient::TCPServerClient(int socket, sockaddr_in address) : socket_(socket), address_(address)
{
    counters_.resize(CLIENT_COUNTERS_COUNT);
    fcntl(socket_, F_SETFL, O_NONBLOCK);
}

bool TCPServerClient::operator==(const TCPServerClient& client) const { return this->getId() == client.getId(); }

void TCPServerClient::setCountersState(bool run_counter) const
{
    bool dataIsReady = true;

    for (int i = 0; i < CLIENT_COUNTERS_COUNT; i++)
		dataIsReady &= counters_[i].isDataSet();
    
    const_cast<TCPServerClient*>(this)->run_counter_ = run_counter && dataIsReady;
}

void TCPServerClient::setCounter(const CounterStruct& counter, int i) const { const_cast<TCPServerClient*>(this)->counters_[i] = counter; }
void TCPServerClient::setCounter(uint64_t start, uint64_t step, int i) const { const_cast<TCPServerClient*>(this)->counters_[i] = CounterStruct(start, step); }


void TCPServerClient::lock() { access_mutex_.lock(); }
void TCPServerClient::unlock() { access_mutex_.unlock(); }


int TCPServerClient::getId() const { return socket_; }
uint32_t TCPServerClient::getHost() const { return address_.sin_addr.s_addr; }
uint16_t TCPServerClient::getPort() const { return address_.sin_port; }
TCPServerClient::status_t TCPServerClient::getStatus() const { return status_; }
uint64_t TCPServerClient::getNextCounterValue(int i) const { return const_cast<TCPServerClient*>(this)->counters_[i].getNext();};

bool TCPServerClient::isCountersRun() const
{
    bool dataIsReady = true;

    for (int i = 0; i < CLIENT_COUNTERS_COUNT; i++)
		dataIsReady &= counters_[i].isDataSet();
    
    return run_counter_;
}

TCPServerClient::CounterStruct TCPServerClient::getCounter(int i) const { return counters_.at(i);}

TCPServerClient::status_t TCPServerClient::disconnect()
{
    status_ = status_t::DISCONNECTED;

    if (socket_ == -1)
        return status_;

    shutdown(socket_, SHUT_RD);
    close(socket_);

    socket_ = -1;

    return status_;
}

std::string TCPServerClient::getData()
{

    if (status_ != status_t::CONNECTED)
        return std::string("");

    int err;
    uint32_t size = 0;
    char buffer[BUFFER_SIZE];
    std::string buffer_str;

    int resp = recv(socket_, &buffer, BUFFER_SIZE, 0);

    if(resp > 0)
    {
        buffer[resp] = char(0);
        buffer_str = buffer;
    }
    else if(!resp)
    {
        disconnect();
        return buffer_str;
    }
    else if (resp == -1)
    {
        socklen_t len = sizeof(err);
        getsockopt(socket_, SOL_SOCKET, SO_ERROR, &err, &len);
        if (!err)
            err = errno;

        switch (err)
        {
        case 0:
            break;
        case ETIMEDOUT:
            return buffer_str;
        case ECONNRESET:
            return buffer_str;
        case EPIPE:
            disconnect();
            return buffer_str;
        case EAGAIN:
            return buffer_str;
        default:
            disconnect();
            LOGE(std::strerror(err));
            return buffer_str;
        }
    }

    return buffer_str;
}

bool TCPServerClient::sendData(const void *buffer, const size_t size) const
{
    if (status_ != status_t::CONNECTED)
        return false;

    int resp = send(socket_, buffer, size, 0);

    if (resp < 0)
        return false;

    return true;
}

bool TCPServerClient::sendData(const std::string& str) const
{
    std::string str_to_send = "\n";
    str_to_send = str + str_to_send;

    return sendData(str_to_send.c_str(), str_to_send.size());
}

TCPServerClient::~TCPServerClient()
{
    disconnect();
}
