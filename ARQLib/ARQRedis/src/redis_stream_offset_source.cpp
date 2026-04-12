#include "redis_stream_offset_source.h"

#include <ARQUtils/logger.h>

#include <sw/redis++/redis++.h>

namespace ARQ
{

IStreamOffsetSource* createStreamOffsetSource( const std::string_view dsh )
{
	return new RedisStreamOffsetSource( dsh );
}

void RedisStreamOffsetSource::saveOffsets( const std::string_view key, const StreamTopicPartitionOffsets& offsets )
{
    // 1. Connect to local Redis
    sw::redis::ConnectionOptions opts;
    opts.host = "127.0.0.1";
    opts.port = 6379;

    auto redis = std::make_unique<sw::redis::Redis>( opts );
    std::string pingRes = redis->ping();

    redis->set( "Evan", "Hello World!" );
    auto optStr = redis->get( "Evan" );
    if( !optStr )
        throw ARQException( "Failed to get value for key [Evan] from Redis" );
    else
	    Log( Module::CORE ).info( "Got value [{}] for key [Evan] from Redis", optStr.value() );
}

std::optional<StreamTopicPartitionOffsets> RedisStreamOffsetSource::getOffsets( const std::string_view key )
{
	return std::nullopt;
}

}