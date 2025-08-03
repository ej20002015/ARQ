#include <TMQMarket/market.h>
#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include <TMQUtils/error.h>
#include <TMQUtils/os.h>
#include <TMQSerialisation/ser_mktdata_entities.h>

using namespace TMQ;
using namespace std::chrono_literals;
using namespace std::chrono;

using ::testing::Return;
using ::testing::HasSubstr;

class MarketTests : public ::testing::Test
{
protected:
    Mkt::Market m_market;

    std::chrono::system_clock::time_point makeTp( year year, month month, day day, hours hour = 0h, minutes min = 0min, seconds sec = 0s )
    {
        const year_month_day ymd( year, month, day );
        const std::chrono::sys_days datePoint = std::chrono::sys_days( ymd );
        return datePoint + hour + min + sec;
    }
};

TEST( MarketNameTests, LiveMkt )
{
    EXPECT_EQ( Mkt::Name::LIVE.tag, "LIVE" );
    EXPECT_FALSE( Mkt::Name::LIVE.date.isValid() );
    EXPECT_EQ( Mkt::Name::LIVE.str(), "LIVE" );
}

TEST( ContextTest, DatedMkt )
{
    Mkt::Name eodContext = { "EOD", Time::Date( Time::Year( 2025 ), Time::Month::Apr, Time::Day( 28 ) ) };
    EXPECT_EQ( eodContext.tag, "EOD" );
    ASSERT_TRUE( eodContext.date.isValid() );
    EXPECT_EQ( eodContext.str(), "EOD|20250428" );
}

TEST_F( MarketTests, DefaultConstructedIsEmpty )
{
    EXPECT_FALSE( m_market.getFXRate( "ID1" ).has_value() );
    EXPECT_FALSE( m_market.getEQPrice( "ID1" ).has_value() );
}

TEST_F( MarketTests, SetAndGetFXRate )
{
    MDEntities::FXRate fx1;
    fx1.ID = "EURUSD";
    fx1.source = "SRC_A";
    fx1.asofTs = makeTp( 2025y, May, 6d, 10h, 0min, 0s );
    fx1.mid = 1.1;
    fx1.bid = 1.099;
    fx1.ask = 1.101;

    m_market.setFXRate( fx1 );
    EXPECT_TRUE( m_market.getFXRate( fx1.ID ).has_value() );
    EXPECT_DOUBLE_EQ( m_market.getFXRate( fx1.ID )->mid, fx1.mid );
    EXPECT_FALSE( m_market.getEQPrice( fx1.ID ).has_value() );
}

TEST_F( MarketTests, SetAndGetEQ )
{
    MDEntities::EQPrice eq1;
    eq1.ID = "AAPL US";
    eq1.source = "SRC_B";
    eq1.asofTs = makeTp( 2025y, May, 6d, 10h, 0min, 0s );
    eq1.last = 150.5;

    m_market.setEQPrice( eq1 );
    EXPECT_TRUE( m_market.getEQPrice( eq1.ID ).has_value() );
    EXPECT_DOUBLE_EQ( m_market.getEQPrice( eq1.ID )->last, eq1.last );
    EXPECT_FALSE( m_market.getFXRate( eq1.ID ).has_value() );
}

TEST_F( MarketTests, SetDifferentTypesWithSameID )
{
    MDEntities::FXRate fx1;
    fx1.ID = "COMMON_ID";
    fx1.mid = 1.23;
    fx1.asofTs = makeTp( 2025y, January, 1d );
    MDEntities::EQPrice eq1;
    eq1.ID = "COMMON_ID";
    eq1.last = 45.67;
    eq1.asofTs = makeTp( 2025y, January, 1d );

    m_market.setFXRate( fx1 );
    m_market.setEQPrice( eq1 );

    auto retrievedFx = m_market.getFXRate( "COMMON_ID" );
    ASSERT_TRUE( retrievedFx.has_value() );
    EXPECT_DOUBLE_EQ( retrievedFx.value().mid, 1.23 );

    auto retrievedEq = m_market.getEQPrice( "COMMON_ID" );
    ASSERT_TRUE( retrievedEq.has_value() );
    EXPECT_DOUBLE_EQ( retrievedEq.value().last, 45.67 );
}

