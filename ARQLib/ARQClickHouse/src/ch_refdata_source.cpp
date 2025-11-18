#include <ARQClickHouse/ch_refdata_source.h>

#include <ARQClickHouse/ch_refdata_source_interface.h>

namespace ARQ
{

extern "C" IRefDataSource* createRefDataSource( const std::string_view dsh )
{
	return new CHRefDataSource( dsh );
}

}
