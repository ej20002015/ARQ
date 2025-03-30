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

std::vector<RefDataSource::FetchData> TSDBRefDataSource::fetchLatest( const std::string_view table )
{
	using Schema = QuerySchema<std::chrono::system_clock::time_point, std::string, std::string>;

	static constexpr auto SELECT_STMT = R"(
		SELECT toUnixTimestamp64Nano(LastUpdatedTs), LastUpdatedBy, Blob
		FROM RefData.{0}
		WHERE Active = 1
		QUALIFY LastUpdatedTs = MAX(LastUpdatedTs) OVER (PARTITION BY ID)
		ORDER BY ID ASC;
	)";

	auto res = CHQuery::select<Schema>( std::format( SELECT_STMT, table ) );

	std::vector<RefDataSource::FetchData> buffers( res.size() );
	for( size_t i = 0; i < res.size(); ++i )
	{
		buffers[i] = {
			std::get<0>( res[i] ),
			std::move( std::get<1>( res[i] ) ),
			std::move( std::get<2>( res[i] ) )
		};
	}

	return buffers;
}

std::vector<RefDataSource::FetchData> TSDBRefDataSource::fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts )
{
	// TODO
	return std::vector<RefDataSource::FetchData>();
}

void TSDBRefDataSource::insert( const std::string_view table, const std::vector<InsertData>& insData )
{
	using Schema = QuerySchema<std::string, std::string, bool, std::string>;
	static constexpr std::array<std::string_view, 4> COL_NAMES = { "ID", "LastUpdatedBy", "Active", "Blob" };

	// TODO: Do we need to just make InsertData itself a tuple for performance?
	std::vector<Schema::TupleType> data;
	for( const auto& dataItem : insData )
		data.emplace_back( dataItem.ID, dataItem.lastUpdatedBy, dataItem.active, dataItem.blob );

	CHQuery::insert<Schema>( std::format( "RefData.{0}", table ), data, COL_NAMES );
}

}