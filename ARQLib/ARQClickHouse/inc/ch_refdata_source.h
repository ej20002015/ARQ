#pragma once
#include <ARQClickHouse/dll.h>

#include <ARQCore/refdata_source.h>

namespace ARQ
{

extern "C" ARQClickHouse_API RefDataSource* createRefDataSource( const std::string_view dsh );

}