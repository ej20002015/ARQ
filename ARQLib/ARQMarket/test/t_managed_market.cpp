#include <ARQMarket/managed_market.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ARQCore/mktdata_source.h>
#include <ARQMarket/market.h>
#include <t_ARQ/mock_mktdata_source_interface.h>

#include <random>
#include <atomic>
#include <future>

using namespace ARQ;

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::_;
using ::testing::InSequence;

class TestSubscriber : public Mkt::Subscriber
{
public:
    TestSubscriber( const std::string& name ) : m_name( name ) {}
    const std::string_view description() const override { return m_name; }

    // Test helpers
    std::atomic<int> mktObjUpdateCount{ 0 };
    std::atomic<int> fxRateUpdateCount{ 0 };
    std::atomic<int> eqPriceUpdateCount{ 0 };

    std::vector<std::pair<MDEntities::Type, std::string>> receivedMktObjUpdates;
    std::vector<MDEntities::FXRate> receivedFXRateUpdates;
    std::vector<MDEntities::EQPrice> receivedEQPriceUpdates;

    mutable std::mutex updatesMutex;

private:
    std::string m_name;
};

class ManagedMarketTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockSource = std::make_shared<NiceMock<MockMktDataSource>>();
        MktDataSourceFactory::addCustomSource( "TestSource", mockSource );

        managedMarket = std::make_unique<Mkt::ManagedMarket>( "TestSource", Mkt::Name::LIVE );

        subscriber1 = std::make_shared<TestSubscriber>( "Subscriber1" );
        subscriber2 = std::make_shared<TestSubscriber>( "Subscriber2" );

        setupCallbacks();
    }

    void TearDown() override
    {
        subscriber1.reset();
        subscriber2.reset();
        managedMarket.reset();

        MktDataSourceFactory::delCustomSource( "TestSource" );
    }

    void setupCallbacks()
    {
        subscriber1->setOnMktObjUpdateFunc( [this] ( const MDEntities::Type type, const std::string_view ID )
        {
            subscriber1->mktObjUpdateCount++;
            std::lock_guard<std::mutex> lock( subscriber1->updatesMutex );
            subscriber1->receivedMktObjUpdates.emplace_back( type, std::string( ID ) );
        } );

        subscriber1->setOnFXRateUpdateFunc( [this] ( const MDEntities::FXRate& rate )
        {
            subscriber1->fxRateUpdateCount++;
            std::lock_guard<std::mutex> lock( subscriber1->updatesMutex );
            subscriber1->receivedFXRateUpdates.push_back( rate );
        } );

        subscriber1->setOnEQPriceUpdateFunc( [this] ( const MDEntities::EQPrice& price )
        {
            subscriber1->eqPriceUpdateCount++;
            std::lock_guard<std::mutex> lock( subscriber1->updatesMutex );
            subscriber1->receivedEQPriceUpdates.push_back( price );
        } );

        subscriber2->setOnMktObjUpdateFunc( [this] ( const MDEntities::Type type, const std::string_view ID )
        {
            subscriber2->mktObjUpdateCount++;
            std::lock_guard<std::mutex> lock( subscriber2->updatesMutex );
            subscriber2->receivedMktObjUpdates.emplace_back( type, std::string( ID ) );
        } );

        subscriber2->setOnFXRateUpdateFunc( [this] ( const MDEntities::FXRate& rate )
        {
            subscriber2->fxRateUpdateCount++;
            std::lock_guard<std::mutex> lock( subscriber2->updatesMutex );
            subscriber2->receivedFXRateUpdates.push_back( rate );
        } );

        subscriber2->setOnEQPriceUpdateFunc( [this] ( const MDEntities::EQPrice& price )
        {
            subscriber2->eqPriceUpdateCount++;
            std::lock_guard<std::mutex> lock( subscriber2->updatesMutex );
            subscriber2->receivedEQPriceUpdates.push_back( price );
        } );
    }

    MDEntities::FXRate createFXRate( const std::string& id, double bid, double ask, const std::string& source = "TestSource" )
    {
        MDEntities::FXRate rate;
        rate.ID = id;
        rate.bid = bid;
        rate.ask = ask;
        rate.mid = ( bid + ask ) / 2.0;
        rate.source = source;
        rate.asofTs = Time::DateTime::nowUTC();
        rate._lastUpdatedTs = Time::DateTime::nowUTC();
        return rate;
    }

    MDEntities::EQPrice createEQPrice( const std::string& id, double price, const std::string& source = "TestSource" )
    {
        MDEntities::EQPrice eqPrice;
        eqPrice.ID = id;
        eqPrice.last = price;
        eqPrice.source = source;
        eqPrice.asofTs = Time::DateTime::nowUTC();
        eqPrice._lastUpdatedTs = Time::DateTime::nowUTC();
        return eqPrice;
    }

