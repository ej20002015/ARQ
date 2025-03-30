#pragma once
#include <TMQClickHouse/dll.h>

#include <clickhouse/client.h>

#include <TMQUtils/error.h>

#include <queue>
#include <mutex>
#include <memory>

namespace TMQ
{

class CHQuery;

class CHConnPool
{
public:
	static CHConnPool& inst()
	{
		static CHConnPool inst;
		return inst;
	}

	TMQClickHouse_API std::unique_ptr<clickhouse::Client> getConn();

	TMQClickHouse_API void retConn( std::unique_ptr<clickhouse::Client> conn );

private:
	CHConnPool() = default;

private:
	std::deque<std::unique_ptr<clickhouse::Client>> m_conns;
	std::mutex m_mutex;
};

class CHConn
{
public:
	CHConn()
		: m_client( CHConnPool::inst().getConn() )
	{}
	
	~CHConn()
	{
		if( m_client )
			CHConnPool::inst().retConn( std::move( m_client ) );
	}

private:
	      clickhouse::Client& client()       { return *m_client; }
	const clickhouse::Client& client() const { return *m_client; }

	friend class CHQuery;

private:
	std::unique_ptr<clickhouse::Client> m_client;
};

}