#include "Server.hpp"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "Macros.hpp"

#define BUFFER_SIZE 128

namespace server
{

    TCPServerClient::TCPServerClient(int socket, sockaddr_in address) : socket_(socket), address_(address)
    {
        counters_.resize(CLIENT_COUNTERS_COUNT);
        fcntl(socket_, F_SETFL, O_NONBLOCK);
    }

    void TCPServerClient::lock() { access_mutex_.lock(); }
    void TCPServerClient::unlock(){ access_mutex_.unlock(); }

    int TCPServerClient::getId() const { return socket_; }

    uint32_t TCPServerClient::getHost() const { return address_.sin_addr.s_addr; }

    uint16_t TCPServerClient::getPort() const { return address_.sin_port; }

    TCPServerClient::status_t TCPServerClient::getStatus() const { return status_; }

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

    TCPServerClient::status_t TCPServerClient::waitData()
    {

        if (status_ != status_t::CONNECTED)
            return status_t::ERR_WAITDATA;

        int err;
        uint32_t size = 0;
        // fd_set rfds;
        // struct timeval tv;

        // tv.tv_usec = 50;
        // tv.tv_sec = 0;
        // FD_ZERO(&rfds);
        // FD_SET(socket_, &rfds);

        char buffer[BUFFER_SIZE];
        //socklen_t addr_len = sizeof(sockaddr_in);

        int resp = recv(socket_, &buffer, BUFFER_SIZE, 0);
        // int resp = select(socket_ + 1, &rfds, NULL, NULL, &tv);

        buffer[BUFFER_SIZE-1] = '/0';

        if (!resp)
        {
            disconnect();
            return status_t::ERR_WAITDATA;
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
                return status_t::ERR_WAITDATA;
            case ECONNRESET:
                return status_t::ERR_WAITDATA;
            case EPIPE:
                disconnect();
                return status_t::ERR_WAITDATA;
            case EAGAIN:
                return status_t::ERR_WAITDATA;
            default:
                disconnect();
                LOGE(std::strerror(err));
                return status_t::ERR_WAITDATA;
            }
        }

        if (size == 0 && false)
            return status_t::ERR_WAITDATA;
        

        //LOGI(s);
        processData(std::string(buffer));

        return status_t::ERR_WAITDATA;
    }

    bool TCPServerClient::sendDataBuffer(const void *buffer, const size_t size) const
    {
        if (status_ != status_t::CONNECTED)
            return false;

        int resp = send(socket_, buffer, size, 0);

        if (resp < 0)
            return false;

        return true;
    }

    void TCPServerClient::processData(std::string s)
    {
        LOGI(s);
        LOGI(s.length());
    }

    TCPServerClient::~TCPServerClient()
    {
        disconnect();
    }
}