#pragma once
#include <TMQClickHouse/dll.h>

#include <TMQCore/mktdata_source.h>

namespace TMQ
{

extern "C" TMQClickHouse_API MktDataSource* createMktDataSource( const std::string_view dsh );

}