#include "Server.hpp"

#include "Macros.hpp"

namespace server
{

}

/*
int TCPServer::init()
{
	int resp, opt = 1;

	std::fill_n(clientSocket_, SERVER_MAX_CLIENTS, 0);

	masterSocket_ = socket(AF_INET, SOCK_STREAM, 0);
	CHECK(masterSocket_ > 0, "Create master socket.");

	resp = setsockopt(masterSocket_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	CHECK(resp == 0, "Setup master socket");

	address_.sin_family = AF_INET;
	address_.sin_addr.s_addr = INADDR_ANY;
	address_.sin_port = htons(SERVER_PORT);

	resp = bind(masterSocket_, (struct sockaddr *)&address_, sizeof(address_));
	CHECK(resp == 0, "Bind port.");

	CHECK(listen(masterSocket_, 3) == 0, ("Listener on port %d \n", SERVER_PORT));

	addrLen_ = sizeof(address_);

	return SERVER_OK;
}

void TCPServer::run()
{
	int sd, maxSd, activity;
	fd_set readfds;

	FD_ZERO(&readfds);

	FD_SET(masterSocket_, &readfds);
	maxSd = masterSocket_;

	for (int i = 0; i < SERVER_MAX_CLIENTS; i++)
	{
		// socket descriptor
		sd = clientSocket_[i];

		// if valid socket descriptor then add to read list
		if (sd > 0)
			FD_SET(sd, &readfds);

		// highest file descriptor number, need it for the select function
		if (sd > maxSd)
			maxSd = sd;
	}

	activity = select(maxSd + 1, &readfds, NULL, NULL, NULL);

	if ((activity < 0) && (errno != EINTR))
		LOGE("Select falued.");

	if (FD_ISSET(masterSocket_, &readfds))
	{
		if ((newSocket_ = accept(masterSocket_,
								 (struct sockaddr *)&address_, (socklen_t *)&addrLen_)) < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		// inform user of socket number - used in send and receive commands
		LOGI(
			("New connection , socket fd is %d , ip is : %s , port : %d\n",
			newSocket_,
			inet_ntoa(address_.sin_addr),
			ntohs(address_.sin_port)));

		// send new connection greeting message
		if (send(newSocket_, "TEST", strlen("TEST"), 0) != strlen("TEST"))
		{
			perror("send");
		}

		puts("Welcome message sent successfully");

		// add new socket to array of sockets
		for (int i = 0; i < SERVER_MAX_CLIENTS; i++)
		{
			// if position is empty
			if (clientSocket_[i] == 0)
			{
				clientSocket_[i] = newSocket_;
				printf("Adding to list of sockets as %d\n", i);

				break;
			}
		}

		for (int i = 0; i < SERVER_MAX_CLIENTS; i++)
		{
			sd = clientSocket_[i];
				
			if (FD_ISSET( sd , &readfds))
			{
				//Check if it was for closing , and also read the
				//incoming message
				if ((valRead_ = read( sd , buffer_, 1024)) == 0)
				{
					//Somebody disconnected , get his details and print
					getpeername(sd , (struct sockaddr*)&address_ , \
						(socklen_t*)&addrLen_);
					printf("Host disconnected , ip %s , port %d \n" ,
						inet_ntoa(address_.sin_addr) , ntohs(address_.sin_port));
						
					//Close the socket and mark as 0 in list for reuse
					close( sd );
					clientSocket_[i] = 0;
				}
					
				//Echo back the message that came in
				else
				{
					//set the string terminating NULL byte on the end
					//of the data read
					buffer_[valRead_] = '\0';
					send(sd , buffer_ , strlen(buffer_) , 0 );
				}
			}
		}
	}
}
*/