#include "Client.hpp"

void Client::connect()
{
	m_pws = std::make_unique<websocket::stream<tcp::socket>>(m_ioc);

	tcp::resolver resolver{ m_ioc };
	auto const results = resolver.resolve(m_host, m_port);
	auto ep = net::connect(m_pws->next_layer(), results);

	std::string host = m_host + ':' + std::to_string(ep.port());
	m_pws->handshake(host, "/");
}

void Client::disconnect()
{
	if (m_pws && m_pws->is_open())
	{
		m_pws->close(websocket::close_code::normal);
	}
}

Client::Client(std::string& host, std::string& port) :
	m_host(host),
	m_port(port)
{
	m_uuid = boost::uuids::random_generator()();

	try
	{
		connect();
	}
	catch (...)
	{
		throw std::exception("Can't connect.");
	}
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
	std::uniform_int_distribution<int> ig(Client::INTERVAL_MIN, Client::INTERVAL_MAX);
	std::uniform_real_distribution<double> dg(Client::RANDOM_REAL_MIN, Client::RANDOM_REAL_MAX);

	auto nextTime = std::chrono::system_clock::now() + std::chrono::seconds(ig(dre));

	do
	{	
		if (std::chrono::system_clock::now() >= nextTime)
		{
			std::cout << "Sending a packet..." << std::endl;

			tz::ClientPacket::Data data;
			data.set_uuid(to_string(m_uuid));
			data.set_timestamp(std::chrono::system_clock::now().time_since_epoch().count());
			data.set_x(dg(dre));
			data.set_y(dg(dre));

			tz::ClientPacket packet;
			packet.set_type(tz::ClientPacket::DATA);
			packet.set_allocated_data(&data);

			try
			{
				m_pws->write(net::buffer(packet.SerializeAsString()));
			}
			catch (beast::system_error const&)
			{
				throw std::exception("Connection closed.");
			}

			nextTime = std::chrono::system_clock::now() + std::chrono::seconds(ig(dre));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(200));

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

	tz::ClientPacket packet;
	packet.set_type(tz::ClientPacket::STATISTICS);
	packet.set_allocated_data(nullptr);

	m_pws->write(net::buffer(packet.SerializeAsString()));

	beast::flat_buffer buffer;
	m_pws->read(buffer);

	tz::ServerStatistic stats;
	stats.ParseFromString(beast::buffers_to_string(buffer.data()));

	std::string tmp = "UUID X_1 Y_1 X_5 Y_5\n";
	for (int i = 0; i < stats.client_size(); ++i)
	{
		if (i)
			tmp += "\n";

		auto client = stats.client(i);
		tmp += client.uuid() + " " +
			std::to_string(client.x1()) + " " +
			std::to_string(client.y1()) + " " +
			std::to_string(client.x5()) + " " +
			std::to_string(client.y5());
	}

	return tmp;
}
