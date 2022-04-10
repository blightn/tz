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

			ws.read(buffer);
			ws.text(ws.got_text());
			ws.write(buffer.data());

			std::cout << beast::make_printable(buffer.data()) << std::endl;
		}
	}
	catch (beast::system_error const& se)
	{
		throw std::exception("Connection closed.");
	}
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
		std::this_thread::sleep_for(std::chrono::seconds(1));

		try
		{
			tcp::socket socket{ m_ioc };
			acceptor.accept(socket);
			std::thread(&Server::clientThread, this, std::move(socket)).detach();
		}
		catch (beast::system_error const&)
		{
		}
	}
}

void Server::stop()
{
	m_needExit = true;
}
