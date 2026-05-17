#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ARQMarket/market_live.h>

using namespace ARQ;
using namespace ARQ::MD;
using namespace ARQ::Time;

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::NiceMock;

// ---------------------------------------------------------
// Helper Functions
// ---------------------------------------------------------

namespace
{

Record<FXRate> makeFXRecord( std::string id, double mid, uint64_t ts, bool active = true )
{
    Record<FXRate> rec;
    rec.header.id = std::move( id );
    rec.header.asofTs = DateTime( Microseconds( ts ) );
    rec.header.isActive = active;
    rec.data.mid = mid;
    return rec;
}

// ---------------------------------------------------------
// Mocks
// ---------------------------------------------------------

// A dummy subscription to return from the mock messaging service
class MockSubscription : public ISubscription
{
public:
    int64_t          getID() { return 0; }
    std::string_view getTopic() { return ""; }
    bool             isValid() { return true; }
    SubStats         getStats() { return SubStats{}; }
    void             unsubscribe() {}
    void             drain( const std::chrono::milliseconds ) {}
};

class MockMessagingService : public IMessagingService
{
public:
    MOCK_METHOD( void, publish, ( const std::string_view, const Message& ), ( override ) );
    MOCK_METHOD( std::unique_ptr<ISubscription>, subscribe, ( const std::string_view, std::shared_ptr<ISubscriptionHandler> ), ( override ) );
    MOCK_METHOD( void, registerEventCallback, ( const MessagingEventCallbackFunc& ), ( override ) );
    MOCK_METHOD( GlobalStats, getStats, ( ), ( const, override ) );
};

class MockMarketSource : public IMarketSource
{
public:
    MOCK_METHOD( RecordCollection, load, ( const std::string_view, const TIDSet& ), ( override ) );
    MOCK_METHOD( void, save, ( const std::string_view, const RecordCollection& ), ( override ) );
};

class MockOffsetSource : public IStreamOffsetSource
{
public:
    MOCK_METHOD( void, saveOffsets, ( const std::string_view, const StreamTopicPartitionOffsets& ), ( override ) );
    MOCK_METHOD( std::optional<StreamTopicPartitionOffsets>, getOffsets, ( const std::string_view ), ( override ) );
};

// Mock the specific type handler for MarketUpdateBatch
class MockMarketUpdateBatchHandler : public ISerialisableType<MarketUpdateBatch>
{
public:
    MOCK_METHOD( Buffer, serialise, ( const MarketUpdateBatch& ), ( const, override ) );
    MOCK_METHOD( void, deserialise, ( const BufferView, MarketUpdateBatch& ), ( const, override ) );
};

}

// ---------------------------------------------------------
// Test Fixture
// ---------------------------------------------------------

class LiveMarketUpdaterTest : public ::testing::Test
{
protected:
    std::shared_ptr<Market>                         market;
    std::shared_ptr<NiceMock<MockMessagingService>> mockMsgSvc;
    std::shared_ptr<NiceMock<MockMarketSource>>     mockMarketSrc;
    std::shared_ptr<NiceMock<MockOffsetSource>>     mockOffsetSrc;

    // We keep a raw pointer to our mock handler so we can set expectations,
    // while the real Serialiser takes ownership of the unique_ptr.
    MockMarketUpdateBatchHandler* mockBatchHandler;

    // Helper state to control the mock handler's behavior
    MarketUpdateBatch                             nextBatchToReturn;
    bool                                          shouldThrowDuringDeserialise = false;

    // Capture the handler so we can simulate incoming network messages
    std::shared_ptr<ISubscriptionHandler>         activeHandler;

