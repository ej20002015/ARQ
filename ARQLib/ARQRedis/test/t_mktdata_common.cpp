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
		std::string value = m_prefix + record.header.id;
		if( !record.header.lastUpdatedBy.empty() )
			value += ":" + record.header.lastUpdatedBy;
		return Buffer( value.data(), value.size() );
	}

	void deserialise( const BufferView, MD::Record<T>& ) const override
	{}

private:
	std::string m_prefix;
};

const Redis::MD::RedisFields& fieldsForKey( const Redis::MD::RedisHashUpdates& updates, const std::string_view key )
{
	const auto it = std::ranges::find_if( updates.sets, [key] ( const auto& update )
	{
		return update.first == key;
	} );

	if( it == updates.sets.end() )
		throw std::runtime_error( std::format( "Redis update key [{}] not found", key ) );

	return it->second;
}

const Redis::MD::RedisFieldNames& fieldsToDeleteForKey( const Redis::MD::RedisHashUpdates& updates, const std::string_view key )
{
	const auto it = std::ranges::find_if( updates.dels, [key] ( const auto& update )
	{
		return update.first == key;
	} );

	if( it == updates.dels.end() )
		throw std::runtime_error( std::format( "Redis deletion key [{}] not found", key ) );

	return it->second;
}

MD::Record<MD::FXRate> makeFXRecord( std::string id, const bool isActive, std::string version = {} )
{
	return MD::Record<MD::FXRate>{
		.header = {
			.id            = std::move( id ),
			.isActive      = isActive,
			.lastUpdatedBy = std::move( version )
		}
	};
}

void registerFXSerialiser( Serialiser& serialiser )
{
	serialiser.registerHandler<MD::Record<MD::FXRate>>( std::make_unique<DeterministicRecordSerialiser<MD::FXRate>>( "fx:" ) );
}

}

TEST( RedisMktDataCommonTest, PrepareMarketUpdatesProducesExpectedKeysAndSerializedFields )
{
	Serialiser serialiser;
	serialiser.registerHandler<MD::Record<MD::FXRate>>( std::make_unique<DeterministicRecordSerialiser<MD::FXRate>>( "fx:" ) );
	serialiser.registerHandler<MD::Record<MD::EQPrice>>( std::make_unique<DeterministicRecordSerialiser<MD::EQPrice>>( "eq:" ) );

	MD::RecordCollection records;
	records.get<MD::Record<MD::FXRate>>().push_back(  MD::Record<MD::FXRate>{  .header = { .id = "GBPUSD", .isActive = true  } } );
	records.get<MD::Record<MD::FXRate>>().push_back(  MD::Record<MD::FXRate>{  .header = { .id = "EURUSD", .isActive = false } } );
	records.get<MD::Record<MD::EQPrice>>().push_back( MD::Record<MD::EQPrice>{ .header = { .id = "AAPL",   .isActive = true  } } );

	const Redis::MD::RedisHashUpdates updates = Redis::MD::prepareMarketUpdates( "LIVE", records, serialiser );

	ASSERT_EQ( updates.sets.size(), 2 );
	ASSERT_EQ( updates.dels.size(), 1 );

	const Redis::MD::RedisFields& fxFields = fieldsForKey( updates, "ARQ:Markets:LIVE:FXR" );
	ASSERT_EQ( fxFields.size(), 1 );
	EXPECT_EQ( fxFields.at( "GBPUSD" ), "fx:GBPUSD" );

	const Redis::MD::RedisFieldNames& fieldsToDel = fieldsToDeleteForKey( updates, "ARQ:Markets:LIVE:FXR" );
	ASSERT_EQ( fieldsToDel.size(), 1 );
	ASSERT_EQ( fieldsToDel.front(), "EURUSD" );

	const Redis::MD::RedisFields& eqFields = fieldsForKey( updates, "ARQ:Markets:LIVE:EQP" );
	ASSERT_EQ( eqFields.size(), 1 );
	EXPECT_EQ( eqFields.at( "AAPL" ), "eq:AAPL" );
}

