#include <ARQMarket/tid.h>
#include <gtest/gtest.h>

using namespace ARQ;
using namespace ARQ::MD;

TEST( TIDTest, DefaultConstructor )
{
    TID tid;
    EXPECT_EQ( tid.type, Type::_NOTSET_ );
    EXPECT_FALSE( tid.id.has_value() );
}

TEST( TIDTest, ParameterizedConstructor )
{
    TID tidSpecific( Type::FXR, "EURUSD" );
    EXPECT_EQ( tidSpecific.type, Type::FXR );
    EXPECT_TRUE( tidSpecific.id.has_value() );
    EXPECT_EQ( tidSpecific.id.value(), "EURUSD" );

    TID tidWildcard( Type::EQP ); // ID defaults to nullopt
    EXPECT_EQ( tidWildcard.type, Type::EQP );
    EXPECT_FALSE( tidWildcard.id.has_value() );
}

TEST( TIDTest, EqualityOperator )
{
    TID tid1( Type::FXR, "EURUSD" );
    TID tid2( Type::FXR, "EURUSD" );
    TID tid3( Type::FXR, "GBPUSD" );
    TID tid4( Type::EQP, "EURUSD" );

    EXPECT_EQ( tid1, tid2 );
    EXPECT_NE( tid1, tid3 ); // Different ID
    EXPECT_NE( tid1, tid4 ); // Different Type
}

TEST( TIDTest, SpaceshipOperatorSorting )
{
    TID wildcard( Type::FXR, std::nullopt );
    TID specific( Type::FXR, "EURUSD" );
    TID differentType( Type::EQP, "AAPL" );

    // 1. nullopt MUST be less than a valid string
    EXPECT_LT( wildcard, specific );

    // 2. Types are sorted first
    if( Type::EQP < Type::FXR )
        EXPECT_LT( differentType, wildcard );
    else
		EXPECT_LT( wildcard, differentType );
}

// ============================================================================
// TIDSET TESTS
// ============================================================================

TEST( TIDSetTest, DefaultState )
{
    TIDSet set;
    EXPECT_TRUE( set.empty() );
    EXPECT_EQ( set.getAll().size(), 0 );
}

TEST( TIDSetTest, InitializerListConstructor )
{
    TIDSet set{
        { Type::FXR, "EURUSD" },
        { Type::EQP, "AAPL" }
    };

    EXPECT_FALSE( set.empty() );
    EXPECT_EQ( set.getAll().size(), 2 );
    EXPECT_TRUE( set.contains( TID( Type::FXR, "EURUSD" ) ) );
}

TEST( TIDSetTest, BasicInsertAndContains )
{
    TIDSet set;
    set.insert( TID( Type::FXR, "EURUSD" ) );
    set.insert( TID( Type::EQP, "AAPL" ) );

    EXPECT_TRUE( set.contains( TID( Type::FXR, "EURUSD" ) ) );
    EXPECT_TRUE( set.contains( TID( Type::EQP, "AAPL" ) ) );
    EXPECT_FALSE( set.contains( TID( Type::FXR, "GBPUSD" ) ) );
}

TEST( TIDSetTest, DuplicateRemoval )
{
    TIDSet set;
    set.insert( TID( Type::FXR, "EURUSD" ) );
    set.insert( TID( Type::FXR, "EURUSD" ) ); // Duplicate
    set.insert( TID( Type::FXR, "EURUSD" ) ); // Duplicate

    // Consolidate should crush this down to 1 item
    EXPECT_EQ( set.getAll().size(), 1 );
    EXPECT_TRUE( set.contains( TID( Type::FXR, "EURUSD" ) ) );
}

TEST( TIDSetTest, WildcardSubsumption )
{
    TIDSet set;
    // Insert specific items
    set.insert( TID( Type::FXR, "EURUSD" ) );
    set.insert( TID( Type::FXR, "GBPUSD" ) );

    // Insert the wildcard ("ALL FXRs")
    set.insert( TID( Type::FXR, std::nullopt ) );

    // Insert another specific AFTER the wildcard
    set.insert( TID( Type::FXR, "JPYUSD" ) );

    // The consolidate algorithm should wipe out all specific FXRs
    const auto& items = set.getAll();
    EXPECT_EQ( items.size(), 1 );
    EXPECT_FALSE( items[0].id.has_value() ); // The only thing left should be the wildcard

    // BUT, contains() should still return true for specific requests!
    EXPECT_TRUE( set.contains( TID( Type::FXR, "EURUSD" ) ) );
    EXPECT_TRUE( set.contains( TID( Type::FXR, "RANDOM_UNSEEN_TICKER" ) ) );
}

TEST( TIDSetTest, CrossTypeSubsumptionIsolation )
{
    TIDSet set;
    set.insert( TID( Type::FXR, std::nullopt ) ); // ALL FX
    set.insert( TID( Type::EQP, "AAPL" ) );      // Specific EQ

    // The FX wildcard should NOT wipe out the EQ specific ticker
    const auto& items = set.getAll();
    EXPECT_EQ( items.size(), 2 );
    EXPECT_TRUE( set.contains( TID( Type::EQP, "AAPL" ) ) );
}

