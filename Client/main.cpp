#include <iostream>
#include <boost/algorithm/string.hpp>
#include <csignal>

#include "Client.hpp"

static Client* g_pClient = nullptr;

void printUsage()
{
	std::cerr << "Usage:\n"
	          << "\thost:port\n"
	          << "\t--statistic"
	          << std::endl;
}

void sigIntHandler(int signal)
{
	g_pClient->stop();
}

int main(int argc, char* argv[])
{
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

			g_pClient = new Client(host, port);

			if (statistic)
			{
				std::cout << "Statistics: " << g_pClient->getStatistics() << std::endl;
			}
			else
			{
				std::signal(SIGINT, &sigIntHandler);
				g_pClient->start();
			}

			delete g_pClient;
			g_pClient = nullptr;

			return EXIT_SUCCESS;
		}
		catch (std::exception const& ex)
		{
			std::cerr << "Error: " << ex.what() << std::endl;
		}
	}
	else
		printUsage();

	return EXIT_FAILURE;
}