protected:
    std::shared_ptr<NiceMock<MockMktDataSource>> mockSource;
    std::unique_ptr<Mkt::ManagedMarket> managedMarket;
    std::shared_ptr<TestSubscriber> subscriber1;
    std::shared_ptr<TestSubscriber> subscriber2;
};

// Test basic subscription and loading
TEST_F( ManagedMarketTest, BasicSubscriptionAndLoad )
{
    auto fxRate = createFXRate( "EUR/USD", 1.05, 1.06 );

    EXPECT_CALL( *mockSource, fetchFXRates( Mkt::Name::LIVE.str() ) )
        .WillOnce( Return( std::vector<MDEntities::FXRate>{fxRate} ) );

    managedMarket->subscribeAndLoad( subscriber1, { MDEntities::Type::FXR } );

	EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 1 ); // Load triggers an update
    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 1 );

    // Verify the data was loaded into the market
    auto retrievedRate = managedMarket->getFXRate( "EUR/USD" );
    ASSERT_TRUE( retrievedRate.has_value() );
    EXPECT_EQ( retrievedRate->ID, "EUR/USD" );
    EXPECT_DOUBLE_EQ( retrievedRate->bid, 1.05 );
    EXPECT_DOUBLE_EQ( retrievedRate->ask, 1.06 );
}

// Test subscription without loading
TEST_F( ManagedMarketTest, SubscriptionWithoutLoad )
{
    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR } );

    // Market should be empty
    auto retrievedRate = managedMarket->getFXRate( "EUR/USD" );
    EXPECT_FALSE( retrievedRate.has_value() );
}

// Test market data updates and notifications
TEST_F( ManagedMarketTest, MarketDataUpdatesAndNotifications )
{
    // Subscribe to FX rates
    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR } );
    managedMarket->subscribe( subscriber2, { MDEntities::Type::FXR } );

    auto fxRate1 = createFXRate( "GBP/USD", 1.25, 1.26 );
    auto fxRate2 = createFXRate( "EUR/USD", 1.05, 1.06 );

    // Send updates
    managedMarket->onFXRateUpdate( fxRate1 );
    managedMarket->onFXRateUpdate( fxRate2 );

    // Both subscribers should receive updates
    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 2 );
    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 2 );
    EXPECT_EQ( subscriber2->mktObjUpdateCount.load(), 2 );
    EXPECT_EQ( subscriber2->fxRateUpdateCount.load(), 2 );

    // Verify the data is in the market
    auto retrievedRate1 = managedMarket->getFXRate( "GBP/USD" );
    auto retrievedRate2 = managedMarket->getFXRate( "EUR/USD" );
    ASSERT_TRUE( retrievedRate1.has_value() );
    ASSERT_TRUE( retrievedRate2.has_value() );
}

// Test selective subscription by ID
TEST_F( ManagedMarketTest, SelectiveSubscriptionById )
{
    // Subscribe to specific FX rate
    managedMarket->subscribe( subscriber1, { {MDEntities::Type::FXR, "EUR/USD"} } );
    managedMarket->subscribe( subscriber2, { {MDEntities::Type::FXR, "GBP/USD"} } );

    auto fxRateEUR = createFXRate( "EUR/USD", 1.05, 1.06 );
    auto fxRateGBP = createFXRate( "GBP/USD", 1.25, 1.26 );
    auto fxRateJPY = createFXRate( "USD/JPY", 150.0, 150.1 );

    managedMarket->onFXRateUpdate( fxRateEUR );
    managedMarket->onFXRateUpdate( fxRateGBP );
    managedMarket->onFXRateUpdate( fxRateJPY );

    // Subscriber1 should only receive EUR/USD updates
    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 1 );
    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 1 );

    // Subscriber2 should only receive GBP/USD updates
    EXPECT_EQ( subscriber2->mktObjUpdateCount.load(), 1 );
    EXPECT_EQ( subscriber2->fxRateUpdateCount.load(), 1 );

    // Verify received updates
    {
        std::lock_guard<std::mutex> lock( subscriber1->updatesMutex );
        ASSERT_EQ( subscriber1->receivedFXRateUpdates.size(), 1 );
        EXPECT_EQ( subscriber1->receivedFXRateUpdates[0].ID, "EUR/USD" );
    }

    {
        std::lock_guard<std::mutex> lock( subscriber2->updatesMutex );
        ASSERT_EQ( subscriber2->receivedFXRateUpdates.size(), 1 );
        EXPECT_EQ( subscriber2->receivedFXRateUpdates[0].ID, "GBP/USD" );
    }
}

