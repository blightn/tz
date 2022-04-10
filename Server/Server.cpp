#include "Server.hpp"

void Server::startServer()
{

}

void Server::stopServer()
{

}

Server::Server(short port) :
	m_port(port)
{
	startServer();
}

Server::~Server()
{
	stopServer();

	std::cout << "Server's destructor" << std::endl;
}

void Server::start()
{
	while (!needExit)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void Server::stop()
{
	needExit = true;
}
