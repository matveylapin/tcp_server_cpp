#include <iostream>

#include <regex>
#include <chrono>
#include <sstream>
#include <algorithm>

#include "Server.hpp"
#include "Macros.hpp"

#define PORT 8888

uint64_t stringToU64(const std::string &str)
{
	char *end;
	return strtoull(str.c_str(), &end, 10);
}

void con_hander(const TCPServerClient &client)
{
	std::string msg =
	"Hello!!!\n Commands for use:\n seq<1|2|3> <start_value> <step_value> - setting up counter 1, 2 or 3\n export seq - start sending values\n stop seq - stop sending values\n";
	client.sendData(msg);
	LOGI(client.getId());
}

void each_loop_hander(const TCPServerClient &client)
{
	if (client.isCountersRun())
	{
		std::string res = "";
		uint64_t seq[3] = {0};

		for (int i = 0; i < CLIENT_COUNTERS_COUNT; i++)
			seq[i] = client.getNextCounterValue(i);
		for (uint64_t i : seq)
			res += (std::to_string(i) + "\t");
		client.sendData(res);
	}
}

void discon_hander(const TCPServerClient &client)
{
	LOGI("Disconnect.");
}

void hander(const TCPServerClient &client, std::string &msg)
{
	std::regex counter_set_regex("seq(1|2|3) [0-9]{1,20} [0-9]{1,20} ");
	std::regex start_counter_regex("export seq ");
	std::regex stop_counter_regex("stop seq ");

	msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());
	msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
	msg += " ";

	std::stringstream streamData(msg);

	if (std::regex_match(msg, counter_set_regex))
	{
		int seq;
		uint64_t start, step;
		std::string seq_s, start_s, step_s;

		std::getline(streamData, seq_s, ' ');
		std::getline(streamData, start_s, ' ');
		std::getline(streamData, step_s, ' ');

		seq = std::stoi(seq_s.substr(3, 3));
		start = stringToU64(start_s);
		step = stringToU64(step_s);

		client.setCounter(start, step, seq - 1);
		LOGI(msg);

		return;
	}
	else if (std::regex_match(msg, start_counter_regex))
	{
		client.setCountersState(true);

		LOGI(msg);

		return;
	} else if (std::regex_match(msg, stop_counter_regex))
	{
		client.setCountersState(false);

		LOGI(msg);

		return;
	}

	LOGE(msg);
}

int main(int argc, char *argv[])
{
	TCPServer myserver(
		DEFAULT_SERVER_PORT,
		{},
		con_hander,
		each_loop_hander,
		discon_hander,
		hander);
	if (myserver.start() == TCPServer::status_t::UP)
	{
		LOGI("Start");
		myserver.joinServer();
	}

	return 0;
}
