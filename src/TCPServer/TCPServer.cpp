#include "Server.hpp"

#include "Macros.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>

namespace server
{
	TCPServer::TCPServer(uint16_t port,
						 keepAliveConfig KAConfig,
						 action_handler_function_t connect_handler,
						 action_handler_function_t disconnect_handler,
						 action_handler_function_t handler) : port_(port),
													   keepAliveConfig_(KAConfig),
													   connect_handler_(connect_handler),
													   disconnect_handler_(disconnect_handler),
													   handler_(handler)
	{
	}

	TCPServer::status_t TCPServer::start()
	{
		int resp, flag = 1;
		if (status_ == status_t::UP)
			stop();

		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port_);

		master_socket_ = socket(AF_INET, SOCK_STREAM, 0);
		if (master_socket_ == -1)
		{
			LOGE("Socket init.");
			return status_ = status_t::ERR_SOCKET_INIT;
		}

		resp = setsockopt(master_socket_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
		if (resp != 0)
		{
			LOGE("Socket set SO_REUSEADDR.");
			return status_ = status_t::ERR_SOCKET_SETOPTION;
		}

		resp = bind(master_socket_, (struct sockaddr *)&address, sizeof(sockaddr_in));
		if (resp < 0)
		{
			LOGE("Socket bind.");
			return status_ = status_t::ERR_SOCKET_BIND;
		}

		resp = listen(master_socket_, 3);
		if (resp < 0)
		{
			LOGE("Socket listen.");
			return status_ = status_t::ERR_SOCKET_LISTENING;
		}

		thread_pool_.addJob([this]{ clientManagerLoop(); });
		thread_pool_.addJob([this]{ requestWaitLoop(); });

		status_ = status_t::UP;

		return status_;
	}

	void TCPServer::requestWaitLoop()
	{
		client_mutex_.lock();
		for (auto i = clients_.begin(); i != clients_.end(); i++)
		{
			i->get()->waitData();
		}
		client_mutex_.unlock();

		thread_pool_.addJob([this]
							{ requestWaitLoop(); });
	}

	void TCPServer::clientManagerLoop()
	{
		sockaddr_in address;
		int addrlen = sizeof(sockaddr_in);
		int newSocket = accept(master_socket_, (sockaddr *)&address, (socklen_t *)&addrlen);

		if ((status_ != status_t::UP) || (newSocket == -1))
			return;

		if (keepAliveForClientSetup(newSocket))
		{
			std::unique_ptr<TCPServerClient> newClient(new TCPServerClient(newSocket, address));

			// thread_pool_.addJob([this, &newClient]
			//					{ connect_handler_(*newClient); });

			connect_handler_(*newClient);

			client_mutex_.lock();
			clients_.push_back(std::move(newClient));
			client_mutex_.unlock();
		}
		else
		{
			shutdown(newSocket, SHUT_RD);
			close(newSocket);
		}

		thread_pool_.addJob([this]
							{ clientManagerLoop(); });
	}

	bool TCPServer::keepAliveForClientSetup(int newSocket)
	{
		int resp, alive = 1;
		int idle = (keepAliveConfig_.idle * 1000), intvl = (keepAliveConfig_.intvl * 1000), cnt = keepAliveConfig_.cnt;

		resp = setsockopt(newSocket, SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof(alive));
		if (resp == -1)
			return false;

		resp = setsockopt(newSocket, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle));
		if (resp == -1)
			return false;

		resp = setsockopt(newSocket, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
		if (resp == -1)
			return false;

		resp = setsockopt(newSocket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
		if (resp == -1)
			return false;

		return true;
	}

	void TCPServer::disconnectAll()
	{
		client_mutex_.lock();

		for (auto it = clients_.begin(); it != clients_.end(); it++)
		{
			auto &client = *it;
			client.get()->disconnect();
		}

	}

	void TCPServer::joinServer()
	{
		thread_pool_.join();
	}

	TCPServer::status_t TCPServer::stop()
	{
		disconnectAll();
		thread_pool_.terminate();
		clients_.clear();

		status_ = status_t::STOPED;

		LOGI("Stoped.");

		return status_;
	}

	TCPServer::~TCPServer()
	{
		stop();
	}
}