    void SetUp() override
    {
        try
        {
            market        = std::make_shared<Market>();
            mockMsgSvc    = std::make_shared<NiceMock<MockMessagingService>>();
            mockMarketSrc = std::make_shared<NiceMock<MockMarketSource>>();
            mockOffsetSrc = std::make_shared<NiceMock<MockOffsetSource>>();

            // Intercept subscribe() to capture the handler and return a dummy sub
            ON_CALL( *mockMsgSvc, subscribe( _, _ ) )
                .WillByDefault( Invoke( [this] ( const std::string_view, std::shared_ptr<ISubscriptionHandler> handler )
            {
                this->activeHandler = handler;
                return std::make_unique<MockSubscription>();
            } ) );

            // --- Serialiser Setup ---
            auto realSerialiser = std::make_shared<Serialiser>();
            auto handlerMock = std::make_unique<NiceMock<MockMarketUpdateBatchHandler>>();
            mockBatchHandler = handlerMock.get();

            // Set up the default mock behavior using an Invoke lambda that reads our fixture state
            ON_CALL( *mockBatchHandler, deserialise( _, _ ) )
                .WillByDefault( Invoke( [this] ( const BufferView, MarketUpdateBatch& objOut )
            {
                if( this->shouldThrowDuringDeserialise )
                    throw ARQException( "Mock deserialisation failure" );

                objOut = this->nextBatchToReturn;
            } ) );

            realSerialiser->registerHandler<MarketUpdateBatch>( std::move( handlerMock ) );

            // --- Factory Injection ---
            MessagingServiceFactory::inst().addCustomService( "MOCK_NATS", mockMsgSvc );
            MarketSourceFactory::inst().addCustomSource( "MOCK_DB", mockMarketSrc );
            StreamOffsetSourceFactory::inst().addCustomSource( "MOCK_DB", mockOffsetSrc );

            // Serialiser may have been created by other tests so make sure to delete first
            try
            {
                SerialiserFactory::inst().delCustomSerialiser( SerialiserFactory::SerialiserImpl::Protobuf );
            }
            catch( ... ) {}
            SerialiserFactory::inst().addCustomSerialiser( SerialiserFactory::SerialiserImpl::Protobuf, realSerialiser );
        }
        catch( ARQException& e )
        {
            throw std::runtime_error( e.what() );
        }
    }

    void TearDown() override
    {
        MessagingServiceFactory::inst().delCustomService( "MOCK_NATS" );
        MarketSourceFactory::inst().delCustomSource( "MOCK_DB" );
        StreamOffsetSourceFactory::inst().delCustomSource( "MOCK_DB" );
        SerialiserFactory::inst().delCustomSerialiser( SerialiserFactory::SerialiserImpl::Protobuf );
    }
};

// ---------------------------------------------------------
// Tests
// ---------------------------------------------------------

TEST_F( LiveMarketUpdaterTest, SubscribesToCorrectTopic )
{
    EXPECT_CALL( *mockMsgSvc, subscribe( std::string_view( "ARQ.MktData.Updates.PROD_FX" ), _ ) )
        .Times( 1 );

    auto updater = LiveMarketUpdater::create( LiveMarketUpdater::Params{
        market, "", "MOCK_NATS", MarketName( "PROD_FX" ), TIDSet(), TIDSet()
    } );
    updater->start();
}

TEST_F( LiveMarketUpdaterTest, StartsInLiveModeWithoutBaseline )
{
    auto updater = LiveMarketUpdater::create( LiveMarketUpdater::Params{
        market, "", "MOCK_NATS", MarketName( "PROD_FX" ), TIDSet(), TIDSet()
    } );
    updater->start();

    ASSERT_NE( activeHandler, nullptr ); // Verifies it connected
}

TEST_F( LiveMarketUpdaterTest, FiltersByMsgTIDSetInLiveMode )
{
    // Setup TIDSet to only allow GBP
    TIDSet msgFilter;
    msgFilter.insert( TID( Type::FXR, "GBP" ) );
    
    auto updater = LiveMarketUpdater::create( LiveMarketUpdater::Params{
        market, "", "MOCK_NATS", MarketName( "PROD_FX" ), TIDSet(), msgFilter
    } );
    updater->start();

    // Prepare a mocked network batch containing both EUR and GBP
    this->nextBatchToReturn.records.get<Record<FXRate>>().push_back( makeFXRecord( "EUR", 1.08, 100 ) );
    this->nextBatchToReturn.records.get<Record<FXRate>>().push_back( makeFXRecord( "GBP", 1.29, 100 ) );
    this->nextBatchToReturn.offsets.emplace( std::make_pair( "ARQ.MktData.Updates.FXR", 0 ), 10 );

    // Fire the NATS message
    activeHandler->onMsg( Message{} );

    // Assert: GBP was applied, EUR was filtered out
    auto snap = market->snapshot();
    EXPECT_TRUE( snap->get<FXRate>( "GBP" ) );
    EXPECT_FALSE( snap->get<FXRate>( "EUR" ) );
}

