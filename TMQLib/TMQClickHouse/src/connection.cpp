#include "connection.h"

#include <TMQUtils/error.h>
#include <TMQCore/logger.h>
#include <TMQCore/data_source_config.h>

namespace TMQ
{

std::unique_ptr<clickhouse::Client> CHConnPool::getConn()
{
	std::scoped_lock<std::mutex> lock( m_mutex );
	if( !m_conns.empty() )
	{
		std::unique_ptr<clickhouse::Client> conn = std::move( m_conns.front() );
		m_conns.pop_front();
		return conn;
	}

	try
	{
		Log( Module::CLICKHOUSE ).info( "Creating new ClickHouse connection" );

		const DataSourceConfig config = DataSourceConfigManager::inst().get( "ClickHouseDB" );
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

void CHConnPool::retConn( std::unique_ptr<clickhouse::Client> conn )
{
	if( conn )
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		m_conns.push_back( std::move( conn ) );
	}
}

}
