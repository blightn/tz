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
				std::cout << "Packet received: "
					      << packet.data().uuid()                      << " "
					      << std::to_string(packet.data().timestamp()) << " "
					      << std::to_string(packet.data().x())         << " "
					      << std::to_string(packet.data().y())         << std::endl;
				saveClientPacket(packet);
				break;

			case tz::ClientPacket::STATISTICS:
				ws.write(net::buffer(collectStatistics()->SerializeAsString()));
				std::cout << "Statistics sent." << std::endl;
				break;

			default:
				std::cout << "Unknown packet type. Ignoring..." << std::endl;
			}
		}
	}
	catch (const beast::system_error&)
	{
		std::cerr << "Connection closed." << std::endl;
	}
	catch (const std::bad_variant_access&)
	{
		std::cerr << "Invalid type from the DB." << std::endl;
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	std::cout << "Client disconnected." << std::endl;
}

void Server::saveClientPacket(const tz::ClientPacket& packet)
{
	if (packet.has_data())
	{
		auto& data = packet.data();

		std::vector<TableColumn> columns
		{
			TableColumn(CLIENTS_COLUMN_ID, ColumnType::CT_INTEGER, true ),
		};
		WhereClause whereClause(TableValue(CLIENTS_COLUMN_UUID, data.uuid()), ComparisonType::CT_EQUAL);

		try
		{
			auto client = m_psqlite3->selectOne(Server::CLIENTS_TABLE_NAME, columns, &whereClause);

			if (client.empty())
			{
				std::vector<TableValue> values
				{
					TableValue(CLIENTS_COLUMN_UUID, data.uuid()),
				};
				m_psqlite3->insertOne(Server::CLIENTS_TABLE_NAME, values);

				client = m_psqlite3->selectOne(Server::CLIENTS_TABLE_NAME, columns, &whereClause);
			}

			std::vector<TableValue> values
			{
				TableValue(PACKETS_COLUMN_CLIENT_ID, client.at(0).value()),
				TableValue(PACKETS_COLUMN_TIMESTAMP, data.timestamp()    ),
				TableValue(PACKETS_COLUMN_X,         data.x()            ),
				TableValue(PACKETS_COLUMN_Y,         data.y()            ),
			};
			m_psqlite3->insertOne(Server::PACKETS_TABLE_NAME, values);
		}
		catch (const std::exception& ex)
		{
			std::string text = "Can't save packet: " + std::string(ex.what());
			throw std::exception(text.c_str());
		}
	}
}

