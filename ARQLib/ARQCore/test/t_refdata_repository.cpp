#include <ARQCore/refdata_repository.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace ARQ;
using namespace ARQ::RD;

namespace
{

ID::UUID makeUUID( const uint8_t marker )
{
	ID::UUID id;
	id.bytes.back() = marker;
	return id;
}

Record<Currency> makeCurrencyRecord( const uint8_t marker, std::string ccyID, std::string name )
{
	Record<Currency> record;
	record.header.uuid          = makeUUID( marker );
	record.header.lastUpdatedBy = "test-user";
	record.header.version       = marker;
	record.data.uuid            = record.header.uuid;
	record.data.ccyID           = std::move( ccyID );
	record.data.name            = std::move( name );
	record.data.decimalPlaces   = 2;
	record.data.settlementDays  = 2;
	return record;
}

Record<User> makeUserRecord( const uint8_t marker, std::string userID, std::optional<std::string> tradingDesk )
{
	Record<User> record;
	record.header.uuid          = makeUUID( marker );
	record.header.lastUpdatedBy = "test-user";
	record.header.version       = marker;
	record.data.uuid            = record.header.uuid;
	record.data.userID          = std::move( userID );
	record.data.fullName        = std::format( "Test User {}", marker );
	record.data.email           = std::format( "user{}@example.test", marker );
	record.data.tradingDesk     = std::move( tradingDesk );
	return record;
}

std::vector<std::string> userIDs( const std::vector<OptConstRef<User>>& users )
{
	std::vector<std::string> ids;
	ids.reserve( users.size() );
	for( const auto& user : users )
	{
		EXPECT_TRUE( user );
		if( user )
			ids.push_back( user->userID );
	}
	std::ranges::sort( ids );
	return ids;
}

template<c_RefData T>
struct EntitySourceState
{
	std::vector<Record<T>> m_records;
	std::atomic<size_t>    m_fetchCount = 0;
	std::atomic<bool>      m_failNextFetch = false;
};

template<c_RefData T>
class FakeEntitySource final : public IEntitySource<T>
{
public:
	explicit FakeEntitySource( std::shared_ptr<EntitySourceState<T>> state )
		: m_state( std::move( state ) )
	{
	}

	std::vector<Record<T>> fetch() const override
	{
		++m_state->m_fetchCount;
		if( m_state->m_failNextFetch.exchange( false ) )
			throw ARQException( "Deliberate reference-data fetch failure" );

		return m_state->m_records;
	}

	void insert( const std::vector<Record<T>>& ) override
	{
	}

private:
	std::shared_ptr<EntitySourceState<T>> m_state;
};

}

class RefDataRepositoryTest : public ::testing::Test
{
protected:
	static constexpr std::string_view DATA_SOURCE_HANDLE = "TEST_REF_DATA_REPOSITORY";

	void SetUp() override
	{
		m_currencyState = std::make_shared<EntitySourceState<Currency>>();
		m_userState = std::make_shared<EntitySourceState<User>>();
		m_source = std::make_shared<Source>();
		m_source->registerEntitySource<Currency>( std::make_unique<FakeEntitySource<Currency>>( m_currencyState ) );
		m_source->registerEntitySource<User>( std::make_unique<FakeEntitySource<User>>( m_userState ) );
		SourceFactory::inst().addCustomSource( DATA_SOURCE_HANDLE, m_source );
	}

	void TearDown() override
	{
		SourceFactory::inst().delCustomSource( DATA_SOURCE_HANDLE );
	}

	std::shared_ptr<EntitySourceState<Currency>> m_currencyState;
	std::shared_ptr<EntitySourceState<User>>     m_userState;
	std::shared_ptr<Source>                      m_source;
};

TEST( RefDataCacheTest, EmptyCacheReportsNoRecordsAndReturnsNoMatches )
{
	Cache<Currency> cache( std::vector<Record<Currency>>{} );

	EXPECT_TRUE( cache.empty() );
	EXPECT_EQ( cache.size(), 0 );
	EXPECT_TRUE( cache.getMap().empty() );
	EXPECT_TRUE( cache.getList().empty() );
	EXPECT_FALSE( cache.getRecord( makeUUID( 1 ) ) );
	EXPECT_FALSE( cache.get( makeUUID( 1 ) ) );
	EXPECT_FALSE( cache.getRecordByIndex( "ccyID", "GBP" ) );
	EXPECT_FALSE( cache.getByIndex( "ccyID", "GBP" ) );
}

