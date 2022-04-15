#include "SQLite.hpp"

SQLite::SQLite(const std::string& dbName)
{
	int res = sqlite3_open_v2(dbName.c_str(), &m_psqlite3, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
	if (res != SQLITE_OK)
	{
		throw std::exception("Can't create/open DB.");
	}
}

SQLite::~SQLite()
{
	if (m_psqlite3)
	{
		sqlite3_close_v2(m_psqlite3);
		m_psqlite3 = nullptr;
	}
}

void SQLite::createTable(const std::string& tableName, const std::vector<TableColumn>& columns)
{
	if (tableName.empty() || columns.empty())
	{
		throw std::exception("Invalid arguments.");
	}

	std::string query = "CREATE TABLE IF NOT EXISTS " + tableName + "(";

	bool first = true;
	for (const auto& c : columns)
	{
		std::string typeStr;

		switch (c.type())
		{
		case ColumnType::CT_INTEGER: typeStr = "INTEGER"; break;
		case ColumnType::CT_REAL:    typeStr = "REAL";    break;
		case ColumnType::CT_TEXT:    typeStr = "TEXT";    break;
		default:
			throw std::exception("Unknown column type.");
		}

		if (first)
		{
			first = false;
		}
		else
			query += ", ";

		query += c.name() + " " + typeStr;

		if (c.primaryKey()) query += " PRIMARY KEY";
		if (c.unique())     query += " UNIQUE";
		if (c.notNull())    query += " NOT NULL";
	}

	query += ");";

	char* errMsg = nullptr;
	int res = sqlite3_exec(m_psqlite3, query.c_str(), nullptr, nullptr, &errMsg);
	if (res != SQLITE_OK)
	{
		std::string text = "Can't CREATE table \"" + tableName + "\" with error: " + errMsg;

		sqlite3_free(errMsg);
		errMsg = nullptr;

		throw std::exception(text.c_str());
	}
}

void SQLite::insertOne(const std::string& tableName, const std::vector<TableValue>& values)
{
	if (tableName.empty() || values.empty())
	{
		throw std::exception("Invalid arguments.");
	}

	std::string query = "INSERT INTO " + tableName + "(";

	bool first = true;
	for (const auto& v : values)
	{
		if (first)
		{
			first = false;
		}
		else
			query += ", ";

		query += v.columnName();
	}

	query += ") VALUES(";

	first = true;
	for (const auto& v : values)
	{
		if (first)
		{
			first = false;
		}
		else
			query += ", ";

		if (v.value().type() == typeid(__int64))
		{
			query += std::to_string(std::any_cast<__int64>(v.value()));
		}
		else if (v.value().type() == typeid(double))
		{
			query += std::to_string(std::any_cast<double>(v.value()));
		}
		else if (v.value().type() == typeid(std::string))
		{
			query += "\"" + std::any_cast<std::string>(v.value()) + "\"";
		}
		else
			throw std::exception("Invalid type.");
	}

	query += ");";

	char* errMsg = nullptr;
	int res = sqlite3_exec(m_psqlite3, query.c_str(), nullptr, nullptr, &errMsg);
	if (res != SQLITE_OK)
	{
		std::string text = "Can't INSERT into table \"" + tableName + "\" with error: " + errMsg;

		sqlite3_free(errMsg);
		errMsg = nullptr;

		throw std::exception(text.c_str());
	}
}

std::unique_ptr<std::vector<TableValue>> SQLite::selectOne(const std::string& tableName, const std::vector<TableColumn>& columns,
	const WhereClause* pWhereClause, const OrderByClause* pOrderByClause)
{
	std::unique_ptr<std::vector<std::vector<TableValue>>> rows = selectMany(tableName, columns, pWhereClause, pOrderByClause, 1);
	std::unique_ptr<std::vector<TableValue>> row = std::make_unique<std::vector<TableValue>>();

	if (!rows->empty())
	{
		*row = rows->at(0);
	}

	return row;
}

std::unique_ptr<std::vector<std::vector<TableValue>>> SQLite::selectMany(const std::string& tableName, const std::vector<TableColumn>& columns,
	const WhereClause* pWhereClause, const OrderByClause* pOrderByClause, size_t rowCount)
{
	if (tableName.empty() || columns.empty())
	{
		throw std::exception("Invalid arguments.");
	}

	std::string query = "SELECT ";

	bool first = true;
	for (const auto& c : columns)
	{
		if (first)
		{
			first = false;
		}
		else
			query += ", ";

		query += c.name();
	}

	query += " FROM " + tableName;

	if (pWhereClause)
	{
		query += " WHERE " + pWhereClause->value().columnName() + " ";

		switch (pWhereClause->type())
		{
		case ComparisonType::CT_LESSER:  query += "<"; break;
		case ComparisonType::CT_GREATER: query += ">"; break;
		case ComparisonType::CT_EQUAL:   query += "="; break;
		default:
			throw std::exception("Unknown comparison type.");
		}

		query += " ";

		if (pWhereClause->value().value().type() == typeid(__int64))
		{
			query += std::to_string(std::any_cast<__int64>(pWhereClause->value().value()));
		}
		else if (pWhereClause->value().value().type() == typeid(double))
		{
			query += std::to_string(std::any_cast<double>(pWhereClause->value().value()));
		}
		else if (pWhereClause->value().value().type() == typeid(std::string))
		{
			query += "\"" + std::any_cast<std::string>(pWhereClause->value().value()) + "\"";
		}
		else
			throw std::exception("Invalid type.");
	}

	if (pOrderByClause)
	{
		query += " ORDER BY " + pOrderByClause->columnName() + " ";

		if (pOrderByClause->order() == SortingOrder::SO_ASC)
		{
			query += "ASC";
		}
		else if (pOrderByClause->order() == SortingOrder::SO_DESC)
		{
			query += "DESC";
		}
		else
			throw std::exception("Invalid sorting order.");
	}

	query += ";";

	sqlite3_stmt* pstmt = nullptr;
	int res = sqlite3_prepare_v2(m_psqlite3, query.c_str(), -1, &pstmt, nullptr);
	if (res != SQLITE_OK)
	{
		std::string text = "Can't SELECT from table \"" + tableName + "\".";
		throw std::exception(text.c_str());
	}

	std::unique_ptr<std::vector<std::vector<TableValue>>> rows = std::make_unique<std::vector<std::vector<TableValue>>>();

	while (rowCount && sqlite3_step(pstmt) == SQLITE_ROW)
	{
		int i = 0;
		std::vector<TableValue> row;

		for (const auto& c : columns)
		{
			switch (c.type())
			{
			case ColumnType::CT_INTEGER: row.push_back(TableValue(c.name(), sqlite3_column_int64(pstmt, i)));  break;
			case ColumnType::CT_REAL:    row.push_back(TableValue(c.name(), sqlite3_column_double(pstmt, i))); break;
			case ColumnType::CT_TEXT:    row.push_back(TableValue(c.name(), static_cast<std::string>((const char*)sqlite3_column_text(pstmt, i)))); break;
			}

			++i;
		}

		rows->push_back(row);
		--rowCount;
	}

	sqlite3_finalize(pstmt);
	pstmt = nullptr;

	return rows;
}
