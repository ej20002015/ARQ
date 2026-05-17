#include <ARQMarket/market.h>
#include <gtest/gtest.h>

using namespace ARQ::MD;
using namespace ARQ::Time;

// Helper to create a dummy FX record
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
// MarketName Tests
// ---------------------------------------------------------

TEST( MarketNameTest, DefaultAndParameterizedConstruction )
{
    MarketName defaultName;
    EXPECT_FALSE( defaultName.isSet() );

    MarketName dummyName( "DUMMY", Date( Year( 2026 ), Month( 5 ), Day( 17 ) ) );
    EXPECT_TRUE( dummyName.isSet() );
    EXPECT_EQ( dummyName.tag, "DUMMY" );
    
    const std::string str = dummyName.str();
    EXPECT_EQ( str, "DUMMY|20260517" );
    
    const MarketName nameFromStr = MarketName::fromStr( str );
    EXPECT_EQ( dummyName, nameFromStr );
}

TEST( MarketNameTest, StrOperations )
{
    MarketName dummyName( "DUMMY", Date( Year( 2026 ), Month( 5 ), Day( 17 ) ) );

    const std::string str = dummyName.str();
    EXPECT_EQ( str, "DUMMY|20260517" );

    const MarketName nameFromStr = MarketName::fromStr( str );
    EXPECT_EQ( dummyName, nameFromStr );
}


// ---------------------------------------------------------
// Market(Impl) Tests
// ---------------------------------------------------------

TEST( MarketImplTest, RCU_ReaderIsolation_MaintainsImmutability )
{
    Market market;

    // 1. Grab initial empty snapshot
    auto snap1 = market.snapshot();
    EXPECT_FALSE( snap1->get<FXRate>( "EUR/USD" ) );

    // 2. Update the market
    RecordCollection updates;
    updates.get<Record<FXRate>>().push_back( makeFXRecord( "EUR/USD", 1.08, 100 ) );
    market.update( std::move( updates ) );

    // 3. Grab new snapshot
    auto snap2 = market.snapshot();

    // 4. Verify RCU ISOLATION: snap1 must remain empty!
    EXPECT_FALSE( snap1->get<FXRate>( "EUR/USD" ) );

    // 5. Verify snap2 has the data
    auto eurUsd = snap2->get<FXRate>( "EUR/USD" );
    ASSERT_TRUE( eurUsd );
    EXPECT_DOUBLE_EQ( eurUsd->data.mid, 1.08 );
    EXPECT_EQ( snap2->asofTs().microsecondsSinceEpoch(), 100 ); // Snapshot global timestamp updated
}

TEST( MarketImplTest, StaleUpdates_AreSafelyRejected )
{
    Market market;

    // 1. Initial State (Time = 100)
    RecordCollection initial;
    initial.get<Record<FXRate>>().push_back( makeFXRecord( "GBP/USD", 1.29, 100 ) );
    market.update( std::move( initial ) );

    // 2. Stale Update (Time = 50) - e.g., delayed network packet
    RecordCollection staleUpdate;
    staleUpdate.get<Record<FXRate>>().push_back( makeFXRecord( "GBP/USD", 1.50, 50 ) );
    market.update( std::move( staleUpdate ) );

    // 3. Verify stale data was dropped
    auto snapAfterStale = market.snapshot();
    auto gbpUsd = snapAfterStale->get<FXRate>( "GBP/USD" );
    ASSERT_TRUE( gbpUsd );
    EXPECT_DOUBLE_EQ( gbpUsd->data.mid, 1.29 ); // Kept the old value!
    EXPECT_EQ( gbpUsd->header.asofTs.microsecondsSinceEpoch(), 100 );

    // 4. Fresh Update (Time = 150)
    RecordCollection freshUpdate;
    freshUpdate.get<Record<FXRate>>().push_back( makeFXRecord( "GBP/USD", 1.30, 150 ) );
    market.update( std::move( freshUpdate ) );

    // 5. Verify fresh data was applied
    auto snapAfterFresh = market.snapshot();
    EXPECT_DOUBLE_EQ( snapAfterFresh->get<FXRate>( "GBP/USD" )->data.mid, 1.30 );
}

TEST( MarketImplTest, EqualTimestamp_IsKept )
{
    Market market;

    // Exact same timestamp should be kept to allow newer updates to be captured
    RecordCollection update1;
    update1.get<Record<FXRate>>().push_back( makeFXRecord( "JPY/USD", 150.0, 100 ) );
    market.update( std::move( update1 ) );

    RecordCollection update2;
    update2.get<Record<FXRate>>().push_back( makeFXRecord( "JPY/USD", 155.0, 100 ) ); // Same time, different price
    market.update( std::move( update2 ) );

    auto snap = market.snapshot();
    EXPECT_DOUBLE_EQ( snap->get<FXRate>( "JPY/USD" )->data.mid, 155.0 );
}

TEST( MarketImplTest, Erase_RemovesEntityFromFutureSnapshots )
{
    Market market;

    // 1. Add Record
    RecordCollection initial;
    initial.get<Record<FXRate>>().push_back( makeFXRecord( "EUR/GBP", 0.85, 100 ) );
    market.update( std::move( initial ) );

    auto snap1 = market.snapshot();
    EXPECT_TRUE( snap1->get<FXRate>( "EUR/GBP" ) );

    // 2. Delete Record (isActive = false)
    RecordCollection deletion;
    deletion.get<Record<FXRate>>().push_back( makeFXRecord( "EUR/GBP", 0.0, 150, false ) );
    market.update( std::move( deletion ) );

    auto snap2 = market.snapshot();

    // 3. Verify Isolation & Deletion
    EXPECT_TRUE( snap1->get<FXRate>( "EUR/GBP" ) );
    EXPECT_FALSE( snap2->get<FXRate>( "EUR/GBP" ) );
}