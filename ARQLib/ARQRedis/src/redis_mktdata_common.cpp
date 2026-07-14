#include "redis_mktdata_common.h"

#include "redis_keys.h"

#include <ARQUtils/buffer.h>
#include <ARQUtils/error.h>

namespace ARQ::Redis::MD
{

RedisHashUpdates prepareMarketUpdates( const std::string_view marketName, const ARQ::MD::RecordCollection& records, const Serialiser& serialiser )
{
	RedisHashUpdates updates;
	const std::string marketKeyRoot = Keys::market( marketName );

	records.visitVectors( [&] <ARQ::MD::c_MktData T> ( const std::vector<ARQ::MD::Record<T>>& vector )
	{
		if( vector.empty() )
			return;

		RedisFields redisFields;
		redisFields.reserve( vector.size() );

		for( const ARQ::MD::Record<T>& record : vector )
		{
			try
			{
				Buffer buffer = serialiser.serialise( record );
				redisFields.emplace( record.header.id, std::move( buffer.toString() ) );
			}
			catch( const std::exception& e )
			{
				throw ARQException( std::format( "Redis market data: Error serialising {} [{}] for market [{}]: {}", ARQ::MD::Traits<T>::type(), record.header.id, marketName, e.what() ) );
			}
		}

		updates.emplace_back( std::format( "{}:{}", marketKeyRoot, ARQ::MD::Traits<T>::type() ), std::move( redisFields ) );
	} );

	return updates;
}

std::map<std::string, std::string> prepareOffsetUpdates( const StreamTopicPartitionOffsets& offsets )
{
	std::map<std::string, std::string> redisFields;
	for( const auto& [tp, offset] : offsets )
		redisFields[std::format( "{}-{}", tp.first, tp.second )] = std::to_string( offset );

	return redisFields;
}

}
