#include "Server.hpp"

void Server::clientThread(tcp::socket socket)
{
	try
	{
		websocket::stream<tcp::socket> ws{ std::move(socket) };
		ws.binary(true);
		ws.accept();

		while (!m_needExit)
		{
			beast::flat_buffer buffer;
			ws.read(buffer);

			tz::ClientPacket packet;
			packet.ParseFromString(beast::buffers_to_string(buffer.data()));

			switch (packet.type())
			{
			case tz::ClientPacket::DATA:
				saveClientPacket(packet);
				break;

			case tz::ClientPacket::STATISTICS:
				ws.write(net::buffer(collectStatistics()->SerializeAsString()));
				break;

			default:
				std::cout << "Unknown packet type. Ignoring..." << std::endl;
			}
		}
	}
	catch (beast::system_error const&)
	{
		std::cout << "Connection closed." << std::endl;
	}

	std::cout << "Client disconnected." << std::endl;
}

void Server::saveClientPacket(const tz::ClientPacket& packet)
{
	if (packet.has_data())
	{
		auto data = packet.data();
		std::string tmp = "Packet: " + data.uuid() + " " + std::to_string(data.timestamp()) + " " + std::to_string(data.x()) + " " + std::to_string(data.y());
		std::cout << tmp << std::endl;
	}
}

std::unique_ptr<tz::ServerStatistic> Server::collectStatistics()
{
	auto stats = std::make_unique<tz::ServerStatistic>();

	auto client = stats->add_client();
	client->set_uuid("uuid");
	client->set_x1(1.0);
	client->set_y1(2.0);
	client->set_x5(3.0);
	client->set_y5(4.0);

	return stats;
}

Server::Server(const std::string& port) :
	m_port(port),
	m_psqlite3(std::make_unique<SQLite>(Server::DB_NAME))
{

}

Server::~Server()
{
	std::cout << "Server's destructor" << std::endl;
}

void Server::start()
{
	auto const address = net::ip::make_address("0.0.0.0");
	auto const port = static_cast<unsigned short>(std::atoi(m_port.c_str()));

	tcp::acceptor acceptor{ m_ioc, {address, port} };
	acceptor.non_blocking(true);

	while (!m_needExit)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		try
		{
			tcp::socket socket{ m_ioc };
			acceptor.accept(socket);
			m_threads.emplace_back(std::thread(&Server::clientThread, this, std::move(socket)));
		}
		catch (beast::system_error const&)
		{
		}
	}

	for (auto& th : m_threads)
	{
		th.join();
	}
}

void Server::stop()
{
	m_needExit = true;
}
