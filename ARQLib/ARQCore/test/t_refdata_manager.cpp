#include <ARQCore/refdata_manager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <t_ARQ/mock_refdata_source_interface.h>

using namespace ARQ;
using namespace ARQ::RDEntities;
using ::testing::_;
using ::testing::Return;
using ::testing::Throw;
using ::testing::NiceMock;

class RefDataManagerTests : public ::testing::Test
{
protected:
    // Use a NiceMock to avoid warnings about uninteresting calls
    std::shared_ptr<NiceMock<MockRefDataSource>> mockSource;
    std::unique_ptr<RefDataManager> manager;

    void SetUp() override
    {
        mockSource = std::make_shared<NiceMock<MockRefDataSource>>();
        RefDataSourceFactory::addCustomSource( "TestRD", mockSource );
        // Instantiate the manager with our mock data source
        manager = std::make_unique<RefDataManager>( "TestRD" );
    }

    void TearDown() override
    {
        RefDataSourceFactory::delCustomSource( "TestRD" );
    }
};

TEST_F( RefDataManagerTests, InitiallyHasNoData )
{
    // A fresh manager should not have any caches loaded yet.
    EXPECT_FALSE( manager->hasCurrencies() );
    EXPECT_FALSE( manager->hasUsers() );
}

TEST_F( RefDataManagerTests, FirstGetTriggersInitialLoad )
{
    // Expect that the first call to Currencies() will trigger a fetch.
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{} ) );

    auto cache = manager->Currencies(); // This should trigger the load
    EXPECT_TRUE( manager->hasCurrencies() );
    ASSERT_NE( cache, nullptr );
}

TEST_F( RefDataManagerTests, SecondGetDoesNotTriggerLoad )
{
    // After the first load, subsequent gets should not hit the database.
    // We set the expectation for the first call only.
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{} ) )
        .RetiresOnSaturation(); // This expectation is only valid once

    auto cache = manager->Currencies(); // First call, triggers fetch
    cache = manager->Currencies();      // Second call, should use cache, no fetch
    cache = manager->Currencies();      // Third call, should use cache, no fetch
}

TEST_F( RefDataManagerTests, ReloadFetchesNewData )
{
    // Setup initial data
    Currency initialCcy;
    initialCcy.ccyID = "USD";
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{ initialCcy } ) );

    auto initialCache = manager->Currencies();
    ASSERT_TRUE( initialCache->get( "USD" ).has_value() );

    // Setup data for the reload
    Currency reloadedCcy;
    reloadedCcy.ccyID = "EUR"; // New data
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{ reloadedCcy } ) );

    // Act
    manager->reloadCurrencies();

    // Assert
    auto reloadedCache = manager->Currencies();
    EXPECT_FALSE( reloadedCache->get( "USD" ).has_value() ); // Old data is gone
    ASSERT_TRUE( reloadedCache->get( "EUR" ).has_value() );  // New data is present
}

TEST_F( RefDataManagerTests, InsertHappyPath )
{
    Currency ccyToInsert;
    ccyToInsert.ccyID = "JPY";

    // Expect the insert method on the source to be called.
    EXPECT_CALL( *mockSource, upsertCurrencies( _ ) ).Times( 1 );
    // Expect a reload to happen after the insert.
    EXPECT_CALL( *mockSource, fetchCurrencies() ).Times( 1 );

    ASSERT_NO_THROW( manager->insertCurrency( ccyToInsert, RefDataManager::StaleCheck::NONE ) );
}

TEST_F( RefDataManagerTests, InsertStaleRecordThrowsWithCacheCheck )
{
    const Time::DateTime baseTs = Time::DateTime::nowUTC();

    // 1. Pre-populate the cache with a "newer" item
    Currency cachedCcy;
    cachedCcy.ccyID = "USD";
    cachedCcy._lastUpdatedTs = baseTs + Time::Seconds( 100 ); // Newer timestamp
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{ cachedCcy } ) );
    auto cachePtr = manager->Currencies(); // Trigger initial load

    // 2. Prepare an item to insert that is "stale"
    Currency staleCcy;
    staleCcy.ccyID = "USD";
    staleCcy._lastUpdatedTs = baseTs + Time::Seconds( 50 ); // Older timestamp

    // 3. Assert
    // Expect an exception because the stale check is active by default.
    EXPECT_THROW( manager->insertCurrency( staleCcy ), ARQException );
    // Expect that the data source's insert method is NEVER called.
    EXPECT_CALL( *mockSource, upsertCurrencies( _ ) ).Times( 0 );
}