TEST_F( LiveMarketUpdaterTest, RejectsStaleOffsets )
{
    
    auto updater = LiveMarketUpdater::create( LiveMarketUpdater::Params{
        market, "", "MOCK_NATS", MarketName( "PROD_FX" ), TIDSet(), TIDSet()
    } );
    updater->start();

    // 1. Send Batch 1 (Offset 100)
    this->nextBatchToReturn.offsets.emplace( std::make_pair( "ARQ.MktData.Updates.FXR", 0 ), 100 );
    this->nextBatchToReturn.records.get<Record<FXRate>>().push_back( makeFXRecord( "EUR", 1.08, 100 ) );
    activeHandler->onMsg( Message{} );

    EXPECT_DOUBLE_EQ( market->snapshot()->get<FXRate>( "EUR" )->data.mid, 1.08 );

    // 2. Send Batch 2 (Stale Offset 50, wildly different price)
    this->nextBatchToReturn.offsets.clear();
    this->nextBatchToReturn.offsets.emplace( std::make_pair( "ARQ.MktData.Updates.FXR", 0 ), 50 );
    this->nextBatchToReturn.records.get<Record<FXRate>>().clear();
    this->nextBatchToReturn.records.get<Record<FXRate>>().push_back( makeFXRecord( "EUR", 9.99, 200 ) );
    activeHandler->onMsg( Message{} );

    // 3. Assert: The stale batch was completely ignored
    EXPECT_DOUBLE_EQ( market->snapshot()->get<FXRate>( "EUR" )->data.mid, 1.08 );
}

TEST_F( LiveMarketUpdaterTest, BuffersAndReconcilesDuringStartup )
{
    auto updater = LiveMarketUpdater::create( LiveMarketUpdater::Params{
        market, "MOCK_DB", "MOCK_NATS", MarketName( "PROD_FX" ), TIDSet(), TIDSet()
    } );

    // 1. Setup the baseline data (Offset 100, Price 1.05)
    StreamTopicPartitionOffsets baselineOffsets;
    baselineOffsets.emplace( std::make_pair( "ARQ.MktData.Updates.FXR", 0 ), 100 );
    EXPECT_CALL( *mockOffsetSrc, getOffsets( _ ) ).WillOnce( Return( baselineOffsets ) );

    RecordCollection baselineRecords;
    baselineRecords.get<Record<FXRate>>().push_back( makeFXRecord( "EUR", 1.05, 50 ) );

    // 2. THE TRICK: Intercept the DB load. While loading, fire a NATS message into the buffer!
    EXPECT_CALL( *mockMarketSrc, load( _, _ ) ).WillOnce( Invoke( [&] ( const std::string_view, const TIDSet& )
    {
        // Setup the incoming NATS payload (Offset 101, Price 1.08)
        this->nextBatchToReturn.offsets.emplace( std::make_pair( "ARQ.MktData.Updates.FXR", 0 ), 101 );
        this->nextBatchToReturn.records.get<Record<FXRate>>().push_back( makeFXRecord( "EUR", 1.08, 150 ) );

        activeHandler->onMsg( Message{} );

        return baselineRecords;
    } ) );

    // 3. Execute start
    updater->start();

    // 4. Assert: Market processed baseline (1.05) then successfully reconciled buffer (1.08)
    auto snap = market->snapshot();
    ASSERT_TRUE( snap->get<FXRate>( "EUR" ) );
    EXPECT_DOUBLE_EQ( snap->get<FXRate>( "EUR" )->data.mid, 1.08 );
}

TEST_F( LiveMarketUpdaterTest, HandlesDeserialisationFailureGracefully )
{
    auto updater = LiveMarketUpdater::create( LiveMarketUpdater::Params{
        market, "", "MOCK_NATS", MarketName( "PROD_FX" ), TIDSet(), TIDSet()
    } );
    updater->start();

    EXPECT_CALL( *mockBatchHandler, deserialise( _, _ ) ).Times( 1 );

    // Force an exception during deserialise
    this->shouldThrowDuringDeserialise = true;

    // We EXPECT_NO_THROW because the catch block inside onMsg() must swallow it
    EXPECT_NO_THROW( {
        activeHandler->onMsg( Message{} );
    } );

    // Assert: Market remains completely untouched
    EXPECT_FALSE( market->snapshot()->get<FXRate>( "EUR" ) );
}