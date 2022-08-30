#include "Server.hpp"

#include <stdio.h>
#include <errno.h>

#include "Macros.hpp"

namespace server
{

    int TCPServerClient::getId() const {return socket_;}

    uint32_t TCPServerClient::getHost() const { return address_.sin_addr.s_addr; }

    uint16_t TCPServerClient::getPort() const { return address_.sin_port; }

    TCPServerClient::status_t TCPServerClient::getStatus() const { return status_; }

    void TCPServerClient::setClientData(void* client_data) { client_data_ = client_data; }
    void TCPServerClient::setClientHandler(std::function<void()> func) { handler_ = func; }

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

    DataBuffer TCPServerClient::waitData()
    {
        int err;
        size_t size = 0;
        DataBuffer buffer;

        if (status_ != status_t::CONNECTED)
            return DataBuffer();

        int res = recv(socket_, (char *)&size, sizeof(size), MSG_DONTWAIT);

        if (!res)
        {
            disconnect();
            return DataBuffer();
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
                return DataBuffer();
            default:
                disconnect();
                LOGE(std::strerror(err));
            }
        }

        if (!size)
            return DataBuffer();
        buffer.size = size;
        recv(socket_, (void *)buffer.data_ptr, buffer.size, 0);

        return buffer;
    }

    bool TCPServerClient::sendData(const void *buffer, const size_t size) const
    {
        if (status_ != status_t::CONNECTED) return false;

        if(send(socket_, buffer, size, 0) < 0) return false;
        return true;
    }

    bool TCPServerClient::sendData(const DataBuffer &data) const
    {
        return sendData(data.data_ptr, data.size);
    }

    TCPServerClient::~TCPServerClient()
    {
        if (socket_ == -1)
            return;

        shutdown(socket_, SHUT_RD);
        close(socket_);
    }
}