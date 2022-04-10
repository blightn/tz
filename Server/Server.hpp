#ifndef _SERVER_H_
#define _SERVER_H_

#include <iostream> // !
#include <thread>

class Server
{
	short m_port;
	std::atomic_bool needExit = false;

	void startServer();
	void stopServer();

public:
	Server(short port);
	~Server();

	void start();
	void stop();
};

#endif // _SERVER_H_
