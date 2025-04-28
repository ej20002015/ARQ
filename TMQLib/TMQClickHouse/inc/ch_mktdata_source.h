#pragma once
#include <TMQClickHouse/dll.h>

#include <TMQCore/mktdata_source.h>

namespace TMQ
{

class CHMktDataSource : public MktDataSource
{
public:
	[[nodiscard]] TMQClickHouse_API virtual std::vector<FetchData> fetchLatest( const std::string_view context ) override;
};

extern "C" TMQClickHouse_API MktDataSource* createMktDataSource();

}