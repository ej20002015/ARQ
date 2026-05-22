#include "redis_stream_offset_source.h"

#include <redis_connection.h>

#include <ARQUtils/logger.h>
#include <ARQUtils/instr.h>

namespace ARQ
{

static constexpr auto KEY_ROOT = "ARQ:StreamOffsets";

IStreamOffsetSource* createStreamOffsetSource( const std::string_view dsh )
{
    return new RedisStreamOffsetSource( dsh );
}

void RedisStreamOffsetSource::saveOffsets( const std::string_view key, const StreamTopicPartitionOffsets& offsets )
{
	Instr::Timer tmTotal;
	Instr::Timer tmConn;

    RedisConn redConn( m_dsh );
	sw::redis::Redis& redis = redConn.client();

	const auto connTime = tmConn.duration();

	Instr::Timer tmPrep;

	const std::string redisKey = std::format( "{}:{}", KEY_ROOT, key );
	std::map<std::string, std::string> redisFields;
	for( const auto& [tp, offset] : offsets )
	{
		const std::string field = std::format( "{}-{}", tp.first, tp.second );
		redisFields[field] = std::to_string( offset );
	}

	const auto prepTime = tmPrep.duration();

	Instr::Timer tmNet;

	try
	{
		redis.hset( redisKey, redisFields.begin(), redisFields.end() );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "Error saving stream offsets to Redis for key [{}]: {}", key, e.what() ) );
	}

	Log( Module::REDIS ).debug( "Saved stream offsets for key [{}] to Redis in total: {} (conn: {}, prep: {}, net: {})", key, tmTotal.duration(), connTime, prepTime, tmNet.duration() );
}

std::optional<StreamTopicPartitionOffsets> RedisStreamOffsetSource::getOffsets( const std::string_view key )
{
	Instr::Timer tmTotal;
	Instr::Timer tmConn;

	RedisConn redConn( m_dsh );
	sw::redis::Redis& redis = redConn.client();

	const auto connTime = tmConn.duration();

	Instr::Timer tmPrep;

	const std::string redisKey = std::format( "{}:{}", KEY_ROOT, key );
	std::map<std::string, std::string> redisFields;

	const auto prepTime = tmPrep.duration();

	Instr::Timer tmNet;

	try
	{
		redis.hgetall( redisKey, std::inserter( redisFields, redisFields.begin() ) );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "Error getting stream offsets from Redis for key [{}]: {}", key, e.what() ) );
	}

	const auto netTime = tmNet.duration();

	Instr::Timer tmParse;

	if( redisFields.empty() )
		return std::nullopt;

	try
	{
		StreamTopicPartitionOffsets offsets;
		for( const auto& [field, offsetStr] : redisFields )
		{
			const auto delimPos = field.rfind( '-' );
			if( delimPos == std::string::npos )
			{
				Log( Module::REDIS ).error( "Invalid field [{}] in Redis stream offsets for key [{}] - ignoring field", field, key );
				continue;
			}

			const std::string topic     = field.substr( 0, delimPos );
			const int32_t     partition = std::stoi( field.substr( delimPos + 1 ) );
			const int64_t     offset    = std::stoll( offsetStr );

			offsets[{ topic, partition }] = offset;
		}

		return offsets;
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "Error parsing stream offsets from Redis for key [{}]: {}", key, e.what() ) );
	}

	Log( Module::REDIS ).debug( "Got stream offsets for key [{}] from Redis in total: {} (conn: {}, prep: {}, net: {}, parse: {})", key, tmTotal.duration(), connTime, prepTime, netTime, tmParse.duration() );
}

}