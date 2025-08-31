#include <ARQMarket/market.h>
#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include <ARQUtils/error.h>
#include <ARQUtils/os.h>
#include <ARQSerialisation/ser_mktdata_entities.h>

using namespace ARQ;
using namespace ARQ::Time;

using ::testing::Return;
using ::testing::HasSubstr;

class MarketTests : public ::testing::Test
{
protected:
    Mkt::Market m_market;
};

TEST( MarketNameTests, LiveMkt )
{
    EXPECT_EQ( Mkt::Name::LIVE.tag, "LIVE" );
    EXPECT_FALSE( Mkt::Name::LIVE.date.isValid() );
    EXPECT_EQ( Mkt::Name::LIVE.str(), "LIVE" );
}

TEST( MarketNameTests, DatedMkt )
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
    fx1.asofTs = DateTime( Date( Year( 2025 ), Month::May, Day( 6 ) ), Hour( 10 ), Minute( 0 ), Second( 0 ) );
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
    eq1.asofTs = DateTime( Date( Year( 2025 ), Month::May, Day( 6 ) ), Hour( 10 ), Minute( 0 ), Second( 0 ) );
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
    fx1.asofTs = DateTime( Date( Year( 2025 ), Month::May, Day( 6 ) ) );
    MDEntities::EQPrice eq1;
    eq1.ID = "COMMON_ID";
    eq1.last = 45.67;
    eq1.asofTs = DateTime( Date( Year( 2025 ), Month::May, Day( 6 ) ) );

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
    fx1.asofTs = DateTime( Date( Year( 2025 ), Month::Jan, Day( 1 ) ) );
    MDEntities::EQPrice eq1;
    eq1.ID = "AAPL US";
    eq1.last = 150.0;
    eq1.asofTs = DateTime( Date( Year( 2025 ), Month::Jan, Day( 1 ) ) );
    MDEntities::EQPrice eq2;
    eq2.ID = "MSFT US";
    eq2.last = 300.0;
    eq2.asofTs = DateTime( Date( Year( 2025 ), Month::Jan, Day( 1 ) ) );

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
    fx1.asofTs = DateTime( Date( Year( 2025 ), Month::Jan, Day( 1 ) ) );
    m_market.setFXRate( fx1 );

    // Snapshot taken here
    auto snapshot1 = m_market.snap();

    // Modify the live m_market AFTER snapshotting
    MDEntities::FXRate fx2;
    fx2.ID = "EURUSD";
    fx2.mid = 1.2; // Updated rate
    fx2.asofTs = fx1.asofTs + Hours( 1 );
    m_market.setFXRate( fx2 );

    MDEntities::EQPrice eq1;
    eq1.ID = "AAPL US";
    eq1.last = 150.0;
    eq1.asofTs = DateTime( Date( Year( 2025 ), Month::Jan, Day( 1 ) ) );
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

class ConsolidatingTIDSetTests : public ::testing::Test
{
protected:
    Mkt::ConsolidatingTIDSet m_set;
};

TEST_F( ConsolidatingTIDSetTests, DefaultConstructorIsEmpty )
{
    EXPECT_TRUE( m_set.getAll().empty() );
}

TEST_F( ConsolidatingTIDSetTests, InitializerListConstructor )
{
    Mkt::ConsolidatingTIDSet set{
        {MDEntities::Type::FXR, "EUR/USD"},
        {MDEntities::Type::EQP, "AAPL"},
        {MDEntities::Type::FXR} // Type without ID
    };

    const auto& items = set.getAll();
    EXPECT_EQ( items.size(), 2 ); // Should have consolidated FXR entries

    // Should contain type-level FXR (which subsumes the specific EUR/USD)
    EXPECT_TRUE( set.contains( { MDEntities::Type::FXR } ) );
    EXPECT_TRUE( set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_TRUE( set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) ); // Any FXR ID should match

    // Should contain specific EQP
    EXPECT_TRUE( set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
    EXPECT_FALSE( set.contains( { MDEntities::Type::EQP, "MSFT" } ) ); // Different EQP ID should not match
    EXPECT_FALSE( set.contains( { MDEntities::Type::EQP } ) ); // Type-level EQP should not match
}

TEST_F( ConsolidatingTIDSetTests, AddSpecificItems )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    EXPECT_EQ( m_set.getAll().size(), 3 );

    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );

    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "USD/JPY" } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::EQP, "MSFT" } ) );
}

