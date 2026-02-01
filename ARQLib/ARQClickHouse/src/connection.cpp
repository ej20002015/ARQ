#include "connection.h"

#include <ARQUtils/error.h>
#include <ARQUtils/logger.h>
#include <ARQCore/data_source_config.h>

namespace ARQ
{

std::unique_ptr<clickhouse::Client> CHConnPool::getConn( const std::string_view dsh )
{
	std::scoped_lock<std::mutex> lock( m_mut );
	ConnQueue& connQueue = m_connQueues[dsh.data()];
	if( !connQueue.empty() )
	{
		std::unique_ptr<clickhouse::Client> conn = std::move( connQueue.front() );
		connQueue.pop_front();
		return conn;
	}

	try
	{
		Log( Module::CLICKHOUSE ).info( "Creating new ClickHouse connection for dsh [{}]", dsh );

		const DataSourceConfig config = DataSourceConfigManager::inst().get( dsh );
		const DataSourceConfig::ConnProps connProps = config.connPropsMap.at( "Main" );
		clickhouse::ClientOptions opt = {
			.host = connProps.hostname,
			.port = connProps.port
		};

		auto newConn = std::make_unique<clickhouse::Client>( opt );
		return newConn;
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "Cannot connect to clickhouse DB: {}", e.what() ) );
	}
	catch( ... )
	{
		throw ARQException( std::format( "Cannot connect to clickhouse DB: unknown exception occured" ) );
	}
}

void CHConnPool::retConn( const std::string_view dsh, std::unique_ptr<clickhouse::Client> conn )
{
	if( conn )
	{
		std::lock_guard<std::mutex> lock( m_mut );
		m_connQueues[dsh.data()].push_back(std::move(conn));
	}
}

}
