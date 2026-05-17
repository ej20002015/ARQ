#include <ARQMarket/market_builders.h>
#include <gtest/gtest.h>

#include <ARQCore/messaging_service.h>

using namespace ARQ;
using namespace ARQ::MD;
using namespace ARQ::Time;

// ---------------------------------------------------------
// Tests scaffolding
// ---------------------------------------------------------

namespace
{

// A lightweight mock to satisfy the MessagingServiceFactory.
// Because we never call updater->start() in these tests, 
// these methods are never actually invoked.
class MockMessagingService : public IMessagingService
{
public:
    void publish( const std::string_view topic, const Message& msg ) override {}

    std::unique_ptr<ISubscription> subscribe( const std::string_view topicPattern, std::shared_ptr<ISubscriptionHandler> subHandler ) override
    {
        return nullptr;
    }

    void registerEventCallback( const MessagingEventCallbackFunc& eventCallbackFunc ) override {}

    GlobalStats getStats() const override
    {
        return {}; // Return empty stats
    }
};

}

class LiveMarketBuilderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Inject the mock service so the LiveMarketUpdater constructor doesn't throw
        auto mockSvc = std::make_shared<MockMessagingService>();
        MessagingServiceFactory::inst().addCustomService( "MOCK_NATS_SVC", mockSvc );
    }

    void TearDown() override
    {
        // Clean up the singleton to prevent state leakage between tests
        MessagingServiceFactory::inst().delCustomService( "MOCK_NATS_SVC" );
    }
};

// ---------------------------------------------------------
// LiveMarketBuilder Tests
// ---------------------------------------------------------

TEST_F( LiveMarketBuilderTest, BuildsValidMarketAndUpdater )
{
    LiveMarketBuilder builder;

    // Use structured binding to unpack the pair
    auto [mkt, updater] = builder
        .fromSource( "MOCK_REDIS_SRC", MarketName( "PROD_FX" ) )
        .withMessagingFeed( "MOCK_NATS_SVC" )
        .build();

    // 1. Assert objects were actually created
    ASSERT_NE( mkt, nullptr );
    ASSERT_NE( updater, nullptr );

    // 2. Assert wiring: The updater must hold a reference to the EXACT SAME market
    EXPECT_EQ( updater->market(), mkt );
}

TEST_F( LiveMarketBuilderTest, BuildsWithTIDSetFilters )
{
    // Create a TIDSet to simulate filtering just GBP and EUR
    TIDSet filterSet;
    filterSet.insert( TID( Type::FXR, "GBP" ) );
    filterSet.insert( TID( Type::FXR, "EUR" ) );

    LiveMarketBuilder builder;

    auto [mkt, updater] = builder
        .fromSource( "MOCK_REDIS_SRC", MarketName( "PROD_FX", Date( Year( 2026 ), Month( 5 ), Day( 17 ) ) ) )
        // Pass the TIDSet into the messaging feed
        .withMessagingFeed( "MOCK_NATS_SVC", filterSet )
        .build();

    ASSERT_NE( mkt, nullptr );
    ASSERT_NE( updater, nullptr );
    EXPECT_EQ( updater->market(), mkt );
}

TEST_F( LiveMarketBuilderTest, BuildsWithoutBaselineSource )
{
    LiveMarketBuilder builder;

    // We should be able to build a completely empty market purely from a stream,
    // bypassing fromSource() entirely.
    auto [mkt, updater] = builder
        .withMessagingFeed( "MOCK_NATS_SVC", TIDSet(), MarketName( "NEW_LISTINGS" ) )
        .build();

    ASSERT_NE( mkt, nullptr );
    ASSERT_NE( updater, nullptr );
    EXPECT_EQ( updater->market(), mkt );
}

TEST_F( LiveMarketBuilderTest, BuildThrowsIfMessagingFeedNotSpecified )
{
    LiveMarketBuilder builder;

    // We supply a source and market name, but FORGET to call withMessagingFeed()
    builder.fromSource( "MOCK_REDIS_SRC", MarketName( "PROD_FX" ) );

    // Expect the builder to throw an ARQException when build() is called
    EXPECT_THROW( builder.build(), ARQException );
}

TEST_F( LiveMarketBuilderTest, BuildThrowsIfMarketNameNotSpecified )
{
    LiveMarketBuilder builder;

    // We supply a messaging feed, but FORGET to supply a MarketName in either 
    // fromSource() or withMessagingFeed()
    builder.withMessagingFeed( "MOCK_NATS_SVC" ); // Uses default empty MarketName()

    // Expect the builder to throw an ARQException due to missing name
    EXPECT_THROW( builder.build(), ARQException );
}

TEST_F( LiveMarketBuilderTest, BuildSucceedsIfMarketNameProvidedOnlyInMessagingFeed )
{
    LiveMarketBuilder builder;

    // Validates the flip side of the condition: 
    // If no fromSource() is called, providing the MarketName in withMessagingFeed() 
    // MUST satisfy the requirement and not throw.
    EXPECT_NO_THROW( {
        builder.withMessagingFeed( "MOCK_NATS_SVC", TIDSet(), MarketName( "PROD_FX" ) )
               .build();
    } );
}