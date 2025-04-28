#include "query.h"

#include <TMQUtils/error.h>
#include <TMQUtils/buffer.h>
#include <TMQUtils/instr.h>
#include <TMQCore/logger.h>

#include "types.h"
#include "connection.h"

#include <clickhouse/client.h>

#include <array>

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
    return extractRowImpl<Tuple>( block, row, std::make_index_sequence<std::tuple_size_v<Tuple>>() );
}

template<c_QuerySchema Schema>
QueryResult<Schema> CHQuery::select( const std::string_view query )
{
    CHConn conn;

    QueryResult<Schema> result;

    Instr::Timer tm;

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

    Log( Module::CLICKHOUSE ).debug( "Ran Clickhouse select query in {}", tm.duration() );

    return result;
}

template<typename Tuple, size_t... Indices>
void prepareColsImpl( std::array<clickhouse::ColumnRef, std::tuple_size_v<Tuple>>& colsArr, std::index_sequence<Indices...> )
{
    ( 
        ( colsArr[Indices] = std::make_shared<typename CHColType<std::tuple_element_t<Indices, Tuple>>::Type>() ),
        ...
    );
}

template<typename Tuple>
void prepareCols( std::array<clickhouse::ColumnRef, std::tuple_size_v<Tuple>>& colsArr )
{
    prepareColsImpl<Tuple>( colsArr, std::make_index_sequence<std::tuple_size_v<Tuple>>() );
}

template<typename Tuple, size_t... Indices>
void insertRowImpl( std::array<clickhouse::ColumnRef, std::tuple_size_v<Tuple>>& colsArr, const Tuple& row, std::index_sequence<Indices...> )
{
    ( 
        ( static_cast<typename CHColType<std::tuple_element_t<Indices, Tuple>>::Type*>( colsArr[Indices].get() )->Append( convToCHType( std::get<Indices>( row ) ) ) ),
        ... 
    );
}

template<typename Tuple>
void insertRow( std::array<clickhouse::ColumnRef, std::tuple_size_v<Tuple>>& colsArr, const Tuple& row )
{
    insertRowImpl<Tuple>( colsArr, row, std::make_index_sequence<std::tuple_size_v<Tuple>>() );
}

template<typename Tuple, size_t... Indices>
void appendColsImpl( std::array<clickhouse::ColumnRef, std::tuple_size_v<Tuple>>& colsArr, const std::array<std::string_view, std::tuple_size_v<Tuple>>& colNames, clickhouse::Block& block, std::index_sequence<Indices...> )
{
    (
        ( block.AppendColumn( colNames[Indices].data(), colsArr[Indices]) ),
        ...
    );
}

template<typename Tuple>
void appendCols( std::array<clickhouse::ColumnRef, std::tuple_size_v<Tuple>>& colsArr, const std::array<std::string_view, std::tuple_size_v<Tuple>>& colNames, clickhouse::Block& block )
{
    appendColsImpl<Tuple>( colsArr, colNames, block, std::make_index_sequence<std::tuple_size_v<Tuple>>() );
}

template<c_QuerySchema Schema>
void CHQuery::insert( const std::string_view tableName, 
                      const std::vector<typename Schema::TupleType>& data,
                      const std::array<std::string_view, std::tuple_size_v<typename Schema::TupleType>>& colNames )
{
    CHConn conn;

    Instr::Timer tm;

    try
    {
        using Tuple = typename Schema::TupleType;
        using ColsArr = std::array<clickhouse::ColumnRef, std::tuple_size_v<typename Schema::TupleType>>;

        ColsArr colsArr;
        prepareCols<Tuple>( colsArr );

        for( const auto& row : data )
            insertRow<Tuple>( colsArr, row );

        clickhouse::Block block;
        appendCols<Tuple>( colsArr, colNames, block );

        conn.client().Insert( tableName.data(), block );
    }
    catch( const std::exception& e )
    {
        throw TMQException( std::format( "Error executing INSERT query: {0}", e.what() ) );
    }

    Log( Module::CLICKHOUSE ).debug( "Ran Clickhouse insert query in {}", tm.duration() );
}


void CHQuery::execute( const std::string_view query )
{
    CHConn conn;

    Instr::Timer tm;

    try
    {
        conn.client().Execute( query.data() );
    }
    catch( const std::exception& e )
    {
        throw TMQException( std::format( "Error executing query: {0}", e.what() ) );
    }

    Log( Module::CLICKHOUSE ).debug( "Ran Clickhouse query in {}", tm.duration() );
}

// Explicit template instantiations for all possible QuerySchemas go here


using StringSchema = QuerySchema<std::string>;
template QueryResult<StringSchema> CHQuery::select<StringSchema>( const std::string_view query );

using RDFetchSchema = QuerySchema<std::chrono::system_clock::time_point, std::string, Buffer>;
template QueryResult<RDFetchSchema> CHQuery::select<RDFetchSchema>( const std::string_view query );

using RDInsertSchema = QuerySchema<std::string_view, std::string_view, bool, BufferView>;
template void CHQuery::insert<RDInsertSchema>( const std::string_view tableName, const std::vector<typename RDInsertSchema::TupleType>& data, const std::array<std::string_view, std::tuple_size_v<typename RDInsertSchema::TupleType>>& colNames );

using MDFetchSchema = QuerySchema<
    std::string,
    std::string,
    std::chrono::system_clock::time_point,
    Buffer,
    std::string,
    std::chrono::system_clock::time_point,
    std::string,
    bool>;
template QueryResult<MDFetchSchema> CHQuery::select<MDFetchSchema>( const std::string_view query );

}