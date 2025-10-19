#pragma once
#include <ARQGrpc/dll.h>

#include <ARQCore/refdata_source.h>

namespace ARQ
{

extern "C" ARQGrpc_API RefDataSource* createRefDataSource( const std::string_view dsh );

}