/*
	Yes, I know about SQL JOINS, AVG(), ABS() and so on :)
	For example: SELECT c.uuid, AVG(p.x) x1, SUM(ABS(p.y)) y1 FROM clients c LEFT JOIN packets p ON c.id=p.client_id GROUP BY c.uuid HAVING p.timestamp > 123;
*/
std::unique_ptr<tz::ServerStatistic> Server::collectStatistics()
{
	auto stats = std::make_unique<tz::ServerStatistic>();

	std::vector<TableColumn> columns
	{
		TableColumn(CLIENTS_COLUMN_ID,   ColumnType::CT_INTEGER, true       ),
		TableColumn(CLIENTS_COLUMN_UUID, ColumnType::CT_TEXT,    false, true),
	};

	auto clients = m_psqlite3->selectMany(Server::CLIENTS_TABLE_NAME, columns);

	auto currentTime = std::chrono::system_clock::now();
	auto interval1 = (currentTime - std::chrono::minutes(1)).time_since_epoch().count(); // Last 1 minute.
	auto interval5 = (currentTime - std::chrono::minutes(5)).time_since_epoch().count(); // Last 5 minutes.

	for (const auto& client : clients)
	{
		int64_t clientId       = std::get<int64_t>    (client.at(0).value());
		std::string clientUuid = std::get<std::string>(client.at(1).value());

		std::vector<TableColumn> columns
		{
			TableColumn(PACKETS_COLUMN_TIMESTAMP, ColumnType::CT_INTEGER),
			TableColumn(PACKETS_COLUMN_X,         ColumnType::CT_REAL   ),
			TableColumn(PACKETS_COLUMN_Y,         ColumnType::CT_REAL   ),
		};
		WhereClause whereClause(TableValue(PACKETS_COLUMN_CLIENT_ID, clientId), ComparisonType::CT_EQUAL);
		auto packets = m_psqlite3->selectMany(Server::PACKETS_TABLE_NAME, columns, &whereClause);

		double sumX1 = 0;
		double sumX5 = 0;
		int countX1 = 0;
		int countX5 = 0;
		double y1 = 0;
		double y5 = 0;
		double avgX1 = 0;
		double avgX5 = 0;
		bool needToAdd1 = false;
		bool needToAdd5 = false;

		for (const auto& packet : packets)
		{
			int64_t timestamp = std::get<int64_t>(packet.at(0).value());
			double x          = std::get<double> (packet.at(1).value());
			double y          = std::get<double> (packet.at(2).value());

			if (timestamp >= interval1)
			{
				sumX1 += x;
				++countX1;

				y1 += abs(y);

				needToAdd1 = true;
			}

			if (timestamp >= interval5)
			{
				sumX5 += x;
				++countX5;

				y5 += abs(y);

				needToAdd5 = true;
			}
		}

		if (needToAdd1)
		{
			avgX1 = sumX1 / countX1;
		}

		if (needToAdd5)
		{
			avgX5 = sumX5 / countX5;
		}

		if (needToAdd1 || needToAdd5)
		{
			auto client = stats->add_client();
			client->set_uuid(clientUuid);
			client->set_x1(avgX1);
			client->set_y1(y1);
			client->set_x5(avgX5);
			client->set_y5(y5);
		}
	}

	return stats;
}

Server::Server(const std::string& port) :
	m_port(port),
	m_psqlite3(std::make_unique<SQLite>(Server::DB_NAME))
{
	std::vector<TableColumn> columns
	{
		TableColumn(CLIENTS_COLUMN_ID,   ColumnType::CT_INTEGER, true       ),
		TableColumn(CLIENTS_COLUMN_UUID, ColumnType::CT_TEXT,    false, true),
	};
	m_psqlite3->createTable(Server::CLIENTS_TABLE_NAME, columns);

	columns.assign(
	{
		TableColumn(PACKETS_COLUMN_ID,        ColumnType::CT_INTEGER, true),
		TableColumn(PACKETS_COLUMN_CLIENT_ID, ColumnType::CT_INTEGER      ),
		TableColumn(PACKETS_COLUMN_TIMESTAMP, ColumnType::CT_INTEGER      ),
		TableColumn(PACKETS_COLUMN_X,         ColumnType::CT_REAL         ),
		TableColumn(PACKETS_COLUMN_Y,         ColumnType::CT_REAL         ),
	});
	m_psqlite3->createTable(Server::PACKETS_TABLE_NAME, columns);
}

void Server::start()
{
	auto const address = net::ip::make_address(BIND_IP_ADDRESS);
	auto const port    = static_cast<unsigned short>(std::atoi(m_port.c_str()));

	tcp::acceptor acceptor{ m_ioc, {address, port} };
	acceptor.non_blocking(true);

	std::cout << "Server started." << std::endl;

	while (!m_needExit)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		try
		{
			tcp::socket socket{ m_ioc };
			acceptor.accept(socket);

			std::cout << "Client connected from " << socket.local_endpoint().address() << ":" << socket.local_endpoint().port() << std::endl;

			m_threads.emplace_back(std::thread(&Server::clientThread, this, std::move(socket)));
		}
		catch (const beast::system_error&)
		{
		}
	}

	for (auto& thread : m_threads)
	{
		thread.join();
	}

	std::cout << "Server stopped." << std::endl;
}

void Server::stop()
{
	m_needExit = true;
}
