#include "Server.hpp"

#include <stdio.h>
#include <errno.h>

#include "Macros.hpp"

namespace server
{

    TCPServerClient::TCPServerClient(int socket, sockaddr_in address) : socket_(socket), address_(address)
    {
        counters_.resize(CLIENT_COUNTERS_COUNT);
    }

    int TCPServerClient::getId() const {return socket_;}

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
        int err;
        uint32_t size = 0;

        if (status_ != status_t::CONNECTED) return status_t::ERR_WAITDATA;

        int res = recv(socket_, (char *)&size, sizeof(size), MSG_DONTWAIT);

        if (!res)
        {
            disconnect();
            return status_t::ERR_WAITDATA;
        }
        else if (res == -1)
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
            case ECONNRESET:
            case EPIPE:
                disconnect();
            case EAGAIN:
                return status_t::ERR_WAITDATA;
            default:
                disconnect();
                LOGE(std::strerror(err));
            }
        }

        if (size == 0)
            return status_t::ERR_WAITDATA;

        //char* buffer = (char *)calloc(size, sizeof(char));
        char* buffer = (char *)malloc(size);
        recv(socket_, (void *)buffer, size, 0);
        //LOGE(std::strerror(err));
        processData(std::string(buffer, size));

        return status_t::ERR_WAITDATA;
    }

    bool TCPServerClient::sendDataBuffer(const void *buffer, const size_t size)
    {
        if (status_ != status_t::CONNECTED) return false;

        int resp = send(socket_, buffer, size, 0);
        
        if(resp < 0) return false;

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