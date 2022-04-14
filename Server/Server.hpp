#ifndef _SERVER_H_
#define _SERVER_H_

#include <iostream> // !
#include <thread>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "../protobuf/tz.pb.h"
#include "SQLite.hpp"

#ifdef _DEBUG
#	pragma comment(lib, "libprotobufd")
#else
#	pragma comment(lib, "libprotobuf")
#endif

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class Server
{
	inline static const std::string DB_NAME = "tz.sqlite3";

	std::string              m_port;
	net::io_context          m_ioc;
	std::unique_ptr<SQLite>  m_psqlite3;
	std::vector<std::thread> m_threads;
	std::atomic_bool         m_needExit = false;

	void clientThread(tcp::socket socket);

	void saveClientPacket(const tz::ClientPacket& packet);
	std::unique_ptr<tz::ServerStatistic> collectStatistics();

public:
	Server(const std::string& port);
	~Server();

	void start();
	void stop();
};

#endif // _SERVER_H_
