#include "redis_live_market_store.h"

#include "redis_connection.h"
#include "redis_keys.h"
#include "redis_mktdata_common.h"

#include <ARQUtils/error.h>
#include <ARQUtils/instr.h>
#include <ARQUtils/logger.h>

namespace ARQ::Redis::MD
{

namespace
{

void validateTransactionReplies( sw::redis::QueuedReplies& replies )
{
	// redis++ commands may have errors but these are only thrown as exceptions when they are accessed
	for( size_t i = 0; i < replies.size(); ++i )
		static_cast<void>( replies.get( i ) );
}

}

ARQ::MD::ILiveMarketStore* createLiveMarketStore( const std::string_view dsh )
{
	return new RedisLiveMarketStore( dsh );
}

void RedisLiveMarketStore::apply( const ARQ::MD::MarketUpdateBatch& updateBatch )
{
	Instr::Timer tmTotal;
	Instr::Timer tmPrep;

	const std::string marketName = updateBatch.marketName.str();
	if( marketName.empty() )
		throw ARQException( "RedisLiveMarketStore: Cannot apply a live market batch without a market name" );
	if( updateBatch.offsets.empty() )
		throw ARQException( std::format( "RedisLiveMarketStore: Cannot apply live market batch [{}] without source offsets", marketName ) );

	const RedisHashUpdates marketUpdates = prepareMarketUpdates( marketName, updateBatch.records, *m_serialiser );
	const auto             offsetUpdates = prepareOffsetUpdates( updateBatch.offsets );

	const auto prepTime = tmPrep.duration();

	Instr::Timer tmConn;

	RedisConn conn( m_dsh );
	sw::redis::Redis& redis   = conn.client();
	sw::redis::Transaction tx = redis.transaction( true, false );

	const auto connTime = tmConn.duration();

	try
	{
		for( const auto& [hashKey, redisFields] : marketUpdates.sets )
			tx.hset( hashKey, redisFields.begin(), redisFields.end() );
		for( const auto& [hashKey, redisFields] : marketUpdates.dels )
			tx.hdel( hashKey, redisFields.begin(), redisFields.end() );

		const std::string offsetKey = Keys::liveMarketOffsets( marketName );
		tx.hset( offsetKey, offsetUpdates.begin(), offsetUpdates.end() );

		Instr::Timer tmNet;
		auto replies = tx.exec();
		validateTransactionReplies( replies );
		Log( Module::REDIS ).debug( "Applied live market batch [{}] to Redis with {} objects and {} offsets atomically in total: {} (conn: {}, prep: {}, net: {})", marketName, updateBatch.records.size(), updateBatch.offsets.size(), tmTotal.duration(), connTime, prepTime, tmNet.duration() );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "RedisLiveMarketStore: Error atomically applying live market batch [{}]: {}", marketName, e.what() ) );
	}
}

}