TEST_F( ConsolidatingTIDSetTests, AddTypeWithoutID_ConsolidatesExistingItems )
{
    // Add specific items first
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    EXPECT_EQ( m_set.getAll().size(), 3 );

    // Add type-level FXR - should remove all specific FXR items
    m_set.add( { MDEntities::Type::FXR } );

    const auto& items = m_set.getAll();
    EXPECT_EQ( items.size(), 2 ); // Should have FXR (type-level) and EQP (specific)

    // All FXR IDs should now match the type-level entry
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "USD/JPY" } ) ); // Any FXR ID
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR } ) ); // Type-level

    // EQP should remain specific
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::EQP, "MSFT" } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::EQP } ) ); // Not type-level
}

TEST_F( ConsolidatingTIDSetTests, AddDuplicateItemsIgnored )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } ); // Duplicate
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } ); // Another duplicate

    EXPECT_EQ( m_set.getAll().size(), 1 );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
}

TEST_F( ConsolidatingTIDSetTests, DeleteSpecificItem )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    m_set.del( { MDEntities::Type::FXR, "EUR/USD" } );

    EXPECT_EQ( m_set.getAll().size(), 2 );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
}

TEST_F( ConsolidatingTIDSetTests, DeleteTypeWithoutID_RemovesAllOfThatType )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    // Delete all FXR items
    m_set.del( { MDEntities::Type::FXR } );

    EXPECT_EQ( m_set.getAll().size(), 1 );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
}

TEST_F( ConsolidatingTIDSetTests, DeleteTypeWithoutID_RemovesTypeLevelEntry )
{
    m_set.add( { MDEntities::Type::FXR } ); // Type-level entry
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    m_set.del( { MDEntities::Type::FXR } );

    EXPECT_EQ( m_set.getAll().size(), 1 );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
}

TEST_F( ConsolidatingTIDSetTests, DeleteNonExistentItem )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );

    // Should not crash or affect existing items
    m_set.del( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.del( { MDEntities::Type::EQP, "AAPL" } );

    EXPECT_EQ( m_set.getAll().size(), 1 );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
}

TEST_F( ConsolidatingTIDSetTests, AppendOtherSet )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    Mkt::ConsolidatingTIDSet otherSet{
        {MDEntities::Type::FXR, "GBP/USD"},
        {MDEntities::Type::EQP, "MSFT"}
    };

    m_set.append( otherSet );

    EXPECT_EQ( m_set.getAll().size(), 4 );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "MSFT" } ) );
}

TEST_F( ConsolidatingTIDSetTests, AppendWithConsolidation )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    Mkt::ConsolidatingTIDSet otherSet {
        { MDEntities::Type::FXR } // Type-level - should consolidate existing FXR items
    };

    m_set.append( otherSet );

    const auto& items = m_set.getAll();
    EXPECT_EQ( items.size(), 2 ); // FXR (type-level) and EQP (specific)

    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "USD/JPY" } ) ); // Any FXR ID
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
}

TEST_F( ConsolidatingTIDSetTests, RemoveOtherSet )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );
    m_set.add( { MDEntities::Type::EQP, "MSFT" } );

    Mkt::ConsolidatingTIDSet otherSet {
        { MDEntities::Type::FXR, "EUR/USD" },
        { MDEntities::Type::EQP, "MSFT" }
    };

    m_set.remove( otherSet );

    EXPECT_EQ( m_set.getAll().size(), 2 );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::EQP, "MSFT" } ) );
}

TEST_F( ConsolidatingTIDSetTests, RemoveWithTypeLevelEntry )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    Mkt::ConsolidatingTIDSet otherSet {
        { MDEntities::Type::FXR } // Type-level - should remove all FXR items
    };

    m_set.remove( otherSet );

    EXPECT_EQ( m_set.getAll().size(), 1 );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
}

