#include "Server.hpp"

void Server::clientThread(tcp::socket socket)
{
	try
	{
		websocket::stream<tcp::socket> ws{ std::move(socket) };
		ws.accept();

		while (!m_needExit)
		{
			beast::flat_buffer buffer;

			//ws.read(buffer);
			//ws.text(ws.got_text());
			//ws.write(buffer.data());

			ws.read(buffer);

			tz::ClientPacket packet;
			packet.ParseFromString(beast::buffers_to_string(buffer.data()));

			switch (packet.type())
			{
			case tz::ClientPacket::DATA:
				saveClientPacket(packet);
				break;

			case tz::ClientPacket::STATISTICS:
				//tz::ServerStatistic stats = collectStatistics();
				ws.write(net::buffer(collectStatistics()->SerializeAsString()));
				break;

			default:
				std::cout << "Unknown packet type. Ignoring..." << std::endl;
			}

			//std::cout << beast::make_printable(buffer.data()) << std::endl;
		}
	}
	catch (beast::system_error const&)
	{
		std::cout << "Connection closed." << std::endl;
	}

	std::cout << "Client disconnected." << std::endl;
}

void Server::saveClientPacket(tz::ClientPacket& packet)
{
	if (packet.has_data())
	{

	}
}

std::unique_ptr<tz::ServerStatistic> Server::collectStatistics()
{
	auto stats = std::make_unique<tz::ServerStatistic>();



	return stats;
}

Server::Server(std::string& port) :
	m_port(port)
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
	acceptor.non_blocking(true); // Because async_accept() doesn't want to run the function!!!

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
