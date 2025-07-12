#pragma once
#include <TMQClickHouse/dll.h>

#include <TMQCore/refdata_source.h>

namespace TMQ
{

extern "C" TMQClickHouse_API RefDataSource* createRefDataSource();

}