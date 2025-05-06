#pragma once
#include <TMQClickHouse/dll.h>

#include <TMQCore/mktdata_source.h>

namespace TMQ
{

class CHMktDataSource : public MktDataSource
{
public:
	[[nodiscard]] TMQClickHouse_API virtual std::vector<FetchData> fetchLatest( const std::string_view context ) override;
	TMQClickHouse_API virtual void insert( const std::string_view context, const std::vector<InsertData>& insData ) override;
};

extern "C" TMQClickHouse_API MktDataSource* createMktDataSource();

}