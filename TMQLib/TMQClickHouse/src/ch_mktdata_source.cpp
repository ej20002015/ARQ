#include <TMQClickHouse/ch_mktdata_source.h>

#include "query.h"

namespace TMQ
{

std::vector<MktDataSource::FetchData> CHMktDataSource::fetchLatest( const std::string_view context )
{
	using Schema = QuerySchema<
		std::string,
		std::string,
		std::chrono::system_clock::time_point,
		Buffer,
		std::string,
		std::chrono::system_clock::time_point,
		std::string,
		bool>;

	static constexpr auto SELECT_STMT = R"(
		SELECT
			CAST(Type AS String) AS Type,
			InstrumentID,
			-- Get values from the row with the maximum AsofTs, using LastUpdatedTs as a tie-breaker
			toUnixTimestamp64Nano(argMax(AsofTs,        (AsofTs, LastUpdatedTs))) AS max_AsofTs,
			                      argMax(Blob,          (AsofTs, LastUpdatedTs))  AS max_Blob,
			                      argMax(Source,        (AsofTs, LastUpdatedTs))  AS max_Source,
			toUnixTimestamp64Nano(argMax(LastUpdatedTs, (AsofTs, LastUpdatedTs))) AS max_LastUpdatedTs,
			                      argMax(LastUpdatedBy, (AsofTs, LastUpdatedTs))  AS max_LastUpdatedBy,
			                      argMax(Active,        (AsofTs, LastUpdatedTs))  AS max_Active
		FROM MktData.MktObjs
		WHERE
			Context = '{}'
		GROUP BY
			Type,
			InstrumentID
		HAVING
			max_Active = 1 -- Only return objects where the latest version is active
		ORDER BY
			Type ASC,
			InstrumentID ASC
	)";

	auto res = CHQuery::select<Schema>( std::format( SELECT_STMT, context ) );

	std::vector<MktDataSource::FetchData> fetchedData( res.size() );
	for( size_t i = 0; i < res.size(); ++i )
	{
		fetchedData[i] = {
			.type          = std::move( std::get<0>( res[i] ) ),
			.instrumentID  = std::move( std::get<1>( res[i] ) ),
			.asofTs        =            std::get<2>( res[i] ),
			.blob          = std::move( std::get<3>( res[i] ) ),
			.source        = std::move( std::get<4>( res[i] ) ),
			.lastUpdatedTs =            std::get<5>( res[i] ),
			.lastUpdatedBy = std::move( std::get<6>( res[i] ) )
		};
	}

	return fetchedData;
}

extern "C" MktDataSource* createMktDataSource()
{
	return new CHMktDataSource;
}

}
