#include <TMQClickHouse/ch_refdata_source.h>

#include <TMQClickHouse/ch_refdata_source_interface.h>

namespace TMQ
{

extern "C" RefDataSource* createRefDataSource()
{
	return new CHRefDataSource;
}

}
