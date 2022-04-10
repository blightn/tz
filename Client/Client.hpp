#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <iostream> // !
#include <string>
#include <random>
#include <thread>

class Client
{
	std::string m_host;
	short m_port;
	std::atomic_bool needExit = false;

	void connect();
	void disconnect();

public:
	Client(std::string& host, short port);
	~Client();

	void start();
	void stop();

	std::string getStatistics();
};

#endif // _CLIENT_H_
