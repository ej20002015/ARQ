#include "redis_connection.h"

#include <ARQUtils/error.h>
#include <ARQUtils/logger.h>
#include <ARQCore/data_source_config.h>

namespace ARQ
{

RedisConnPool& RedisConnPool::inst()
{
	static RedisConnPool s_inst;
	return s_inst;
}

std::unique_ptr<sw::redis::Redis> RedisConnPool::getConn( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lock( m_mut );
	ConnQueue& connQueue = m_connQueues[dsh.data()];
	if( !connQueue.empty() )
	{
		std::unique_ptr<sw::redis::Redis> conn = std::move( connQueue.front() );
		connQueue.pop_front();
		return conn;
	}

	try
	{
		Log( Module::CLICKHOUSE ).info( "Creating new Redis connection for dsh [{}]", dsh );

		const DataSourceConfig config = DataSourceConfigManager::inst().get( dsh );
		const DataSourceConfig::ConnProps connProps = config.connPropsMap.at( "Main" );
		sw::redis::ConnectionOptions opts;
		opts.host = connProps.hostname;
		opts.port = connProps.port;

		auto newConn = std::make_unique<sw::redis::Redis>( opts );
		return newConn;
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "Cannot connect to redis DB for DSH [{}]: {}", dsh, e.what() ) );
	}
	catch( ... )
	{
		throw ARQException( std::format( "Cannot connect to redis DB for DSH [{}]: unknown exception occured", dsh ) );
	}
}

void RedisConnPool::retConn( const std::string_view dsh, std::unique_ptr<sw::redis::Redis> conn )
{
	if( conn )
	{
		std::lock_guard<std::mutex> lock( m_mut );
		m_connQueues[dsh.data()].push_back( std::move( conn ) );
	}
}

void RedisConnPool::closeAll()
{
	std::lock_guard<std::mutex> lock( m_mut );
	m_connQueues.clear();
}

}