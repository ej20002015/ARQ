#pragma once

#include <ARQRedis/dll.h>

#include <ARQCore/serialiser.h>
#include <ARQCore/streaming_service.h>
#include <ARQMarket/mktdata_entities.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ARQ::Redis::MD
{

using RedisFields     = std::unordered_map<std::string, std::string>;
using RedisFieldNames = std::vector<std::string>;
using RedisHashSets   = std::vector<std::pair<std::string, RedisFields>>;
using RedisHashDels   = std::vector<std::pair<std::string, RedisFieldNames>>;

struct RedisHashUpdates
{
	RedisHashSets sets;
	RedisHashDels dels;
};

ARQRedis_API RedisHashUpdates                   prepareMarketUpdates( const std::string_view marketName, const ARQ::MD::RecordCollection& records, const Serialiser& serialiser );
ARQRedis_API std::pair<std::string, std::string> prepareOffsetUpdate( const StreamTopicPartitionOffset& sourcePosition );

}
