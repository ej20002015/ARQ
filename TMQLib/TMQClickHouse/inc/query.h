#pragma once
#include <TMQClickHouse/dll.h>

#include <tuple>
#include <vector>
#include <string>

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

    TMQ_API static void execute( const std::string_view query );
};

}