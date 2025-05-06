#pragma once

#include <TMQUtils/buffer.h>
#include <TMQUtils/time.h>

#include <clickhouse/client.h>

#include <chrono>

namespace TMQ
{

// SELECT query type functions

template<typename T>
T extractColVal( const std::shared_ptr<clickhouse::Column>& column, size_t row )
{
    if constexpr( std::is_same_v<T, std::string> )
    {
        return std::string( column->As<clickhouse::ColumnString>()->At( row ) );
    }
    else if constexpr( std::is_same_v<T, Buffer> )
    {
        const auto sv = column->As<clickhouse::ColumnString>()->At( row );
        return Buffer( reinterpret_cast<const uint8_t*>( sv.data() ), sv.size() + 1 );
    }
    else if constexpr( std::is_same_v<T, uint64_t> )
    {
        return column->As<clickhouse::ColumnUInt64>()->At( row );
    }
    else if constexpr( std::is_same_v<T, std::chrono::system_clock::time_point> )
    {
        return Time::longToTp( static_cast<uint64_t>( column->As<clickhouse::ColumnDateTime64>()->At( row ) ) );
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
        static_assert( false, "Unsupported type in ClickHouse query." );
    }
}

// INSERT query type functions

template<typename T>
struct CHColType
{
    static_assert( false, "Unsupported type in ClickHouse query." );
};

template<>
struct CHColType<std::string>
{
    using Type = clickhouse::ColumnString;
};

template<>
struct CHColType<std::string_view>
{
    using Type = clickhouse::ColumnString;
};

template<>
struct CHColType<BufferView>
{
    using Type = clickhouse::ColumnString;
};

template<>
struct CHColType<Buffer>
{
    using Type = clickhouse::ColumnString;
};

template<>
struct CHColType<uint64_t>
{
    using Type = clickhouse::ColumnUInt64;
};

template<>
struct CHColType<std::chrono::system_clock::time_point>
{
    using Type = clickhouse::ColumnDateTime64;
};

template<>
struct CHColType<int32_t>
{
    using Type = clickhouse::ColumnInt32;
};

template<>
struct CHColType<bool>
{
    using Type = clickhouse::ColumnUInt8;
};

template<typename T>
clickhouse::ColumnRef createCol()
{
    if constexpr( std::is_same_v<T, std::chrono::system_clock::time_point> )
    {
        return std::make_shared<clickhouse::ColumnDateTime64>( 9 );
    }
    else
    {
        return std::make_shared<typename CHColType<T>::Type>();
    }
}

template<typename T>
constexpr T convToCHType( const T& value ) {
    return value;
}

inline uint64_t convToCHType( const std::chrono::system_clock::time_point& tp )
{
    return Time::tpToLong( tp );
}

inline std::string_view convToCHType( const BufferView& bv )
{
    return std::string_view( reinterpret_cast<const char*>( bv.data ), bv.size );
}

inline std::string_view convToCHType( const Buffer& bv )
{
    return std::string_view( reinterpret_cast<const char*>( bv.data.get() ), bv.size );
}

}