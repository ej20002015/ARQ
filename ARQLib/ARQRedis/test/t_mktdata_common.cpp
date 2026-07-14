#include "redis_mktdata_common.h"

#include <ARQUtils/buffer.h>
#include <ARQMarket/mktdata_entities.h>

#include <gtest/gtest.h>

#include <format>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

using namespace ARQ;

namespace
{

template<MD::c_MktData T>
class DeterministicRecordSerialiser : public ISerialisableType<MD::Record<T>>
{
public:
	explicit DeterministicRecordSerialiser( std::string prefix )
		: m_prefix( std::move( prefix ) )
	{}

	Buffer serialise( const MD::Record<T>& record ) const override
	{
		const std::string value = m_prefix + record.header.id;
		return Buffer( value.data(), value.size() );
	}

	void deserialise( const BufferView, MD::Record<T>& ) const override
	{}

private:
	std::string m_prefix;
};

const Redis::MD::RedisFields& fieldsForKey( const Redis::MD::RedisHashUpdates& updates, const std::string_view key )
{
	const auto it = std::ranges::find_if( updates, [key] ( const auto& update )
	{
		return update.first == key;
	} );

	if( it == updates.end() )
		throw std::runtime_error( std::format( "Redis update key [{}] not found", key ) );

	return it->second;
}

}

TEST( RedisMktDataCommonTest, PrepareMarketUpdatesProducesExpectedKeysAndSerializedFields )
{
	Serialiser serialiser;
	serialiser.registerHandler<MD::Record<MD::FXRate>>( std::make_unique<DeterministicRecordSerialiser<MD::FXRate>>( "fx:" ) );
	serialiser.registerHandler<MD::Record<MD::EQPrice>>( std::make_unique<DeterministicRecordSerialiser<MD::EQPrice>>( "eq:" ) );

	MD::RecordCollection records;
	records.get<MD::Record<MD::FXRate>>().push_back( MD::Record<MD::FXRate>{ .header = { .id = "GBPUSD" } } );
	records.get<MD::Record<MD::FXRate>>().push_back( MD::Record<MD::FXRate>{ .header = { .id = "EURUSD" } } );
	records.get<MD::Record<MD::EQPrice>>().push_back( MD::Record<MD::EQPrice>{ .header = { .id = "AAPL" } } );

	const Redis::MD::RedisHashUpdates updates = Redis::MD::prepareMarketUpdates( "LIVE", records, serialiser );

	ASSERT_EQ( updates.size(), 2 );

	const Redis::MD::RedisFields& fxFields = fieldsForKey( updates, "ARQ:Markets:LIVE:FXR" );
	ASSERT_EQ( fxFields.size(), 2 );
	EXPECT_EQ( fxFields.at( "GBPUSD" ), "fx:GBPUSD" );
	EXPECT_EQ( fxFields.at( "EURUSD" ), "fx:EURUSD" );

	const Redis::MD::RedisFields& eqFields = fieldsForKey( updates, "ARQ:Markets:LIVE:EQP" );
	ASSERT_EQ( eqFields.size(), 1 );
	EXPECT_EQ( eqFields.at( "AAPL" ), "eq:AAPL" );
}

TEST( RedisMktDataCommonTest, PrepareOffsetUpdatesProducesExpectedTopicPartitionFields )
{
	const StreamTopicPartitionOffsets offsets{
		{ StreamTopicPartition{ "ARQ.MktData.Updates.FXR", 0 }, 42 },
		{ StreamTopicPartition{ "ARQ.MktData.Updates.FXR", 3 }, 97 },
		{ StreamTopicPartition{ "ARQ.MktData.Updates.EQP", 1 }, 1'234 }
	};

	const std::map<std::string, std::string> fields = Redis::MD::prepareOffsetUpdates( offsets );

	ASSERT_EQ( fields.size(), 3 );
	EXPECT_EQ( fields.at( "ARQ.MktData.Updates.FXR-0" ), "42" );
	EXPECT_EQ( fields.at( "ARQ.MktData.Updates.FXR-3" ), "97" );
	EXPECT_EQ( fields.at( "ARQ.MktData.Updates.EQP-1" ), "1234" );
}
