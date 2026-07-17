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

	records.visitVectors( [&] <ARQ::MD::c_MktData T> ( const std::vector<ARQ::MD::Record<T>>&vector )
	{
		if( vector.empty() )
			return;

		std::unordered_map<std::string, std::optional<std::string>> fieldValues;
		fieldValues.reserve( vector.size() );

		for( const ARQ::MD::Record<T>& record : vector )
		{
			std::optional<std::string>& fieldValue = fieldValues[record.header.id];

			if( record.header.isActive )
			{
				try
				{
					Buffer buffer = serialiser.serialise( record );
					fieldValue = std::move( buffer.toString() );
				}
				catch( const std::exception& e )
				{
					throw ARQException( std::format( "Redis market data: Error serialising {} [{}] for market [{}]: {}", ARQ::MD::Traits<T>::type(), record.header.id, marketName, e.what() ) );
				}
			}
			else
				fieldValue = std::nullopt; // Mark for deletion
		}

		RedisFields     redisFieldsToSet;
		RedisFieldNames redisFieldsToDel;
		redisFieldsToSet.reserve( fieldValues.size() );
		redisFieldsToDel.reserve( fieldValues.size() );

		for( auto& [fieldName, fieldValue] : fieldValues )
		{
			if( fieldValue.has_value() )
				redisFieldsToSet.emplace( std::move( fieldName ), std::move( *fieldValue ) );
			else
				redisFieldsToDel.emplace_back( std::move( fieldName ) );
		}

		const std::string key = std::format( "{}:{}", marketKeyRoot, ARQ::MD::Traits<T>::type() );
		if( redisFieldsToSet.size() )
			updates.sets.emplace_back( key, std::move( redisFieldsToSet ) );
		if( redisFieldsToDel.size() )
			updates.dels.emplace_back( key, std::move( redisFieldsToDel ) );
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
