#include <iostream>
#include <boost/lexical_cast.hpp>
#include <csignal>

#include "Server.hpp"

static Server* g_pServer = nullptr;

void printUsage()
{
	std::cerr << "Usage:\n"
	          << "\tport\n"
	          << std::endl;
}

void sigIntHandler(int signal)
{
	g_pServer->stop();
}

int main(int argc, char* argv[])
{
	std::cout << "Server" << std::endl << std::endl;

	if (argc == 2)
	{
		try
		{
			short port = boost::lexical_cast<short>(argv[1]);

			g_pServer = new Server(port);
			std::signal(SIGINT, &sigIntHandler);
			g_pServer->start();

			delete g_pServer;
			g_pServer = nullptr;

			return EXIT_SUCCESS;
		}
		catch (boost::bad_lexical_cast const&)
		{
			std::cout << "Bad port" << std::endl;
		}
		catch (std::exception const& ex)
		{
			std::cout << "Exception: " << ex.what() << std::endl;
		}
	}
	else
		printUsage();

	return EXIT_FAILURE;
}
