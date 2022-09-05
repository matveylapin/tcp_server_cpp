#include <iostream>

#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>	   //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#include "Server.hpp"
#include "Macros.hpp"

#define TRUE 1
#define FALSE 0
#define PORT 8888

using namespace server;

void con_hander(const TCPServerClient &client);
void hander(const TCPServerClient &client);


TCPServer myserver(
	DEFAULT_SERVER_PORT,
	{},
	con_hander,
	[](const TCPServerClient &) {},
	hander);

void con_hander(const TCPServerClient &client)
{
	const char msg[] = "Hello \n";
	const int len = sizeof(msg);
	client.sendDataBuffer((void *)msg, len);
	LOGI(client.getId());
}

void hander(const TCPServerClient &client)
{
	//std::string s((char *)data.data_ptr);
	//if (s.find("stop")) myserver.stop();
}

int main(int argc, char *argv[])
{
	if (myserver.start() == TCPServer::status_t::UP)
	{
		LOGI("Start");
		myserver.joinServer();
	}

	return 0;
}
