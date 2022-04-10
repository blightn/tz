#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <iostream> // !
#include <string>
#include <random>
#include <thread>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class Client
{
	std::string m_host;
	std::string m_port;

	net::io_context m_ioc;
	websocket::stream<tcp::socket>* m_pws;

	std::atomic_bool m_needExit = false;

	void connect();
	void disconnect();

public:
	Client(std::string& host, std::string& port);
	~Client();

	void start();
	void stop();

	std::string getStatistics();
};

#endif // _CLIENT_H_
