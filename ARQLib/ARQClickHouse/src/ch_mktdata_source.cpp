#include <ARQClickHouse/ch_mktdata_source.h>

#include <ARQClickHouse/ch_mktdata_source_interface.h>

namespace ARQ
{

extern "C" MktDataSource* createMktDataSource( const std::string_view dsh )
{
	return new CHMktDataSource( dsh );
}

}
