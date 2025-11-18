#pragma once
#include <ARQClickHouse/dll.h>

#include <ARQCore/mktdata_source.h>

namespace ARQ
{

extern "C" ARQClickHouse_API IMktDataSource* createMktDataSource( const std::string_view dsh );

}