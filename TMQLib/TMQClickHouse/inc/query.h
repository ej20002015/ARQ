#pragma once
#include <TMQClickHouse/dll.h>

#include <tuple>
#include <vector>
#include <string>
#include <array>

namespace TMQ
{

template<typename... Ts>
struct QuerySchema
{
    using TupleType = std::tuple<Ts...>;
};

template<typename T>
concept c_QuerySchema = requires { typename T::TupleType; };

template<c_QuerySchema Schema>
using QueryResult = std::vector<typename Schema::TupleType>;

class CHQuery
{
public:
    CHQuery() = delete;

    template<c_QuerySchema Schema>
    static QueryResult<Schema> select( const std::string_view query );

    template<c_QuerySchema Schema>
    static void insert( const std::string_view tableName,
                        const std::vector<typename Schema::TupleType>& data,
                        const std::array<std::string_view, std::tuple_size_v<typename Schema::TupleType>>& colNames );

    TMQClickHouse_API static void execute( const std::string_view query );
};

}