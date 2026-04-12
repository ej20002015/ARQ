#pragma once
#include <ARQRedis/dll.h>

#include <ARQCore/stream_offset_source.h>

namespace ARQ
{

extern "C" ARQRedis_API IStreamOffsetSource* createStreamOffsetSource( const std::string_view dsh );

class RedisStreamOffsetSource : public IStreamOffsetSource
{
public:
	RedisStreamOffsetSource( const std::string_view dsh )
		: m_dsh( dsh )
	{}

	void                                       saveOffsets( const std::string_view key, const StreamTopicPartitionOffsets& offsets ) override;
	std::optional<StreamTopicPartitionOffsets> getOffsets( const std::string_view key )                                              override;

private:
	std::string m_dsh;
};

}