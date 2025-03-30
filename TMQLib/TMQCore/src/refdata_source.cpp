#include "refdata_source.h"

#include <TMQClickHouse/query.h>

namespace TMQ
{

// TODO: Have some way of setting this
std::shared_ptr<RefDataSource> getGlobalRefDataSource()
{
	static auto globalSource = std::make_shared<TSDBRefDataSource>();
	return globalSource;
}

std::vector<std::string> TSDBRefDataSource::fetchLatest( const std::string_view table )
{
	using Schema = QuerySchema<std::string>;

	static constexpr auto SELECT_STMT = R"(
		SELECT Blob
		FROM RefData.{0}
		WHERE Active = 1
		QUALIFY Ts = MAX(Ts) OVER (PARTITION BY ID)
		ORDER BY ID ASC;
	)";

	auto res = CHQuery::select<Schema>( std::format( SELECT_STMT, table ) );

	std::vector<std::string> buffers( res.size() );
	for( size_t i = 0; i < res.size(); ++i )
		buffers[i] = std::move( std::get<0>( res[i] ) );

	return buffers;
}

std::vector<std::string> TSDBRefDataSource::fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts )
{
	// TODO
	return std::vector<std::string>();
}

void TSDBRefDataSource::insert( const std::string_view table, const std::vector<InsertData>& insData )
{
	using Schema = QuerySchema<std::string, std::chrono::system_clock::time_point, bool, std::string>;
	static constexpr std::array<std::string_view, 4> COL_NAMES = { "ID", "Ts", "Active", "Blob" };

	// TODO: Do we need to just make InsertData itself a tuple for performance?
	std::vector<Schema::TupleType> data;
	for( const auto& dataItem : insData )
		data.emplace_back( dataItem.ID, dataItem.ts, dataItem.active, dataItem.blob );

	CHQuery::insert<Schema>( std::format( "RefData.{0}", table ), data, COL_NAMES );
}

}