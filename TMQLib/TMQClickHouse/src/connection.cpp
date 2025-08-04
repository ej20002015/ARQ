#include "connection.h"

#include <TMQUtils/error.h>
#include <TMQCore/logger.h>
#include <TMQCore/data_source_config.h>

namespace TMQ
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
		Log( Module::CLICKHOUSE ).info( "Creating new ClickHouse connection for dsh []", dsh );

		const DataSourceConfig config = DataSourceConfigManager::inst().get( dsh );
		clickhouse::ClientOptions opt = {
			.host = config.hostname,
			.port = config.port
		};

		auto newConn = std::make_unique<clickhouse::Client>( opt );
		return newConn;
	}
	catch( const std::exception& e )
	{
		throw TMQException( std::format( "Cannot connect to clickhouse DB: {0}", e.what() ) );
	}
	catch( ... )
	{
		throw TMQException( std::format( "Cannot connect to clickhouse DB: unknown exception occured" ) );
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