TEST( RedisMktDataCommonTest, ActiveThenInactiveProducesOnlyDelete )
{
	Serialiser serialiser;
	registerFXSerialiser( serialiser );

	MD::RecordCollection records;
	auto& fxRecords = records.get<MD::Record<MD::FXRate>>();
	fxRecords.push_back( makeFXRecord( "GBPUSD", true ) );
	fxRecords.push_back( makeFXRecord( "GBPUSD", false ) );

	const Redis::MD::RedisHashUpdates updates = Redis::MD::prepareMarketUpdates( "LIVE", records, serialiser );

	EXPECT_TRUE( updates.sets.empty() );
	ASSERT_EQ( updates.dels.size(), 1 );
	const Redis::MD::RedisFieldNames& fieldsToDel = fieldsToDeleteForKey( updates, "ARQ:Markets:LIVE:FXR" );
	ASSERT_EQ( fieldsToDel.size(), 1 );
	EXPECT_EQ( fieldsToDel.front(), "GBPUSD" );
}

TEST( RedisMktDataCommonTest, InactiveThenActiveProducesOnlySet )
{
	Serialiser serialiser;
	registerFXSerialiser( serialiser );

	MD::RecordCollection records;
	auto& fxRecords = records.get<MD::Record<MD::FXRate>>();
	fxRecords.push_back( makeFXRecord( "GBPUSD", false ) );
	fxRecords.push_back( makeFXRecord( "GBPUSD", true ) );

	const Redis::MD::RedisHashUpdates updates = Redis::MD::prepareMarketUpdates( "LIVE", records, serialiser );

	EXPECT_TRUE( updates.dels.empty() );
	ASSERT_EQ( updates.sets.size(), 1 );
	const Redis::MD::RedisFields& fields = fieldsForKey( updates, "ARQ:Markets:LIVE:FXR" );
	ASSERT_EQ( fields.size(), 1 );
	EXPECT_EQ( fields.at( "GBPUSD" ), "fx:GBPUSD" );
}

TEST( RedisMktDataCommonTest, RepeatedInactiveRecordsProduceSingleDelete )
{
	Serialiser serialiser;
	registerFXSerialiser( serialiser );

	MD::RecordCollection records;
	auto& fxRecords = records.get<MD::Record<MD::FXRate>>();
	fxRecords.push_back( makeFXRecord( "GBPUSD", false ) );
	fxRecords.push_back( makeFXRecord( "GBPUSD", false ) );

	const Redis::MD::RedisHashUpdates updates = Redis::MD::prepareMarketUpdates( "LIVE", records, serialiser );

	EXPECT_TRUE( updates.sets.empty() );
	ASSERT_EQ( updates.dels.size(), 1 );
	const Redis::MD::RedisFieldNames& fieldsToDel = fieldsToDeleteForKey( updates, "ARQ:Markets:LIVE:FXR" );
	ASSERT_EQ( fieldsToDel.size(), 1 );
	EXPECT_EQ( fieldsToDel.front(), "GBPUSD" );
}

TEST( RedisMktDataCommonTest, LaterActiveRecordReplacesEarlierValue )
{
	Serialiser serialiser;
	registerFXSerialiser( serialiser );

	MD::RecordCollection records;
	auto& fxRecords = records.get<MD::Record<MD::FXRate>>();
	fxRecords.push_back( makeFXRecord( "GBPUSD", true, "version-1" ) );
	fxRecords.push_back( makeFXRecord( "GBPUSD", true, "version-2" ) );

	const Redis::MD::RedisHashUpdates updates = Redis::MD::prepareMarketUpdates( "LIVE", records, serialiser );

	EXPECT_TRUE( updates.dels.empty() );
	ASSERT_EQ( updates.sets.size(), 1 );
	const Redis::MD::RedisFields& fields = fieldsForKey( updates, "ARQ:Markets:LIVE:FXR" );
	ASSERT_EQ( fields.size(), 1 );
	EXPECT_EQ( fields.at( "GBPUSD" ), "fx:GBPUSD:version-2" );
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
