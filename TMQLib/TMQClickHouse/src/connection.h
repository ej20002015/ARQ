#pragma once

#include <clickhouse/client.h>

#include <TMQUtils/hashers.h>
#include <TMQCore/data_source_config.h>

#include <unordered_map>
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

	[[nodiscard]] std::unique_ptr<clickhouse::Client> getConn( const std::string_view dsh );

	void retConn( const std::string_view dsh, std::unique_ptr<clickhouse::Client> conn );

private:
	CHConnPool() = default;

private:

	using ConnQueue = std::deque<std::unique_ptr<clickhouse::Client>>;

	std::unordered_map<std::string, ConnQueue> m_connQueues;
	std::mutex m_mut;
};

class CHConn
{
public:
	CHConn( const std::string_view dsh )
		: m_dsh( dsh )
		, m_client( CHConnPool::inst().getConn( dsh ) )
	{}
	
	~CHConn()
	{
		if( m_client )
			CHConnPool::inst().retConn( m_dsh, std::move( m_client ) );
	}

	      clickhouse::Client& client()       { return *m_client; }
	const clickhouse::Client& client() const { return *m_client; }

private:
	std::string m_dsh;
	std::unique_ptr<clickhouse::Client> m_client;
};

}