TEST( RefDataCacheTest, ReturnsTheSameRecordByIDAndUniqueIndex )
{
	const ID::UUID gbpID = makeUUID( 1 );
	std::vector<Record<Currency>> records;
	records.push_back( makeCurrencyRecord( 1, "GBP", "Pound Sterling" ) );
	records.push_back( makeCurrencyRecord( 2, "USD", "US Dollar" ) );
	Cache<Currency> cache( std::move( records ) );

	ASSERT_FALSE( cache.empty() );
	ASSERT_EQ( cache.size(), 2 );
	const auto byID = cache.getRecord( gbpID );
	const auto byIndex = cache.getRecordByIndex( "ccyID", "GBP" );
	const auto dataByID = cache.get( gbpID );
	const auto dataByIndex = cache.getByIndex( "ccyID", "GBP" );

	ASSERT_TRUE( byID );
	ASSERT_TRUE( byIndex );
	ASSERT_TRUE( dataByID );
	ASSERT_TRUE( dataByIndex );
	EXPECT_EQ( &byID.value(), &byIndex.value() );
	EXPECT_EQ( &dataByID.value(), &dataByIndex.value() );
	EXPECT_EQ( byID->header.uuid, gbpID );
	EXPECT_EQ( dataByID->ccyID, "GBP" );
	EXPECT_EQ( dataByID->name, "Pound Sterling" );
	EXPECT_FALSE( cache.getByIndex( "ccyID", "EUR" ) );
}

TEST( RefDataCacheTest, NonUniqueIndexReturnsEveryMatchingRecord )
{
	std::vector<Record<User>> records;
	records.push_back( makeUserRecord( 1, "alice", "FX" ) );
	records.push_back( makeUserRecord( 2, "bob", "Rates" ) );
	records.push_back( makeUserRecord( 3, "carol", "FX" ) );
	Cache<User> cache( std::move( records ) );

	const auto fxRecords = cache.getRecordsByNonUniqIndex( "tradingDesk", "FX" );
	const auto fxUsers = cache.getByNonUniqIndex( "tradingDesk", "FX" );

	ASSERT_EQ( fxRecords.size(), 2 );
	EXPECT_EQ( userIDs( fxUsers ), ( std::vector<std::string>{ "alice", "carol" } ) );
	EXPECT_TRUE( cache.getByNonUniqIndex( "tradingDesk", "Credit" ).empty() );
}

TEST( RefDataCacheTest, OptionalNonUniqueIndexUsesEmptyValueForMissingFields )
{
	std::vector<Record<User>> records;
	records.push_back( makeUserRecord( 1, "alice", std::nullopt ) );
	records.push_back( makeUserRecord( 2, "bob", "" ) );
	records.push_back( makeUserRecord( 3, "carol", "FX" ) );
	Cache<User> cache( std::move( records ) );

	EXPECT_EQ( userIDs( cache.getByNonUniqIndex( "tradingDesk", "" ) ), ( std::vector<std::string>{ "alice", "bob" } ) );
}

TEST( RefDataCacheTest, RejectsUnknownOrIncorrectIndexKinds )
{
	std::vector<Record<User>> records;
	records.push_back( makeUserRecord( 1, "alice", "FX" ) );
	Cache<User> cache( std::move( records ) );

	EXPECT_THROW( static_cast<void>( cache.getByIndex( "email", "user1@example.test" ) ), ARQException );
	EXPECT_THROW( static_cast<void>( cache.getByIndex( "tradingDesk", "FX" ) ), ARQException );
	EXPECT_THROW( static_cast<void>( cache.getByNonUniqIndex( "email", "user1@example.test" ) ), ARQException );
	EXPECT_THROW( static_cast<void>( cache.getByNonUniqIndex( "userID", "alice" ) ), ARQException );
}

TEST( RefDataCacheTest, RejectsDuplicateRecordUUIDs )
{
	std::vector<Record<Currency>> records;
	records.push_back( makeCurrencyRecord( 1, "GBP", "Pound Sterling" ) );
	records.push_back( makeCurrencyRecord( 1, "USD", "US Dollar" ) );

	EXPECT_THROW( Cache<Currency>( std::move( records ) ), ARQException );
}

TEST( RefDataCacheTest, RejectsDuplicateValuesInUniqueIndexes )
{
	std::vector<Record<Currency>> records;
	records.push_back( makeCurrencyRecord( 1, "GBP", "Pound Sterling" ) );
	records.push_back( makeCurrencyRecord( 2, "GBP", "Duplicate Pound" ) );

	EXPECT_THROW( Cache<Currency>( std::move( records ) ), ARQException );
}