TEST_F( MarketTests, SnapEmptyMarket )
{
    auto snapshot = m_market.snap();
    EXPECT_FALSE( snapshot.getFXRate( "ID1" ).has_value() );
    EXPECT_FALSE( snapshot.getEQPrice( "ID1" ).has_value() );
    EXPECT_TRUE( snapshot.getAllFXRates().empty() );
    EXPECT_TRUE( snapshot.getAllEQPrices().empty() );
}

TEST_F( MarketTests, SnapPopulatedMarket )
{
    MDEntities::FXRate fx1;
    fx1.ID = "EURUSD";
    fx1.mid = 1.1;
    fx1.asofTs = makeTp( 2025y, January, 1d );
    MDEntities::EQPrice eq1;
    eq1.ID = "AAPL US";
    eq1.last = 150.0;
    eq1.asofTs = makeTp( 2025y, January, 1d );
    MDEntities::EQPrice eq2;
    eq2.ID = "MSFT US";
    eq2.last = 300.0;
    eq2.asofTs = makeTp( 2025y, January, 1d );

    m_market.setFXRate( fx1 );
    m_market.setEQPrice( eq1 );
    m_market.setEQPrice( eq2 );

    auto snapshot = m_market.snap();

    auto snapFx = snapshot.getFXRate( "EURUSD" );
    ASSERT_TRUE( snapFx.has_value() );
    EXPECT_DOUBLE_EQ( snapFx.value().mid, 1.1 );

    auto snapEq = snapshot.getEQPrice( "AAPL US" );
    ASSERT_TRUE( snapEq.has_value() );
    EXPECT_DOUBLE_EQ( snapEq.value().last, 150.0 );

    auto snapEq2 = snapshot.getEQPrice( "MSFT US" );
    ASSERT_TRUE( snapEq2.has_value() );
    EXPECT_DOUBLE_EQ( snapEq2.value().last, 300.0 );

    EXPECT_FALSE( snapshot.getFXRate( "UNKNOWN" ).has_value() );

    const auto& fxMapSnap = snapshot.getAllFXRates();
    EXPECT_EQ( fxMapSnap.size(), 1 );

    const auto& eqMapSnap = snapshot.getAllEQPrices();
    EXPECT_EQ( eqMapSnap.size(), 2 );
}

TEST_F( MarketTests, SnapIsIndependent )
{
    MDEntities::FXRate fx1;
    fx1.ID = "EURUSD";
    fx1.mid = 1.1;
    fx1.asofTs = makeTp( 2025y, January, 1d );
    m_market.setFXRate( fx1 );

    // Snapshot taken here
    auto snapshot1 = m_market.snap();

    // Modify the live m_market AFTER snapshotting
    MDEntities::FXRate fx2;
    fx2.ID = "EURUSD";
    fx2.mid = 1.2; // Updated rate
    fx2.asofTs = makeTp( 2025y, January, 1d, 1h );
    m_market.setFXRate( fx2 );

    MDEntities::EQPrice eq1;
    eq1.ID = "AAPL US";
    eq1.last = 150.0;
    eq1.asofTs = makeTp( 2025y, January, 1d );
    m_market.setEQPrice( eq1 );

    // Check snapshot1 - should still have the old data
    auto snapFx1 = snapshot1.getFXRate( "EURUSD" );
    ASSERT_TRUE( snapFx1.has_value() );
    EXPECT_DOUBLE_EQ( snapFx1.value().mid, 1.1 ); // Rate from time of snapshot
    EXPECT_FALSE( snapshot1.getEQPrice( "AAPL US" ).has_value() ); // EQ was added after snap1

    // Create a new snapshot - should reflect the latest changes
    auto snapshot2 = m_market.snap();
    auto snapFx2 = snapshot2.getFXRate( "EURUSD" );
    ASSERT_TRUE( snapFx2.has_value() );
    EXPECT_DOUBLE_EQ( snapFx2.value().mid, 1.2 ); // Updated rate

    auto snapEq2 = snapshot2.getEQPrice( "AAPL US" );
    ASSERT_TRUE( snapEq2.has_value() ); // EQ is now present
    EXPECT_DOUBLE_EQ( snapEq2.value().last, 150.0 );
}

