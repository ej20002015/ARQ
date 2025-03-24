#pragma once

#include <clickhouse/client.h>

namespace TMQ
{

template<typename T>
T extractColVal( std::shared_ptr<clickhouse::Column> column, size_t row )
{
    if constexpr( std::is_same_v<T, std::string> )
    {
        return std::string( column->As<clickhouse::ColumnString>()->At( row ) );
    }
    else if constexpr( std::is_same_v<T, uint64_t> )
    {
        return column->As<clickhouse::ColumnUInt64>()->At( row );
    }
    else if constexpr( std::is_same_v<T, int32_t> )
    {
        return column->As<clickhouse::ColumnInt32>()->At( row );
    }
    else if constexpr( std::is_same_v<T, bool> )
    {
        return static_cast< bool >( column->As<clickhouse::ColumnUInt8>()->At( row ) );
    }
    else
    {
        static_assert( !std::is_same_v<T, T>, "Unsupported type in ClickHouse query." );
    }
}

}