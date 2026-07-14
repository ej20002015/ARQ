#pragma once

#include <ARQRedis/dll.h>

#include <ARQCore/serialiser.h>
#include <ARQCore/streaming_service.h>
#include <ARQMarket/mktdata_entities.h>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ARQ::Redis::MD
{

using RedisFields      = std::unordered_map<std::string, std::string>;
using RedisHashUpdates = std::vector<std::pair<std::string, RedisFields>>;

ARQRedis_API RedisHashUpdates                   prepareMarketUpdates( const std::string_view marketName, const ARQ::MD::RecordCollection& records, const Serialiser& serialiser );
ARQRedis_API std::map<std::string, std::string> prepareOffsetUpdates( const StreamTopicPartitionOffsets& offsets );

}
