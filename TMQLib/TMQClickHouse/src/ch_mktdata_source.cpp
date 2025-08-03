#include <TMQClickHouse/ch_mktdata_source_interface.h>

namespace TMQ
{

extern "C" MktDataSource* createMktDataSource()
{
	return new CHMktDataSource;
}

}