TEST( TIDSetTest, EraseSpecific )
{
    TIDSet set;
    set.insert( TID( Type::FXR, "EURUSD" ) );
    set.insert( TID( Type::FXR, "GBPUSD" ) );

    set.erase( TID( Type::FXR, "EURUSD" ) );

    EXPECT_FALSE( set.contains( TID( Type::FXR, "EURUSD" ) ) );
    EXPECT_TRUE( set.contains( TID( Type::FXR, "GBPUSD" ) ) );
    EXPECT_EQ( set.getAll().size(), 1 );
}

TEST( TIDSetTest, EraseWildcardWipesType )
{
    TIDSet set;
    set.insert( TID( Type::FXR, "EURUSD" ) );
    set.insert( TID( Type::FXR, "GBPUSD" ) );
    set.insert( TID( Type::EQP, "AAPL" ) );

    // Erasing the wildcard should erase all FXRs
    set.erase( TID( Type::FXR ) );

    EXPECT_EQ( set.getAll().size(), 1 );
    EXPECT_TRUE( set.contains( TID( Type::EQP, "AAPL" ) ) ); // EQ stays
    EXPECT_FALSE( set.contains( TID( Type::FXR, "EURUSD" ) ) ); // FX gone
}

TEST( TIDSetTest, MergeSets )
{
    TIDSet setA;
    setA.insert( TID( Type::FXR, "EURUSD" ) );

    TIDSet setB;
    setB.insert( TID( Type::EQP, "AAPL" ) );
    setB.insert( TID( Type::FXR, std::nullopt ) ); // Wildcard in Set B

    setA.merge( setB );

    // SetA had EURUSD, but SetB had the FX wildcard. 
    // The merge should subsume EURUSD.
    const auto& items = setA.getAll();
    EXPECT_EQ( items.size(), 2 ); // 1 wildcard FX, 1 specific EQ

    EXPECT_TRUE( setA.contains( TID( Type::FXR, "EURUSD" ) ) ); // Subsumed match
    EXPECT_TRUE( setA.contains( TID( Type::EQP, "AAPL" ) ) );  // Exact match
}

TEST( TIDSetTest, Clear )
{
    TIDSet set;
    set.insert( TID( Type::FXR, "EURUSD" ) );
    EXPECT_FALSE( set.empty() );

    set.clear();
    EXPECT_TRUE( set.empty() );
    EXPECT_EQ( set.getAll().size(), 0 );
}

TEST( TIDSetTest, GetIDsForType_EmptyReturnsNone )
{
    TIDSet set;

    auto result = set.getIDsForType( Type::FXR );

    // Should return the 'None' tag struct
    EXPECT_TRUE( std::holds_alternative<TIDSet::None>( result ) );
}

TEST( TIDSetTest, GetIDsForType_WildcardReturnsAll )
{
    TIDSet set;
    set.insert( TID( Type::FXR, std::nullopt ) );

    auto result = set.getIDsForType( Type::FXR );

    // Should return the 'All' tag struct
    EXPECT_TRUE( std::holds_alternative<TIDSet::All>( result ) );
}

TEST( TIDSetTest, GetIDsForType_SpecificReturnsIDList )
{
    TIDSet set;
    set.insert( TID( Type::EQP, "MSFT" ) );
    set.insert( TID( Type::EQP, "AAPL" ) );

    auto result = set.getIDsForType( Type::EQP );

    // 1. Assert we got the specific list back (aborts test if false)
    ASSERT_TRUE( std::holds_alternative<TIDSet::IDList>( result ) );

    // 2. Extract the list and verify contents
    const auto& ids = std::get<TIDSet::IDList>( result );
    EXPECT_EQ( ids.size(), 2 );

    // 3. Because consolidate() sorts lexicographically, AAPL should be first
    EXPECT_EQ( ids[0], "AAPL" );
    EXPECT_EQ( ids[1], "MSFT" );
}

TEST( TIDSetTest, GetIDsForType_SubsumptionReturnsAll )
{
    TIDSet set;
    // Insert specifics, then a wildcard that should subsume them
    set.insert( TID( Type::FXR, "EURUSD" ) );
    set.insert( TID( Type::FXR, "GBPUSD" ) );
    set.insert( TID( Type::FXR, std::nullopt ) ); // The Wildcard

    auto result = set.getIDsForType( Type::FXR );

    // Even though specifics were inserted, the wildcard overrides the state
    EXPECT_TRUE( std::holds_alternative<TIDSet::All>( result ) );
}

TEST( TIDSetTest, GetIDsForType_TypeIsolation )
{
    TIDSet set;
    // FX gets wildcard, EQ gets specific
    set.insert( TID( Type::FXR, std::nullopt ) );
    set.insert( TID( Type::EQP, "AAPL" ) );

    // Check FX
    auto fxResult = set.getIDsForType( Type::FXR );
    EXPECT_TRUE( std::holds_alternative<TIDSet::All>( fxResult ) );

    // Check EQ
    auto eqResult = set.getIDsForType( Type::EQP );
    ASSERT_TRUE( std::holds_alternative<TIDSet::IDList>( eqResult ) );
    EXPECT_EQ( std::get<TIDSet::IDList>( eqResult ).size(), 1 );

    // Check an un-added type (assuming SwapRate or similar exists in your enum)
    // Adjust Type::_NOTSET_ to whatever unpopulated enum value you have
    auto emptyResult = set.getIDsForType( Type::_NOTSET_ );
    EXPECT_TRUE( std::holds_alternative<TIDSet::None>( emptyResult ) );
}
