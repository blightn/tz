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

void SQLite::createTable(const std::string& tableName, const std::vector<TableColumn>& tableColumns)
{
	if (tableName.empty() || tableColumns.empty())
	{
		throw std::exception("Invalid arguments.");
	}

	std::string query = "CREATE TABLE IF NOT EXISTS " + tableName + "(";

	bool first = true;
	for (const auto& tableColumn : tableColumns)
	{
		std::string typeStr;

		switch (tableColumn.type())
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

		query += tableColumn.name() + " " + typeStr;

		if (tableColumn.primaryKey()) query += " PRIMARY KEY";
		if (tableColumn.unique())     query += " UNIQUE";
		if (tableColumn.notNull())    query += " NOT NULL";
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

void SQLite::insertOne(const std::string& tableName, const std::vector<TableValue>& tableValues)
{
	if (tableName.empty() || tableValues.empty())
	{
		throw std::exception("Invalid arguments.");
	}

	std::string query = "INSERT INTO " + tableName + "(";

	bool first = true;
	for (const auto& tableValue : tableValues)
	{
		if (first)
		{
			first = false;
		}
		else
			query += ", ";

		query += tableValue.columnName();
	}

	query += ") VALUES(";

	first = true;
	for (const auto& tableValue : tableValues)
	{
		if (first)
		{
			first = false;
		}
		else
			query += ", ";

	sqlite3_stmt* pstmt = nullptr;

	try
	{
		int res = sqlite3_prepare_v2(m_psqlite3, query.c_str(), -1, &pstmt, nullptr);
		if (res != SQLITE_OK)
		{
			std::string text = "sqlite3_prepare_v2() ERROR: " + std::to_string(res);
			throw std::exception(text.c_str());
		}

		int index = 1;
		for (const auto& tableValue : tableValues)
		{
			auto& cellValue = tableValue.value();

			if (std::holds_alternative<std::string>(cellValue))
			{
				res = sqlite3_bind_text(pstmt, index, std::get<std::string>(cellValue).c_str(), -1, nullptr);
			}
			else if (std::holds_alternative<int64_t>(cellValue))
			{
				res = sqlite3_bind_int64(pstmt, index, std::get<int64_t>(cellValue));
			}
			else if (std::holds_alternative<double>(cellValue))
			{
				res = sqlite3_bind_double(pstmt, index, std::get<double>(cellValue));
			}
			else
			{
				std::string text = "Invalid type for column: " + tableValue.columnName();
				throw std::exception(text.c_str());
			}

			if (res != SQLITE_OK)
			{
				std::string text = "sqlite3_bind_*() ERROR: " + std::to_string(res);
				throw std::exception(text.c_str());
			}

			++index;
		}

		res = 0;
		if ((res = sqlite3_step(pstmt)) != SQLITE_DONE)
		{
			std::string text = "sqlite3_step() ERROR: " + std::to_string(res);
			throw std::exception(text.c_str());
		}
	}
	catch (const std::exception& ex)
	{
		if (pstmt)
		{
			sqlite3_finalize(pstmt);
			pstmt = nullptr;
		}

		std::string text = "Can't INSERT into table \"" + tableName + "\": " + ex.what() + ".";
		throw std::exception(text.c_str());
	}

	sqlite3_finalize(pstmt);
	pstmt = nullptr;
}

std::unique_ptr<std::vector<TableValue>> SQLite::selectOne(const std::string& tableName, const std::vector<TableColumn>& tableColumns,
	const WhereClause* pWhereClause, const OrderByClause* pOrderByClause)
{
	std::unique_ptr<std::vector<std::vector<TableValue>>> rows;

	try
	{
		rows = selectMany(tableName, tableColumns, pWhereClause, pOrderByClause, 1);
	}
	catch (const std::exception& ex)
	{
		std::string text = "Can't SELECT from table \"" + tableName + "\": " + ex.what() + ".";
		throw std::exception(text.c_str());
	}

	std::unique_ptr<std::vector<TableValue>> row = std::make_unique<std::vector<TableValue>>();

	if (!rows->empty())
	{
		*row = rows->at(0);
	}

	return row;
}

std::unique_ptr<std::vector<std::vector<TableValue>>> SQLite::selectMany(const std::string& tableName, const std::vector<TableColumn>& tableColumns,
	const WhereClause* pWhereClause, const OrderByClause* pOrderByClause, size_t rowCount)
{
	if (tableName.empty() || tableColumns.empty())
	{
		throw std::exception("Invalid arguments.");
	}

	std::string query = "SELECT ";

	bool first = true;
	for (const auto& tableColumn : tableColumns)
	{
		if (first)
		{
			first = false;
		}
		else
			query += ", ";

		query += tableColumn.name();
	}

	query += " FROM " + tableName;

	if (pWhereClause)
	{
		query += " WHERE " + pWhereClause->tableValue().columnName() + " ";

		switch (pWhereClause->type())
		{
		case ComparisonType::CT_LESSER:  query += "<"; break;
		case ComparisonType::CT_GREATER: query += ">"; break;
		case ComparisonType::CT_EQUAL:   query += "="; break;
		default:
			throw std::exception("Unknown comparison type.");
		}

		query += " ?";
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
	std::unique_ptr<std::vector<std::vector<TableValue>>> rows = std::make_unique<std::vector<std::vector<TableValue>>>();

	try
	{
		int res = sqlite3_prepare_v2(m_psqlite3, query.c_str(), -1, &pstmt, nullptr);
		if (res != SQLITE_OK)
		{
			std::string text = "sqlite3_prepare_v2() ERROR: " + res;
			throw std::exception(text.c_str());
		}

		if (pWhereClause)
		{
			auto& cellValue = pWhereClause->tableValue().value();

			if (std::holds_alternative<std::string>(cellValue))
			{
				res = sqlite3_bind_text(pstmt, 1, std::get<std::string>(cellValue).c_str(), -1, nullptr);
			}
			else if (std::holds_alternative<int64_t>(cellValue))
			{
				res = sqlite3_bind_int64(pstmt, 1, std::get<int64_t>(cellValue));
			}
			else if (std::holds_alternative<double>(cellValue))
			{
				res = sqlite3_bind_double(pstmt, 1, std::get<double>(cellValue));
			}
			else
			{
				std::string text = "Invalid type for column: " + pWhereClause->tableValue().columnName();
				throw std::exception(text.c_str());
			}

			if (res != SQLITE_OK)
			{
				std::string text = "sqlite3_bind_*() ERROR: " + std::to_string(res);
				throw std::exception(text.c_str());
			}
		}

		while (rowCount && sqlite3_step(pstmt) == SQLITE_ROW)
		{
			int i = 0;
			std::vector<TableValue> row;

			for (const auto& tableColumn : tableColumns)
			{
				switch (tableColumn.type())
				{
				case ColumnType::CT_INTEGER: row.push_back(TableValue(tableColumn.name(), sqlite3_column_int64(pstmt, i)));  break;
				case ColumnType::CT_REAL:    row.push_back(TableValue(tableColumn.name(), sqlite3_column_double(pstmt, i))); break;
				case ColumnType::CT_TEXT:    row.push_back(TableValue(tableColumn.name(), static_cast<std::string>((const char*)sqlite3_column_text(pstmt, i)))); break;
				}

				++i;
			}

			rows->push_back(row);
			--rowCount;
		}
	}
	catch (const std::exception&)
	{
		if (pstmt)
		{
			sqlite3_finalize(pstmt);
			pstmt = nullptr;
		}

		throw;
	}

	sqlite3_finalize(pstmt);
	pstmt = nullptr;

	return rows;
}
