#include <TMQClickHouse/connection.h>

#include <TMQUtils/error.h>

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
		auto newConn = std::make_unique<clickhouse::Client>( clickhouse::ClientOptions().SetHost( "localhost" ) ); // TODO: Can't always be using localhost - get from datasources ref table? If so will need to bootstrap main server
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
