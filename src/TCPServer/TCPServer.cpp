#include "Server.hpp"

#include "Macros.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>

TCPServer::TCPServer(uint16_t port,
					 keepAliveConfig KAConfig,
					 action_handler_function_t connect_handler,
					 action_handler_function_t each_loop_handler,
					 action_handler_function_t disconnect_handler,
					 handler_function_t handler) : port_(port),
												   keepAliveConfig_(KAConfig),
												   connect_handler_(connect_handler),
												   each_loop_handler_(each_loop_handler),
												   disconnect_handler_(disconnect_handler),
												   handler_(handler)
{
}

TCPServer::status_t TCPServer::start()
{
	int resp, flag = 1;
	if (status_ == status_t::UP)
		stop();

	server_addr_.sin_family = AF_INET;
	server_addr_.sin_addr.s_addr = INADDR_ANY;
	server_addr_.sin_port = htons(port_);

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

	resp = bind(master_socket_, (struct sockaddr *)&server_addr_, sizeof(sockaddr_in));
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

	thread_pool_.addJob([this]
						{ clientManagerLoop(); });
	thread_pool_.addJob([this]
						{ requestWaitLoop(); });

	status_ = status_t::UP;

	return status_;
}

void TCPServer::requestWaitLoop()
{

	client_mutex_.lock();

	for (auto it = clients_.begin(); it != clients_.end(); it++)
	{
		std::string *msg = new std::string(it->get()->getData());

		if (thread_pool_.isJobInQueue(it->get()->getId()))
			continue;

		if ((it->get()->getStatus() == TCPServerClient::status_t::CONNECTED) &&
			(msg->size() != 0))
		{
			thread_pool_.addJob([this, p = it->get(), msg]
								{
								p->lock();
								handler_(*p, *msg);
								p->unlock(); },
								it->get()->getId());
		}
		else if ((it->get()->getStatus() == TCPServerClient::status_t::CONNECTED) &&
				 (!thread_pool_.isJobInQueue(-it->get()->getId())))
		{
			thread_pool_.addJob([this, p = it->get()]
								{
								using namespace std::chrono_literals;
								p->lock();
								each_loop_handler_(*p);
								p->unlock(); 
								std::this_thread::sleep_for(500ms); },
								-it->get()->getId());
		}
		else if (it->get()->getStatus() != TCPServerClient::status_t::CONNECTED)
		{
			it->get()->lock();
			it->get()->disconnect();
			disconnect_handler_(*(it->get()));
			it->get()->unlock();

			it = clients_.erase(it);
			it--;
		}
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

		thread_pool_.addJob([this, p = newClient.get()]
							{
								p->lock();
								connect_handler_(*p);
								p->unlock(); });

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

void TCPServer::disconnectClient(const TCPServerClient &client)
{
	std::find_if(clients_.begin(), clients_.end(),
				 [&client](const std::unique_ptr<TCPServerClient> &i)
				 { return i->getId() == client.getId(); });
}

void TCPServer::disconnectAll()
{
	client_mutex_.lock();

	for (auto it = clients_.begin(); it != clients_.end(); it++)
		it->get()->disconnect();

	clients_.clear();
	client_mutex_.unlock();
}

void TCPServer::joinServer()
{
	thread_pool_.join();
}

TCPServer::status_t TCPServer::stop()
{
	disconnectAll();
	thread_pool_.terminate();

	status_ = status_t::STOPED;

	return status_;
}

TCPServer::~TCPServer()
{
	stop();
}