TEST_F( RefDataRepositoryTest, LoadsLazilyAndReusesThePublishedCache )
{
	m_currencyState->m_records.push_back( makeCurrencyRecord( 1, "GBP", "Pound Sterling" ) );
	Repository repository( DATA_SOURCE_HANDLE );

	EXPECT_EQ( m_currencyState->m_fetchCount.load(), 0 );
	const auto first = repository.get<Currency>();
	const auto second = repository.get<Currency>();

	ASSERT_NE( first, nullptr );
	EXPECT_EQ( first, second );
	EXPECT_EQ( m_currencyState->m_fetchCount.load(), 1 );
	ASSERT_EQ( first->size(), 1 );
	ASSERT_TRUE( first->getByIndex( "ccyID", "GBP" ) );
	EXPECT_EQ( first->getByIndex( "ccyID", "GBP" )->name, "Pound Sterling" );
}

TEST_F( RefDataRepositoryTest, CachesEmptyLoadsWithoutRefetching )
{
	Repository repository( DATA_SOURCE_HANDLE );

	const auto first = repository.get<Currency>();
	const auto second = repository.get<Currency>();

	ASSERT_NE( first, nullptr );
	EXPECT_TRUE( first->empty() );
	EXPECT_EQ( first, second );
	EXPECT_EQ( m_currencyState->m_fetchCount.load(), 1 );
}

TEST_F( RefDataRepositoryTest, LoadsEntityTypesIndependently )
{
	m_currencyState->m_records.push_back( makeCurrencyRecord( 1, "GBP", "Pound Sterling" ) );
	m_userState->m_records.push_back( makeUserRecord( 2, "alice", "FX" ) );
	Repository repository( DATA_SOURCE_HANDLE );

	const auto currencies = repository.get<Currency>();
	EXPECT_EQ( m_currencyState->m_fetchCount.load(), 1 );
	EXPECT_EQ( m_userState->m_fetchCount.load(), 0 );

	const auto users = repository.get<User>();
	const auto usersAgain = repository.get<User>();

	ASSERT_EQ( currencies->size(), 1 );
	ASSERT_EQ( users->size(), 1 );
	EXPECT_EQ( users, usersAgain );
	EXPECT_EQ( m_currencyState->m_fetchCount.load(), 1 );
	EXPECT_EQ( m_userState->m_fetchCount.load(), 1 );
}

TEST_F( RefDataRepositoryTest, RetriesAfterASourceFailureWithoutPublishingAPartialCache )
{
	m_currencyState->m_records.push_back( makeCurrencyRecord( 1, "GBP", "Pound Sterling" ) );
	m_currencyState->m_failNextFetch = true;
	Repository repository( DATA_SOURCE_HANDLE );

	EXPECT_THROW( static_cast<void>( repository.get<Currency>() ), ARQException );
	EXPECT_EQ( m_currencyState->m_fetchCount.load(), 1 );

	const auto recovered = repository.get<Currency>();
	const auto cached = repository.get<Currency>();

	ASSERT_NE( recovered, nullptr );
	EXPECT_EQ( recovered, cached );
	EXPECT_EQ( recovered->size(), 1 );
	EXPECT_EQ( m_currencyState->m_fetchCount.load(), 2 );
}

TEST_F( RefDataRepositoryTest, ConcurrentFirstAccessFetchesAndPublishesOnce )
{
	constexpr size_t ReaderCount = 8;
	m_currencyState->m_records.push_back( makeCurrencyRecord( 1, "GBP", "Pound Sterling" ) );
	Repository repository( DATA_SOURCE_HANDLE );
	std::barrier startBarrier( ReaderCount );
	std::vector<std::shared_ptr<Cache<Currency>>> results( ReaderCount );

	{
		std::vector<std::jthread> readers;
		readers.reserve( ReaderCount );
		for( size_t i = 0; i < ReaderCount; ++i )
		{
			readers.emplace_back( [&repository, &results, &startBarrier, i]
			{
				startBarrier.arrive_and_wait();
				results[i] = repository.get<Currency>();
			} );
		}
	}

	ASSERT_NE( results.front(), nullptr );
	for( const auto& result : results )
		EXPECT_EQ( result, results.front() );
	EXPECT_EQ( m_currencyState->m_fetchCount.load(), 1 );
}