//class MockMktDataSource : public MktDataSource
//{
//public:
//    MOCK_METHOD( std::vector<FetchData>, fetchLatest, ( const std::string_view context ), ( override ) );
//    MOCK_METHOD( void, insert, ( const std::string_view context, const std::vector<InsertData>& insData ), ( override ) );
//};
//
//class MarketLoadSaveTests : public ::testing::Test
//{
//public:
//    MarketLoadSaveTests()
//        : m_mockSource( std::make_shared<MockMktDataSource>() )
//    {
//    }
//
//public:
//    std::shared_ptr<MockMktDataSource> m_mockSource;
//    Mkt::Context m_testCtx = { "TEST_EOD", Time::Date( Time::Year( 2025 ), Time::Month::Apr, Time::Day( 28 ) ) };
//};
//
//TEST_F( MarketLoadSaveTests, LoadEmptyMarket )
//{
//    EXPECT_CALL( *m_mockSource, fetchLatest( m_testCtx.str() ) )
//        .WillOnce( Return( std::vector<MktDataSource::FetchData>{} ) );
//
//    std::shared_ptr<Mkt::Market> market = Mkt::load( m_testCtx, m_mockSource );
//    ASSERT_NE( market, nullptr );
//    auto snap = market->snap();
//    EXPECT_EQ( snap.size(), 0 );
//}
//
//TEST_F( MarketLoadSaveTests, LoadFXAndEQ )
//{
//    MDEntities::FXRate fx1Data;
//    fx1Data.ID = "EURUSD";
//    fx1Data.rate = 1.1;
//    fx1Data.source = "S1";
//    fx1Data.asofTs = system_clock::now();
//    fx1Data._active = true;
//
//    MDEntities::EQPrice eq1Data;
//    eq1Data.ID = "AAPL";
//    eq1Data.price = 150.0;
//    eq1Data.source = "S2";
//    eq1Data.asofTs = system_clock::now();
//    eq1Data._active = true;
//
//    std::vector<MktDataSource::FetchData> fetchData;
//    fetchData.emplace_back( "FX", "EURUSD", fx1Data.asofTs, serialise( fx1Data ), "S1", system_clock::now(), "UserFetch" );
//    fetchData.emplace_back( "EQ", "AAPL", eq1Data.asofTs, serialise( eq1Data ), "S2", system_clock::now(), "UserFetch" );
//
//    EXPECT_CALL( *m_mockSource, fetchLatest( m_testCtx.str() ) )
//        .WillOnce( Return( std::move( fetchData ) ) );
//
//    std::shared_ptr<Mkt::Market> market = Mkt::load( m_testCtx, m_mockSource );
//    ASSERT_NE( market, nullptr );
//
//    auto optFx = market->getFXRate( "EURUSD" );
//    ASSERT_TRUE( optFx.has_value() );
//    EXPECT_DOUBLE_EQ( optFx.value().rate, fx1Data.rate );
//    EXPECT_EQ( optFx.value().source, "S1" );
//    EXPECT_EQ( optFx.value().asofTs, fx1Data.asofTs );
//    EXPECT_TRUE( optFx.value()._active );
//    EXPECT_EQ( optFx.value()._lastUpdatedBy, "UserFetch" );
//
//    auto optEq = market->get<EQ>( "AAPL" );
//    ASSERT_TRUE( optEq.has_value() );
//    EXPECT_DOUBLE_EQ( optEq.value().price, eq1Data.price );
//    EXPECT_EQ( optEq.value().source, "S2" );
//    EXPECT_EQ( optEq.value().asofTs, eq1Data.asofTs );
//    EXPECT_TRUE( optEq.value()._active );
//    EXPECT_EQ( optEq.value()._lastUpdatedBy, "UserFetch" );
//}
//
//TEST_F( MarketLoadSaveTests, LoadUnknownTypeThrows )
//{
//    std::vector<MktDataSource::FetchData> fetchData;
//    fetchData.emplace_back( "UNKNOWN_TYPE", "ID1", system_clock::now(), Buffer(), "S1", system_clock::now(), "UserFetch" );
//    
//    EXPECT_CALL( *m_mockSource, fetchLatest( m_testCtx.str() ) )
//        .WillOnce( Return( std::move( fetchData ) ) );
//
//    EXPECT_THROW( {
//        try
//        {
//            Mkt::load( m_testCtx, m_mockSource );
//        }
//        catch( const TMQException& e )
//        {
//            EXPECT_THAT( e.what(), HasSubstr( "Don't know how to load MDEntities of type" ) );
//            throw;
//        }
//    }, TMQException );
//}
//
//TEST_F( MarketLoadSaveTests, LoadUsesGlobalDataSource )
//{
//    const auto oldGlobal = GlobalMktDataSource::get();
//    GlobalMktDataSource::CreatorFunc func = [this] () { return m_mockSource; };
//    GlobalMktDataSource::setFunc( func );
//    EXPECT_EQ( GlobalMktDataSource::get(), m_mockSource );
//
//    EXPECT_CALL( *m_mockSource, fetchLatest( m_testCtx.str() ) )
//        .WillOnce( Return( std::vector<MktDataSource::FetchData>{} ) );
//
//    std::shared_ptr<Mkt::Market> market = Mkt::load( m_testCtx );
//    ASSERT_NE( market, nullptr );
//    EXPECT_EQ( market->snap().size(), 0 );
//
//    GlobalMktDataSource::setFunc( [oldGlobal] () { return oldGlobal; } );
//}
//
//TEST_F( MarketLoadSaveTests, SaveEmptyMarket )
//{
//    auto market = std::make_shared<Mkt::Market>();
//
//    EXPECT_CALL( *m_mockSource, insert( m_testCtx.str(), testing::IsEmpty() ) ) // Expect empty vector
//        .Times( 1 );
//
//    Mkt::save( market, m_testCtx, m_mockSource );
//}
//
//TEST_F( MarketLoadSaveTests, SaveMarketWithFXAndEQ )
//{
//    auto market = std::make_shared<Mkt::Market>();
//
//    FXRate fx1;
//    fx1.ID = "EURUSD";
//    fx1.mid = 1.2;
//    fx1.source = "S_FX";
//    fx1.asofTs = system_clock::now();
//    fx1._active = true;
//
//    MDEntities::EQPrice eq1;
//    eq1.ID = "MSFT";
//    eq1.last = 250.5;
//    eq1.source = "S_EQ";
//    eq1.asofTs = system_clock::now();
//    eq1._active = true;
//
//    market->set( fx1 );
//    market->set( eq1 );
//
//    // Capture the argument to inspect it
//    std::vector<MktDataSource::InsertData> captured_insData;
//    EXPECT_CALL( *m_mockSource, insert( m_testCtx.str(), testing::_ ) )
//        .WillOnce( [&] ( const std::string_view, const std::vector<MktDataSource::InsertData>& insData )
//    {
//        std::vector<MktDataSource::InsertData> insDataCopy;
//        for( const auto& d : insData )
//            insDataCopy.emplace_back( d.type, d.ID, d.asofTs, Buffer( d.blob.data.get(), d.blob.size), d.source, d.lastUpdatedBy, d.active);
//
//        captured_insData = std::move( insDataCopy );
//    } );
//
//    Mkt::save( market, m_testCtx, m_mockSource );
//
//    ASSERT_EQ( captured_insData.size(), 2 );
//
//    bool fxFound = false;
//    bool eqFound = false;
//
//    for( const auto& item : captured_insData )
//    {
//        if( item.ID == "EURUSD" )
//        {
//            fxFound = true;
//            EXPECT_EQ( item.type, MDEntityTraits<FXRate>::type() );
//            EXPECT_EQ( item.source, "S_FX" );
//            EXPECT_EQ( item.asofTs, fx1.asofTs );
//            EXPECT_TRUE( item.active );
//            EXPECT_EQ( item.lastUpdatedBy, "Evan" );
//            FXRate deserialisedFX = deserialise<FXRate>( item.blob );
//            EXPECT_DOUBLE_EQ( deserialisedFX.rate, fx1.mid );
//        }
//        else if( item.ID == "MSFT" )
//        {
//            eqFound = true;
//            EXPECT_EQ( item.type, MDEntityTraits<EQ>::type() );
//            EXPECT_EQ( item.source, "S_EQ" );
//            EXPECT_EQ( item.asofTs, eq1.asofTs );
//            EXPECT_TRUE( item.active );
//            EXPECT_EQ( item.lastUpdatedBy, "Evan" );
//            EQ deserialisedEQ = deserialise<EQ>( item.blob );
//            EXPECT_DOUBLE_EQ( deserialisedEQ.price, eq1.last );
//        }
//    }
//    EXPECT_TRUE( fxFound );
//    EXPECT_TRUE( eqFound );
//}
//
//TEST_F( MarketLoadSaveTests, SaveSnapshotDirectly )
//{
//    auto tempMarket = std::make_shared<Mkt::Market>();
//    FXRate fx1;
//    fx1.ID = "USDJPY";
//    fx1.mid = 110.5;
//    fx1.source = "SNAP_S";
//    fx1.asofTs = system_clock::now();
//    fx1._active = true;
//    tempMarket->set( fx1 );
//    Mkt::MarketSnapshot snapshot = tempMarket->snap();
//
//    std::vector<MktDataSource::InsertData> captured_insData;
//    EXPECT_CALL( *m_mockSource, insert( m_testCtx.str(), testing::_ ) )
//        .WillOnce( [&] ( const std::string_view, const std::vector<MktDataSource::InsertData>& insData )
//    {
//        std::vector<MktDataSource::InsertData> insDataCopy;
//        for( const auto& d : insData )
//            insDataCopy.emplace_back( d.type, d.ID, d.asofTs, Buffer( d.blob.data.get(), d.blob.size ), d.source, d.lastUpdatedBy, d.active );
//
//        captured_insData = std::move( insDataCopy );
//    } );
//
//    Mkt::save( snapshot, m_testCtx, m_mockSource );
//
//    ASSERT_EQ( captured_insData.size(), 1 );
//    const auto& item = captured_insData[0];
//    EXPECT_EQ( item.ID, "USDJPY" );
//    EXPECT_EQ( item.type, MDEntityTraits<FXRate>::type() );
//    EXPECT_EQ( item.source, "SNAP_S" );
//    EXPECT_EQ( item.asofTs, fx1.asofTs );
//    EXPECT_TRUE( item.active );
//    EXPECT_EQ( item.lastUpdatedBy, "Evan" );
//    FXRate deserialisedFX = deserialise<FXRate>( item.blob );
//    EXPECT_DOUBLE_EQ( deserialisedFX.rate, fx1.mid );
//}
//
//TEST_F( MarketLoadSaveTests, SaveUsesGlobalDataSource )
//{
//    const auto oldGlobal = GlobalMktDataSource::get();
//    GlobalMktDataSource::CreatorFunc func = [this] () { return m_mockSource; };
//    GlobalMktDataSource::setFunc( func );
//    EXPECT_EQ( GlobalMktDataSource::get(), m_mockSource );
//
//    EXPECT_CALL( *m_mockSource, insert( m_testCtx.str(), testing::IsEmpty() ) )
//        .Times( 1 );
//
//    Mkt::save( std::make_shared<Mkt::Market>(), m_testCtx );
//
//    GlobalMktDataSource::setFunc( [oldGlobal] () { return oldGlobal; } );
//}

