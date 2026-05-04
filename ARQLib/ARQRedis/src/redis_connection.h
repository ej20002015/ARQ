#pragma once

#include <sw/redis++/redis++.h>

#include <unordered_map>
#include <queue>
#include <mutex>
#include <memory>

namespace ARQ
{

class RedisConnPool
{
public:
	static RedisConnPool& inst();

	[[nodiscard]] std::unique_ptr<sw::redis::Redis> getConn( const std::string_view dsh );

	void retConn( const std::string_view dsh, std::unique_ptr<sw::redis::Redis> conn );

	void closeAll();

private:
	RedisConnPool() = default;

private:
	using ConnQueue = std::deque<std::unique_ptr<sw::redis::Redis>>;

	std::unordered_map<std::string, ConnQueue> m_connQueues;
	std::mutex m_mut;
};

class RedisConn
{
public:
	RedisConn( const std::string_view dsh )
		: m_dsh( dsh )
		, m_client( RedisConnPool::inst().getConn( dsh ) )
	{
	}

	~RedisConn()
	{
		if( m_client )
			RedisConnPool::inst().retConn( m_dsh, std::move( m_client ) );
	}

	sw::redis::Redis& client() { return *m_client; }
	const sw::redis::Redis& client() const { return *m_client; }

private:
	std::string m_dsh;
	std::unique_ptr<sw::redis::Redis> m_client;
};

}