#include <iostream>
#include <csignal>

#include "Server.hpp"

static std::unique_ptr<Server> g_pServer = nullptr;

void printUsage()
{
	std::cerr << "Usage: server <port>\n"
	          << "Example:\n"
	          << "\tserver 12345\n"
	          << std::endl;
}

void sigIntHandler(int signal)
{
	if (g_pServer)
		g_pServer->stop();
}

int main(int argc, char* argv[])
{
	std::cout << "Server" << std::endl << std::endl;

	if (argc == 2)
	{
		try
		{
			std::string port = argv[1];
			g_pServer = std::make_unique<Server>(port);

			std::signal(SIGINT, &sigIntHandler);
			g_pServer->start();

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
