#include "Client.hpp"

void Client::connect()
{

}

void Client::disconnect()
{

}

Client::Client(std::string& host, short port) :
	m_host(host),
	m_port(port)
{
	connect();
}

Client::~Client()
{
	disconnect();

	std::cout << "Client's destructor" << std::endl;
}

void Client::start()
{
	std::random_device rd;
	std::default_random_engine dre(rd());
	std::uniform_int_distribution<int> distr(5, 30);

	auto nextTime = std::chrono::system_clock::now() + std::chrono::seconds(distr(dre));

	do
	{	
		if (std::chrono::system_clock::now() >= nextTime)
		{
			std::cout << "Sending a packet..." << std::endl;

			nextTime = std::chrono::system_clock::now() + std::chrono::seconds(distr(dre));
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));

	} while (!needExit);
}

void Client::stop()
{
	needExit = true;
}

std::string Client::getStatistics()
{
	return "Statistics";
}
