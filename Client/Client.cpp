#include "Client.hpp"

void Client::connect()
{
	tcp::resolver resolver{ m_ioc };

	m_pws = new websocket::stream<tcp::socket>{ m_ioc };
	if (!m_pws)
		throw std::exception("Can't create a websocket.");

	auto const results = resolver.resolve(m_host, m_port);
	auto ep = net::connect(m_pws->next_layer(), results);

	std::string host = m_host + ':' + std::to_string(ep.port());
	m_pws->handshake(host, "/");
}

void Client::disconnect()
{
	if (m_pws)
	{
		m_pws->close(websocket::close_code::normal);

		delete m_pws;
		m_pws = nullptr;
	}
}

Client::Client(std::string& host, std::string& port) :
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

			try
			{
				m_pws->write(net::buffer(std::string("Packet")));
			}
			catch (beast::system_error const&)
			{
				throw std::exception("Connection closed.");
			}

			nextTime = std::chrono::system_clock::now() + std::chrono::seconds(distr(dre));
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));

	} while (!m_needExit);
}

void Client::stop()
{
	m_needExit = true;
}

std::string Client::getStatistics()
{
	std::cout << "Getting statistics..." << std::endl;

	if (!m_pws)
		throw std::exception("Not connected.");

	m_pws->write(net::buffer(std::string("Statistics")));

	beast::flat_buffer buffer;
	m_pws->read(buffer);

	return beast::buffers_to_string(buffer.data());
}