TEST_F( RefDataManagerTests, InsertStaleRecordSucceedsWithNoCheck )
{
    const Time::DateTime baseTs = Time::DateTime::nowUTC();

    // 1. Pre-populate the cache with a "newer" item
    Currency cachedCcy;
    cachedCcy.ccyID = "USD";
    cachedCcy._lastUpdatedTs = baseTs + Time::Seconds( 100 );
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{ cachedCcy } ) );
    auto cachePtr = manager->Currencies();

    // 2. Prepare a stale item
    Currency staleCcy;
    staleCcy.ccyID = "USD";
    staleCcy._lastUpdatedTs = baseTs + Time::Seconds( 50 );

    // 3. Assert
    // Expect no exception, and expect insert to be called because check is NONE.
    EXPECT_CALL( *mockSource, upsertCurrencies( _ ) ).Times( 1 );
    // The post-insert reload will also happen
    EXPECT_CALL( *mockSource, fetchCurrencies() ).Times( 1 );

    ASSERT_NO_THROW( manager->insertCurrency( staleCcy, RefDataManager::StaleCheck::NONE ) );
}

TEST_F( RefDataManagerTests, InsertWithDbStaleCheckForcesReload )
{
    // This tests the StaleCheck::USING_DB logic.

    const Time::DateTime baseTs = Time::DateTime::nowUTC();

    // 1. Initial cache load
    Currency cachedCcy;
    cachedCcy.ccyID = "USD";
    cachedCcy._lastUpdatedTs = baseTs + Time::Seconds( 50 );
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{ cachedCcy } ) );
    auto cachePtr = manager->Currencies();

    // 2. Setup a "fresher" version that only exists in the DB
    Currency dbFresherCcy;
    dbFresherCcy.ccyID = "USD";
    dbFresherCcy._lastUpdatedTs = baseTs + Time::Seconds( 100 );
    // This is the expectation for the forced reload inside insert()
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{ dbFresherCcy } ) );

    // 3. Try to insert something that is stale compared to the DB version
    Currency itemToInsert;
    itemToInsert.ccyID = "USD";
    itemToInsert._lastUpdatedTs = baseTs + Time::Seconds( 75 ); // Newer than cache, older than DB

    // 4. Assert
    // Expect an exception because the forced reload will find the 100s timestamp.
    EXPECT_THROW( manager->insertCurrency( itemToInsert, RefDataManager::StaleCheck::USING_DB ), ARQException );
    // Expect insert never to be called.
    EXPECT_CALL( *mockSource, upsertCurrencies( _ ) ).Times( 0 );
}

TEST_F( RefDataManagerTests, GetReturnsReferenceToDataInSnapshot )
{
    const Time::DateTime baseTs = Time::DateTime::nowUTC();

    Currency ccy;
    ccy.ccyID = "USD";
    ccy._lastUpdatedTs = baseTs + Time::Seconds( 100 );
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{ ccy } ) );

    // 1. Get a handle to the cache. This is a shared_ptr to the RefDataCache.
    auto cachePtr = manager->Currencies();
    ASSERT_NE( cachePtr, nullptr );

    // 2. Get a reference to the item inside the cache's map.
    auto refOpt = cachePtr->get( "USD" );
    ASSERT_TRUE( refOpt.has_value() );

    // 'refToCcy' is a reference to memory managed by 'cachePtr'.
    const Currency& refToCcy = refOpt->get();
    EXPECT_EQ( refToCcy.ccyID, "USD" );
    EXPECT_EQ( refToCcy._lastUpdatedTs, ccy._lastUpdatedTs );

    // 3. Reload the RefDataManagers cache of currencies to remove USD
    Currency newCcy;
    newCcy.ccyID = "EUR";
    newCcy._lastUpdatedTs = baseTs + Time::Seconds( 100 );
    EXPECT_CALL( *mockSource, fetchCurrencies() )
        .WillOnce( Return( std::vector<Currency>{ newCcy } ) );

    manager->reloadCurrencies();

    // 'refToCcy' should be uneffected since cachePtr is still holding onto the original map of data
    EXPECT_EQ( refToCcy.ccyID, "USD" );
    EXPECT_EQ( refToCcy._lastUpdatedTs, ccy._lastUpdatedTs );

    auto newRefOpt = cachePtr->get( "USD" );
    ASSERT_TRUE( newRefOpt.has_value() );

    const Currency& newRefToCcy = newRefOpt->get();
    EXPECT_EQ( newRefToCcy.ccyID, "USD" );
    EXPECT_EQ( newRefToCcy._lastUpdatedTs, ccy._lastUpdatedTs );

    // 4. Get a new currency cache
    auto newCachePtr = manager->Currencies();
    auto newCacheEUR = newCachePtr->get( "EUR" );
    ASSERT_TRUE( newCacheEUR.has_value() );
}