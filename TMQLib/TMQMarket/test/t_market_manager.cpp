#include <TMQMarket/market_manager.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <TMQUtils/error.h>
#include <chrono>

using namespace TMQ::Mkt;
using namespace TMQ::Time;
using namespace std::chrono;
using namespace std::chrono_literals;

// Mock MktDataSource for testing purposes
class MockMktDataSource : public TMQ::MktDataSource
{
public:
    MOCK_METHOD( std::vector<TMQ::MktDataSource::FetchData>, fetchLatest, ( const std::string_view context ), ( override ) );
    MOCK_METHOD( void, insert, ( const std::string_view context, const std::vector<TMQ::MktDataSource::InsertData>& insData ), ( override ) );
};

class MarketManagerTests : public ::testing::Test
{
protected:
    MarketManager& m_manager = MarketManager::inst();
    std::shared_ptr<MockMktDataSource> m_mockSource;

    void SetUp() override
    {
        m_mockSource = std::make_shared<MockMktDataSource>();
        m_manager.clear();
    }
};

TEST_F( MarketManagerTests, CreateAndGetMarket )
{
    const std::string handle = "TestMarket1";
    const Context ctx = Context::LIVE;

    EXPECT_CALL( *m_mockSource, fetchLatest( testing::_) )
        .WillOnce( testing::Return( std::vector<TMQ::MktDataSource::FetchData>{} ) );

    std::weak_ptr<Market> marketWp = m_manager.create( handle, ctx, m_mockSource );
    ASSERT_FALSE( marketWp.expired() ) << "Market should be created and valid";

    auto retrievedMarketOpt = m_manager.get( handle );
    ASSERT_TRUE( retrievedMarketOpt.has_value() ) << "Market should be retrievable";
    ASSERT_FALSE( retrievedMarketOpt.value().expired() ) << "Retrieved market weak_ptr should be valid";

    std::shared_ptr<Market> marketSp = marketWp.lock();
    std::shared_ptr<Market> retrievedMarketSp = retrievedMarketOpt.value().lock();
    ASSERT_NE( marketSp, nullptr );
    ASSERT_NE( retrievedMarketSp, nullptr );
    EXPECT_EQ( marketSp, retrievedMarketSp ) << "Retrieved market should be the same as the created one";
}

TEST_F( MarketManagerTests, GetNonExistentMarket )
{
    auto retrievedMarketOpt = m_manager.get( "NonExistentMarket" );
    EXPECT_FALSE( retrievedMarketOpt.has_value() ) << "Should not retrieve a non-existent market";
}

TEST_F( MarketManagerTests, CreateAndEraseMarket )
{
    const std::string handle = "TestMarketToErase";
    const Context ctx = Context::LIVE;

    EXPECT_CALL( *m_mockSource, fetchLatest( testing::_) )
        .WillOnce( testing::Return( std::vector<TMQ::MktDataSource::FetchData>{} ) );

    std::weak_ptr<Market> marketWp = m_manager.create( handle, ctx, m_mockSource );
    ASSERT_FALSE( marketWp.expired() ) << "Market should be created before erasing";

    bool erased = m_manager.erase( handle );
    EXPECT_TRUE( erased ) << "Erase should return true for an existing market";

    auto retrievedMarketOpt = m_manager.get( handle );
    EXPECT_FALSE( retrievedMarketOpt.has_value() ) << "Market should not be retrievable after erase";

    EXPECT_TRUE( marketWp.expired() ) << "Original weak_ptr should be expired after erase if no other shared_ptrs exist";
}

TEST_F( MarketManagerTests, EraseNonExistentMarket )
{
    bool erased = m_manager.erase( "NonExistentMarketToErase" );
    EXPECT_FALSE( erased ) << "Erase should return false for a non-existent market";
}

TEST_F( MarketManagerTests, CreateMultipleMarkets )
{
    const std::string handle1 = "MultiMarket1";
    const std::string handle2 = "MultiMarket2";
    const Context ctx = Context::LIVE;

    EXPECT_CALL( *m_mockSource, fetchLatest( testing::_) )
        .Times(2)
        .WillRepeatedly( testing::Return( std::vector<TMQ::MktDataSource::FetchData>{} ) );

    std::weak_ptr<Market> market1Wp = m_manager.create( handle1, ctx, m_mockSource );
    ASSERT_FALSE( market1Wp.expired() );

    std::weak_ptr<Market> market2Wp = m_manager.create( handle2, ctx, m_mockSource );
    ASSERT_FALSE( market2Wp.expired() );

    auto retrievedMarket1Opt = m_manager.get( handle1 );
    ASSERT_TRUE( retrievedMarket1Opt.has_value() );
    ASSERT_FALSE( retrievedMarket1Opt.value().expired() );
    EXPECT_EQ( market1Wp.lock(), retrievedMarket1Opt.value().lock() );

    auto retrievedMarket2Opt = m_manager.get( handle2 );
    ASSERT_TRUE( retrievedMarket2Opt.has_value() );
    ASSERT_FALSE( retrievedMarket2Opt.value().expired() );
    EXPECT_EQ( market2Wp.lock(), retrievedMarket2Opt.value().lock() );

    // Ensure they are different markets
    EXPECT_NE( market1Wp.lock(), market2Wp.lock() );
}

TEST_F( MarketManagerTests, CreateMarketWithSameHandleThrows )
{
    const std::string handle = "SameHandleMarketThrows";
    const Context ctx1 = Context::LIVE;
    const Context ctx2 = { "EOD", Date( Year( 2023 ), Month::Jan, Day( 15 ) ) };


    EXPECT_CALL( *m_mockSource, fetchLatest( testing::StrEq("LIVE") ) )
        .WillOnce( testing::Return( std::vector<TMQ::MktDataSource::FetchData>{} ) );
    std::weak_ptr<Market> market1Wp = m_manager.create( handle, ctx1, m_mockSource );
    ASSERT_FALSE( market1Wp.expired() );
    auto market1Sp = market1Wp.lock();
    ASSERT_NE( market1Sp, nullptr );

    // Attempting to create a market with the same handle should throw TMQException
    EXPECT_THROW( m_manager.create( handle, ctx2, m_mockSource ), TMQ::TMQException );

    // Verify that the original market is still there and unchanged
    auto retrievedMarketOpt = m_manager.get( handle );
    ASSERT_TRUE( retrievedMarketOpt.has_value() );
    ASSERT_FALSE( retrievedMarketOpt.value().expired() );
    
    EXPECT_EQ( market1Sp, retrievedMarketOpt.value().lock() ) << "Original market should still be present and unchanged.";
    EXPECT_FALSE( market1Wp.expired() ) << "Original market weak_ptr should still be valid.";
}