// Test unsubscription
TEST_F( ManagedMarketTest, Unsubscription )
{
    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR } );

    auto fxRate = createFXRate( "EUR/USD", 1.05, 1.06 );
    managedMarket->onFXRateUpdate( fxRate );

    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 1 );

    // Unsubscribe
    Mkt::ConsolidatingTIDSet unsubList{ MDEntities::Type::FXR };
    managedMarket->unsubscribe( subscriber1, unsubList );

    // Send another update
    auto fxRate2 = createFXRate( "GBP/USD", 1.25, 1.26 );
    managedMarket->onFXRateUpdate( fxRate2 );

    // Should not receive the second update
    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 1 );
}

// Test full unsubscription (no subscription list)
TEST_F( ManagedMarketTest, FullUnsubscription )
{
    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR } );
    managedMarket->subscribe( subscriber1, { MDEntities::Type::EQP } );

    auto fxRate = createFXRate( "EUR/USD", 1.05, 1.06 );
    auto eqPrice = createEQPrice( "AAPL", 150.0 );

    managedMarket->onFXRateUpdate( fxRate );
    managedMarket->onEQPriceUpdate( eqPrice );

    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 2 );

    // Full unsubscription
    managedMarket->unsubscribe( subscriber1 );

    // Send more updates
    managedMarket->onFXRateUpdate( fxRate );
    managedMarket->onEQPriceUpdate( eqPrice );

    // Should not receive any more updates
    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 2 );
}

// Test mixed entity types
TEST_F( ManagedMarketTest, MixedEntityTypes )
{
    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR, MDEntities::Type::EQP } );

    auto fxRate = createFXRate( "EUR/USD", 1.05, 1.06 );
    auto eqPrice = createEQPrice( "AAPL", 150.0 );

    managedMarket->onFXRateUpdate( fxRate );
    managedMarket->onEQPriceUpdate( eqPrice );

    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 2 );
    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 1 );
    EXPECT_EQ( subscriber1->eqPriceUpdateCount.load(), 1 );
}

// Test that older updates are rejected
TEST_F( ManagedMarketTest, OlderUpdatesRejected )
{
    auto newerRate = createFXRate( "EUR/USD", 1.05, 1.06 );
    newerRate.asofTs = Time::DateTime::nowUTC();

    auto olderRate = createFXRate( "EUR/USD", 1.07, 1.08 );
    olderRate.asofTs = Time::DateTime::nowUTC() - Time::Hours( 1 );

    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR } );

    // Send newer update first
    managedMarket->onFXRateUpdate( newerRate );

    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 1 );

    // Send older update - should be rejected
    managedMarket->onFXRateUpdate( olderRate );

    // Should still only have 1 update
    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 1 );

    // Market should still have newer rate
    auto retrievedRate = managedMarket->getFXRate( "EUR/USD" );
    ASSERT_TRUE( retrievedRate.has_value() );
    EXPECT_DOUBLE_EQ( retrievedRate->bid, 1.05 ); // newer rate values
}

// Test ConsolidatingTIDSet functionality
TEST_F( ManagedMarketTest, ConsolidatingTIDSetBehavior )
{
    // Test type-level subscription (all IDs of that type)
    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR } );

    // Test specific ID subscription
    managedMarket->subscribe( subscriber2, { { MDEntities::Type::FXR, "EUR/USD" } } );

    auto fxRate1 = createFXRate( "EUR/USD", 1.05, 1.06 );
    auto fxRate2 = createFXRate( "GBP/USD", 1.25, 1.26 );

    managedMarket->onFXRateUpdate( fxRate1 );
    managedMarket->onFXRateUpdate( fxRate2 );

    // Subscriber1 should get both (subscribed to all FXR)
    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 2 );

    // Subscriber2 should get only EUR/USD
    EXPECT_EQ( subscriber2->fxRateUpdateCount.load(), 1 );
}

// Test concurrent operations
TEST_F( ManagedMarketTest, ConcurrentOperations )
{
    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR } );

    const int numUpdates = 100;
    std::atomic<int> updatesCompleted{ 0 };

    // Launch multiple threads sending updates
    std::vector<std::future<void>> futs;
    for( int i = 0; i < 4; ++i )
    {
        futs.emplace_back( std::async( std::launch::async, [&, i] ()
        {
            for( int j = 0; j < numUpdates / 4; ++j )
            {
                auto rate = createFXRate( "PAIR" + std::to_string( i ), 1.0 + j * 0.001, 1.001 + j * 0.001 );
                managedMarket->onFXRateUpdate( rate );
                updatesCompleted++;
            }
        } ) );
    }

    for( const auto& fut : futs )
        fut.wait_for( std::chrono::seconds( 1 ) );

    EXPECT_EQ( updatesCompleted.load(), numUpdates );
    EXPECT_EQ( subscriber1->receivedMktObjUpdates.size(), numUpdates);
    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), numUpdates );
}

