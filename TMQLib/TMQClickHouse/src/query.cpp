#include <TMQClickHouse/query.h>

#include <TMQClickHouse/connection.h>
#include <TMQClickHouse/types.h>
#include <TMQUtils/error.h>

#include <clickhouse/client.h>

namespace TMQ
{

template<typename Tuple, size_t... Indices>
Tuple extractRowImpl( const clickhouse::Block& block, size_t row, std::index_sequence<Indices...> )
{
    return std::make_tuple( extractColVal<std::tuple_element_t<Indices, Tuple>>( block[Indices], row )... );
}

template<typename Tuple>
Tuple extractRow( const clickhouse::Block& block, size_t row )
{
    return extractRowImpl<Tuple>( block, row, std::make_index_sequence<std::tuple_size_v<Tuple>>{} );
}

template<c_QuerySchema Schema>
QueryResult<Schema> CHQuery::select( const std::string_view query )
{
    CHConn conn;

    QueryResult<Schema> result;

    try
    {
        conn.client().Select( std::string( query ), [&] ( const clickhouse::Block& block )
        {
            if( block.GetRowCount() == 0 )
                return; // End of data

            static constexpr size_t NUM_EXPECTED_COLS = std::tuple_size_v<typename Schema::TupleType>;
            if( block.GetColumnCount() != NUM_EXPECTED_COLS )
                throw TMQException( std::format( "Column count mismatch between query [{0}] and expected types [{1}]", block.GetColumnCount(), NUM_EXPECTED_COLS ) );

            for( size_t row = 0; row < block.GetRowCount(); ++row )
                result.push_back( extractRow<typename Schema::TupleType>( block, row ) );
        } );
    }
    catch( const std::exception& e )
    {
        throw TMQException( std::format( "Error executing SELECT query: {0}", e.what() ) );
    }

    return result;
}

void CHQuery::execute( const std::string_view query )
{
    CHConn conn;

    try
    {
        conn.client().Execute( query.data() );
    }
    catch( const std::exception& e )
    {
        throw TMQException( std::format( "Error executing query: {0}", e.what() ) );
    }
}

// Explicit template instantiations for all possible QuerySchemas go here

template TMQ_API QueryResult<QuerySchema<std::string>> CHQuery::select<QuerySchema<std::string>>( const std::string_view query );

}