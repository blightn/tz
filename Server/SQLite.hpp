#ifndef _SQLITE_H_
#define _SQLITE_H_

#include <string>
#include <exception>
#include <vector>
#include <variant>
#include <memory>

#include "..\sqlite3\sqlite3.h"

using DBVariants = std::variant<std::string, int64_t, double>;

enum class ColumnType { CT_INTEGER, CT_REAL, CT_TEXT };

class TableColumn
{
	std::string m_name;
	ColumnType  m_type;
	bool        m_primaryKey;
	bool        m_unique;
	bool        m_notNull;

public:
	TableColumn(const std::string& name, const ColumnType& type, bool primaryKey = false, bool unique = false, bool notNull = true) :
		m_name(name),
		m_type(type),
		m_primaryKey(primaryKey),
		m_unique(unique),
		m_notNull(notNull)
	{ }

	std::string name() const { return m_name;       };
	ColumnType type()  const { return m_type;       };
	bool primaryKey()  const { return m_primaryKey; };
	bool unique()      const { return m_unique;     };
	bool notNull()     const { return m_notNull;    };
};

class TableValue
{
	std::string m_columnName;
	DBVariants  m_value;

public:
	TableValue(const std::string& columnName, const DBVariants& value) :
		m_columnName(columnName),
		m_value(value)
	{ }

	std::string columnName()  const { return m_columnName; };
	const DBVariants& value() const { return m_value;      };
};

enum class ComparisonType { CT_LESSER, CT_GREATER, CT_EQUAL };

class WhereClause
{
	TableValue     m_value;
	ComparisonType m_type;

public:
	WhereClause(const TableValue& value, const ComparisonType& type) :
		m_value(value),
		m_type(type)
	{ }

	const TableValue& value() const { return m_value; };
	ComparisonType type()     const { return m_type;  };
};

enum class SortingOrder { SO_ASC, SO_DESC };

class OrderByClause
{
	std::string  m_columnName;
	SortingOrder m_order;

public:
	OrderByClause(const std::string& columnName, const SortingOrder& order) :
		m_columnName(columnName),
		m_order(order)
	{ }

	std::string columnName() const { return m_columnName; };
	SortingOrder order()     const { return m_order;      };
};

class SQLite
{
	sqlite3* m_psqlite3 = nullptr;

public:
	SQLite(const std::string& dbName);
	~SQLite();

	void createTable(const std::string& tableName, const std::vector<TableColumn>& tableColumns);
	void insertOne(const std::string& tableName, const std::vector<TableValue>& tableValues);
	std::unique_ptr<std::vector<TableValue>> selectOne(const std::string& tableName, const std::vector<TableColumn>& tableColumns,
		const WhereClause* pWhereClause = nullptr, const OrderByClause* pOrderByClause = nullptr);
	std::unique_ptr<std::vector<std::vector<TableValue>>> selectMany(const std::string& tableName, const std::vector<TableColumn>& tableColumns,
		const WhereClause* pWhereClause = nullptr, const OrderByClause* pOrderByClause = nullptr, size_t rowCount = -1);
};

#endif // _SQLITE_H_
