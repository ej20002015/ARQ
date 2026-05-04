#include <ARQClickHouse/ch_mktdata_source.h>

#include "connection.h"
#include "ch_mktdata_queries.h"

using namespace ARQ::MD;

namespace ARQ::CH::MD
{

IMarketSource* createMarketSource( const std::string_view dsh )
{
	return new CHMarketSource( dsh );
}

RecordCollection CHMarketSource::load( const std::string_view marketName, const TIDSet& filter )
{
	RecordCollection collection;

	CHConn conn( m_dsh );

	collection.visitVectors( [&] <c_MktData T> ( std::vector<Record<T>>& vector )
	{
		std::vector<std::string_view> ids;

		TIDSet::IDs idSpec = filter.empty() ? TIDSet::All{} : filter.getIDsForType( Traits<T>::typeEnum() );
		if( std::holds_alternative<TIDSet::None>( idSpec ) )
			return;                                   // filter explicitly specifies 'None' for this type, so skip entirely
		else if( std::holds_alternative<TIDSet::All>( idSpec ) )
			ids = {};                                 // filter specifies 'all' for this type, so we keep ids empty to signify no filtering on IDs
		else if( std::holds_alternative<TIDSet::IDList>( idSpec ) )
			ids = std::get<TIDSet::IDList>( idSpec ); // filter specifies specific IDs, so we use them in the query
		
		// Select from ClickHouse DB
		vector = select<T>( conn, marketName, ids );
	} );

	return collection;
}

void CHMarketSource::save( const std::string_view marketName, const RecordCollection& records )
{
	CHConn conn( m_dsh );

	records.visitVectors( [&] <c_MktData T> ( const std::vector<Record<T>>& vector )
	{
		// Insert into ClickHouse DB
		insert<T>( conn, marketName, vector );
	} );
}

}