TEST_F( ConsolidatingTIDSetTests, ItemIsMatchBehavior )
{
    Mkt::ConsolidatingTIDSet::Item typeLevelFXR{ MDEntities::Type::FXR };
    Mkt::ConsolidatingTIDSet::Item specificFXR{ MDEntities::Type::FXR, "EUR/USD" };
    Mkt::ConsolidatingTIDSet::Item otherSpecificFXR{ MDEntities::Type::FXR, "GBP/USD" };
    Mkt::ConsolidatingTIDSet::Item specificEQP{ MDEntities::Type::EQP, "AAPL" };

    // Type-level matches any specific of same type
    EXPECT_TRUE( typeLevelFXR.isMatch( specificFXR ) );
    EXPECT_FALSE( specificFXR.isMatch( typeLevelFXR ) ); // specific doesn't match type-level
    EXPECT_TRUE( typeLevelFXR.isMatch( otherSpecificFXR ) );

    // Type-level doesn't match different type
    EXPECT_FALSE( typeLevelFXR.isMatch( specificEQP ) );

    // Specific items only match exact same ID
    EXPECT_TRUE( specificFXR.isMatch( specificFXR ) );
    EXPECT_FALSE( specificFXR.isMatch( otherSpecificFXR ) );
    EXPECT_FALSE( specificFXR.isMatch( specificEQP ) );
}

TEST_F( ConsolidatingTIDSetTests, ContainsWithTypeLevelEntry )
{
    m_set.add( { MDEntities::Type::FXR } ); // Type-level entry

    // Should match any FXR ID
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "ANY_ID" } ) );

    // Should not match different types
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::EQP } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::EQP, "AAPL" } ) );
}

TEST_F( ConsolidatingTIDSetTests, ContainsWithSpecificEntry )
{
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );

    // Should match exact item and type-level query
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "EUR/USD" } ) );

    // Should not match different IDs or type-level
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR, "GBP/USD" } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::FXR } ) );
    EXPECT_FALSE( m_set.contains( { MDEntities::Type::EQP, "EUR/USD" } ) );
}

TEST_F( ConsolidatingTIDSetTests, ItemEquality )
{
    Mkt::ConsolidatingTIDSet::Item item1{ MDEntities::Type::FXR, "EUR/USD" };
    Mkt::ConsolidatingTIDSet::Item item2{ MDEntities::Type::FXR, "EUR/USD" };
    Mkt::ConsolidatingTIDSet::Item item3{ MDEntities::Type::FXR, "GBP/USD" };
    Mkt::ConsolidatingTIDSet::Item item4{ MDEntities::Type::EQP, "EUR/USD" };
    Mkt::ConsolidatingTIDSet::Item item5{ MDEntities::Type::FXR };

    EXPECT_TRUE( item1 == item2 );
    EXPECT_FALSE( item1 == item3 );
    EXPECT_FALSE( item1 == item4 );
    EXPECT_FALSE( item1 == item5 );
}

TEST_F( ConsolidatingTIDSetTests, ComplexScenario )
{
    // Start with some specific items
    m_set.add( { MDEntities::Type::FXR, "EUR/USD" } );
    m_set.add( { MDEntities::Type::FXR, "GBP/USD" } );
    m_set.add( { MDEntities::Type::EQP, "AAPL" } );

    EXPECT_EQ( m_set.getAll().size(), 3 );

    // Add type-level FXR (should consolidate)
    m_set.add( { MDEntities::Type::FXR } );
    EXPECT_EQ( m_set.getAll().size(), 2 );

    // Add more specific EQP items
    m_set.add( { MDEntities::Type::EQP, "MSFT" } );
    m_set.add( { MDEntities::Type::EQP, "GOOGL" } );
    EXPECT_EQ( m_set.getAll().size(), 4 ); // FXR (type-level) + 3 EQP (specific)

    // Add type-level EQP (should consolidate all EQP)
    m_set.add( { MDEntities::Type::EQP } );
    EXPECT_EQ( m_set.getAll().size(), 2 ); // Both type-level

    // Verify final state
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::FXR, "USD/JPY" } ) ); // Any FXR
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP } ) );
    EXPECT_TRUE( m_set.contains( { MDEntities::Type::EQP, "TSLA" } ) ); // Any EQP
}