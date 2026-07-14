#include "redis_mktdata_source.h"

#include "redis_connection.h"
#include "redis_keys.h"
#include "redis_mktdata_common.h"

#include <ARQUtils/buffer.h>
#include <ARQUtils/logger.h>
#include <ARQUtils/instr.h>

#include <unordered_map>

using namespace ARQ::MD;

namespace ARQ::Redis::MD
{

IMarketSource* createMarketSource( const std::string_view dsh )
{
	return new RedisMarketSource( dsh );
}

RecordCollection RedisMarketSource::load( const std::string_view marketName, const TIDSet& filter )
{
	Instr::Timer tmTotal;
	Instr::Timer tmConn;

	RecordCollection collection;

	RedisConn conn( m_dsh );
	sw::redis::Redis&   redis = conn.client();
	sw::redis::Pipeline pl    = redis.pipeline( false ); // Don't create a new connection for the pipeline - take from conn pool

	const auto connTime = tmConn.duration();

	// -----------------
	// 1. Queue commands
	// -----------------

	Instr::Timer tmPrep;
	const std::string marketKeyRoot = Keys::market( marketName );

	try
	{
		collection.visitVectors( [&] <c_MktData T> ( std::vector<Record<T>>&vector )
		{
			const std::string hashKey = std::format( "{}:{}", marketKeyRoot, Traits<T>::type() );

			TIDSet::IDs idSpec = filter.empty() ? TIDSet::All{} : filter.getIDsForType( Traits<T>::typeEnum() );
			if( std::holds_alternative<TIDSet::None>( idSpec ) )
				return;                                          // filter explicitly specifies 'None' for this type, so skip entirely
			else if( std::holds_alternative<TIDSet::All>( idSpec ) )
				pl.hgetall( hashKey );                           // filter specifies 'all' for this type, so use hgetall
			else if( TIDSet::IDList* list = std::get_if<TIDSet::IDList>( &idSpec ) )
				pl.hmget( hashKey, list->begin(), list->end() ); // filter specifies specific IDs, so only get those fields using hmget
		} );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "RedisMarketSource: Error queuing redis pipeline to load market [{}]: {}", marketName, e.what() ) );
	}

	const auto prepTime = tmPrep.duration();

	// -------------------
	// 2. Execute pipeline
	// -------------------
	sw::redis::QueuedReplies replies;

	Instr::Timer tmNet;

	try
	{
		replies = pl.exec();
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "RedisMarketSource: Error executing redis pipeline to load market [{}]: {}", marketName, e.what() ) );
	}

	const auto netTime = tmNet.duration();

	// -------------------
	// 2. Parse results
	// -------------------

	Instr::Timer tmParse;

	size_t replyIndex = 0;
	collection.visitVectors( [&] <c_MktData T> ( std::vector<Record<T>>& vector )
	{
		TIDSet::IDs idSpec = filter.empty() ? TIDSet::All{} : filter.getIDsForType( Traits<T>::typeEnum() );

		if( std::holds_alternative<TIDSet::None>( idSpec ) )
			return; // We didn't queue a command for this type

		// Helper lambda

		auto deserialiseAndPush = [&] ( const std::string_view id, const std::string_view serializedRecord )
		{
			BufferView buffer( reinterpret_cast<const uint8_t*>( serializedRecord.data() ), serializedRecord.size() );
			ARQ_DO_IN_TRY( arqExc, errMsg );
			{
				auto record = m_serialiser->deserialise<Record<T>>( buffer );
				vector.push_back( std::move( record ) );
			}
			ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

			if( arqExc.what().size() )
				Log( Module::REDIS ).error( arqExc, "RedisMarketSource: Exception deserialising {} [{}] for market [{}]", Traits<T>::type(), id, marketName );
			else if( errMsg.size() )
				Log( Module::REDIS ).error( "RedisMarketSource: Error deserialising {} [{}] for market [{}]: {}", Traits<T>::type(), id, marketName, errMsg );
		};

		// Parse based on the command queued

		if( std::holds_alternative<TIDSet::All>( idSpec ) )
		{
			// HGETALL returns a dictionary
			std::unordered_map<std::string, std::string> hashMap;
			replies.get( replyIndex++, std::inserter( hashMap, hashMap.end() ) );

			vector.reserve( hashMap.size() );
			for( const auto& [id, serializedRecord] : hashMap )
				deserialiseAndPush( id, serializedRecord );
		}
		else if( const auto* list = std::get_if<TIDSet::IDList>( &idSpec ) )
		{
			// HMGET returns an array of Optionals (null if key didn't exist)
			std::vector<sw::redis::OptionalString> optValues;
			replies.get( replyIndex++, std::back_inserter( optValues ) );

			vector.reserve( list->size() );

			// We must zip the requested ID list with the returned optionals
			for( size_t i = 0; i < list->size(); ++i )
			{
				if( optValues[i] )
					deserialiseAndPush( ( *list )[i], *optValues[i] );
			}
		}
	} );

	Log( Module::REDIS ).debug( "Loaded market [{}] from Redis with {} objects in total: {} (conn: {}, prep: {}, net: {}, parse: {})", marketName, collection.size(), tmTotal.duration(), connTime, prepTime, netTime, tmParse.duration() );

	return collection;
}

void RedisMarketSource::save( const std::string_view marketName, const RecordCollection& records )
{
	Instr::Timer tmTotal;
	Instr::Timer tmPrep;

	RedisHashUpdates updates = prepareMarketUpdates( marketName, records, *m_serialiser );
	const auto prepTime = tmPrep.duration();

	Instr::Timer tmConn;

	RedisConn conn( m_dsh );
	sw::redis::Redis&   redis    = conn.client();
	sw::redis::Pipeline pl       = redis.pipeline( false ); // Don't create a new connection for the pipeline - take from conn pool

	const auto connTime = tmConn.duration();

	for( const auto& [hashKey, redisFields] : updates )
		pl.hset( hashKey, redisFields.begin(), redisFields.end() );

	// -------------------
	// 2. Execute pipeline
	// -------------------

	Instr::Timer tmNet;

	try
	{
		pl.exec();
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "RedisMarketSource: Error executing redis pipeline to save market [{}]: {}", marketName, e.what() ) );
	}

	Log( Module::REDIS ).debug( "Saved market [{}] to Redis with {} objects in total: {} (conn: {}, prep: {}, net: {})", marketName, records.size(), tmTotal.duration(), connTime, prepTime, tmNet.duration() );
}

}
