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
		WHERE (Active = 1) AND (Ts = (
			SELECT MAX(Ts)
			FROM RefData.{0} AS inner_{0}
			WHERE inner_{0}.ID = RefData.{0}.ID
		))
		ORDER BY ID ASC
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

}