// Test cleanup with expired weak_ptr
TEST_F( ManagedMarketTest, WeakPtrCleanup )
{
    {
        auto tempSubscriber = std::make_shared<TestSubscriber>( "TempSubscriber" );
        managedMarket->subscribe( tempSubscriber, { MDEntities::Type::FXR } );

        auto fxRate = createFXRate( "EUR/USD", 1.05, 1.06 );
        managedMarket->onFXRateUpdate( fxRate );
        // tempSubscriber goes out of scope here
    }

    // Send another update - should not crash even with expired weak_ptr
    auto fxRate2 = createFXRate( "GBP/USD", 1.25, 1.26 );
    managedMarket->onFXRateUpdate( fxRate2 );

    // Test should complete without crashing
    SUCCEED();
}

// Test automatic cleanup of expired subscribers
TEST_F( ManagedMarketTest, ExpiredSubscriberCleanup )
{
    // Subscribe a valid subscriber first
    managedMarket->subscribe( subscriber1, { MDEntities::Type::FXR } );
    EXPECT_TRUE( managedMarket->isSubscriber( subscriber1 ) );
    
    // Create and subscribe temporary subscribers that will expire
    std::weak_ptr<TestSubscriber> expiredSubscriber1Weak;
    std::weak_ptr<TestSubscriber> expiredSubscriber2Weak;
    
    {
        auto expiredSubscriber1 = std::make_shared<TestSubscriber>( "ExpiredSubscriber1" );
        auto expiredSubscriber2 = std::make_shared<TestSubscriber>( "ExpiredSubscriber2" );
        
        // Keep weak references to verify they expire
        expiredSubscriber1Weak = expiredSubscriber1;
        expiredSubscriber2Weak = expiredSubscriber2;
        
        // Subscribe the temporary subscribers
        managedMarket->subscribe( expiredSubscriber1, { MDEntities::Type::FXR } );
        managedMarket->subscribe( expiredSubscriber2, { MDEntities::Type::EQP } );
        
        // Verify all subscribers are registered
        EXPECT_TRUE( managedMarket->isSubscriber( expiredSubscriber1 ) );
        EXPECT_TRUE( managedMarket->isSubscriber( expiredSubscriber2 ) );
        EXPECT_FALSE( expiredSubscriber1Weak.expired() );
        EXPECT_FALSE( expiredSubscriber2Weak.expired() );
        
        // Subscribers go out of scope here
    }
    
    // Verify weak pointers are now expired but still in subscription map initially
    EXPECT_TRUE( expiredSubscriber1Weak.expired() );
    EXPECT_TRUE( expiredSubscriber2Weak.expired() );
    
    // The expired subscribers should still be in the map before cleanup
    EXPECT_TRUE( managedMarket->isSubscriber( expiredSubscriber1Weak ) );
    EXPECT_TRUE( managedMarket->isSubscriber( expiredSubscriber2Weak ) );
    
    // Send updates that should trigger cleanup of expired subscribers
    auto fxRate = createFXRate( "EUR/USD", 1.05, 1.06 );
    auto eqPrice = createEQPrice( "AAPL", 150.0 );
    
    // This should clean up both expired subscribers
    managedMarket->onFXRateUpdate( fxRate );
    
    // Verify both expired subscribers are now cleaned up
    EXPECT_FALSE( managedMarket->isSubscriber( expiredSubscriber1Weak ) );
    EXPECT_FALSE( managedMarket->isSubscriber( expiredSubscriber2Weak ) );
    
    // Valid subscriber should still be registered and receive updates
    EXPECT_TRUE( managedMarket->isSubscriber( subscriber1 ) );
    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 1 );
    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 1 );
    
    // Send more updates to verify cleanup worked and doesn't affect valid subscribers
    auto fxRate2 = createFXRate( "GBP/USD", 1.25, 1.26 );
    managedMarket->onFXRateUpdate( fxRate2 );
    
    EXPECT_TRUE( managedMarket->isSubscriber( subscriber1 ) );
    EXPECT_EQ( subscriber1->mktObjUpdateCount.load(), 2 );
    EXPECT_EQ( subscriber1->fxRateUpdateCount.load(), 2 );
}
