#include <TMQClickHouse/ch_refdata_source.h>

#include "query.h"

namespace TMQ
{

std::vector<RefDataSource::FetchData> CHRefDataSource::fetchLatest( const std::string_view table )
{
	using Schema = QuerySchema<std::chrono::system_clock::time_point, std::string, Buffer>;

	static constexpr auto SELECT_STMT = R"(
		SELECT LastUpdatedTs, LastUpdatedBy, Blob
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
			.lastUpdatedTs =            std::get<0>( res[i] ),
			.lastUpdatedBy = std::move( std::get<1>( res[i] ) ),
			.blob          = std::move( std::get<2>( res[i] ) )
		};
	}

	return buffers;
}

std::vector<RefDataSource::FetchData> CHRefDataSource::fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts )
{
	// TODO
	return std::vector<RefDataSource::FetchData>();
}

void CHRefDataSource::insert( const std::string_view table, const std::vector<InsertData>& insData )
{
	using Schema = QuerySchema<std::string_view, std::string_view, bool, BufferView>;
	static constexpr std::array<std::string_view, 4> COL_NAMES = { "ID", "LastUpdatedBy", "Active", "Blob" };

	// TODO: Do we need to just make InsertData itself a tuple for performance?
	std::vector<Schema::TupleType> data;
	for( const auto& d : insData )
		data.emplace_back( d.ID, d.lastUpdatedBy, d.active, d.blob );

	CHQuery::insert<Schema>( std::format( "RefData.{0}", table ), data, COL_NAMES );
}

extern "C" RefDataSource* createRefDataSource()
{
	return new CHRefDataSource;
}

}