//TEST( MarketTests, TestLoading )
//{
//	Mkt::Context evanTest = { "EVAN_TEST", year_month_day{ 2025y, May, 5d } };
//
//	auto mkt = Mkt::load( evanTest );
//	int y = 0;
//}
//
//TEST( MarketTests, TestSaving )
//{
//	Mkt::Context evanTest = { "EVAN_TEST", year_month_day{ 2025y, May, 5d } };
//	auto mkt = std::make_shared<Mkt::Market>();
//
//	FXRate fx;
//	fx.ID = "GBPUSD";
//	fx.rate = 1.2;
//	fx.bid = 1.19;
//	fx.ask = 1.21;
//	fx.source = "TP";
//	fx.asofTs = system_clock::now();
//	mkt->set( fx );
//
//	constexpr year_month_day ymd{ 2025y, May, 5d };
//	constexpr std::chrono::sys_days date_point = std::chrono::sys_days( ymd );
//	constexpr system_clock::time_point specific_time = date_point + hours( 13 ) + minutes( 0 ) + seconds( 0 );
//
//	EQ eq;
//	eq.ID = "TSLA US";
//	eq.price = 419.0;
//	eq.open = 415.0;
//	eq.close = 412.0;
//	eq.high = 422.0;
//	eq.low = 414.0;
//	eq.source = "NASDAQ";
//	eq.asofTs = specific_time;
//	mkt->set( eq );
//
//	try
//	{
//		Mkt::save( mkt, evanTest );
//	}
//	catch( const TMQException& e )
//	{
//		int y = 0;
//	}
//}