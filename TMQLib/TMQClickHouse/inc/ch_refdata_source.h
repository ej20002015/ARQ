#pragma once
#include <TMQClickHouse/dll.h>

#include <TMQCore/refdata_source.h>

namespace TMQ
{

class CHRefDataSource : public RefDataSource
{
public:
	[[nodiscard]] TMQClickHouse_API std::vector<FetchData> fetchLatest( const std::string_view table ) override;
	[[nodiscard]] TMQClickHouse_API std::vector<FetchData> fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts ) override;

	TMQClickHouse_API void insert( const std::string_view table, const std::vector<InsertData>& insData ) override;
};

extern "C" TMQClickHouse_API RefDataSource* createRefDataSource();

}