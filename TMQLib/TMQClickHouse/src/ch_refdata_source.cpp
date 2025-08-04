#include <TMQClickHouse/ch_refdata_source.h>

#include <TMQClickHouse/ch_refdata_source_interface.h>

namespace TMQ
{

extern "C" RefDataSource* createRefDataSource( const std::string_view dsh )
{
	return new CHRefDataSource( dsh );
}

}
