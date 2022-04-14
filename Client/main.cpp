#include <iostream>
#include <boost/algorithm/string.hpp>
#include <csignal>

#include "Client.hpp"

static std::unique_ptr<Client> g_pClient = nullptr;

void printUsage()
{
	std::cerr << "Usage: client <host>:<port> [--statistic]\n"
	          << "Example:\n"
	          << "\tclient 0.0.0.0:12345 --statistic\n"
	          << std::endl;
}

void sigIntHandler(int signal)
{
	if (g_pClient)
	{
		g_pClient->stop();
	}
}

int main(int argc, char* argv[])
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;	

	std::cout << "Client" << std::endl << std::endl;

	bool statistic = false;

	if (argc == 2 || argc == 3 && (statistic = !strcmp(argv[2], "--statistic")))
	{
		try
		{
			std::string address(argv[1]);

			std::vector<std::string> res;
			boost::split(res, argv[1], [](char c){ return c == ':'; });

			std::string host = res[0];
			std::string port = res[1];

			g_pClient = std::make_unique<Client>(host, port);

			if (statistic)
			{
				std::cout << "Statistics: " << std::endl << g_pClient->getStatistics() << std::endl;
			}
			else
			{
				std::signal(SIGINT, &sigIntHandler);
				g_pClient->start();
			}

			return EXIT_SUCCESS;
		}
		catch (const std::exception& ex)
		{
			std::cerr << "Error: " << ex.what() << std::endl;
		}
	}
	else
		printUsage();

	return EXIT_FAILURE;
}
