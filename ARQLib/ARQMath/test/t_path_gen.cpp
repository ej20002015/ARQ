#include <gtest/gtest.h>
#include <ARQMath/path_gen.h>

#include <ARQUtils/error.h>

using namespace ARQ;
using namespace ARQ::Math::Stochastic;

class BrownianBridgeTest : public ::testing::Test
{
protected:
    BrownianBridgePathGenerator::Config createValidConfig()
    {
        return {
            .ticksPerStep = 1000,
            .ticksPerPeriod = 86400000,
            .onPeriodRollover = [] ( int64_t idx )
            {
                return PeriodConfig{.startValue = 1.0, .endValue = 2.0, .volatility = 0.005, .seed = static_cast<uint64_t>( idx ) };
            }
        };
    }
};

TEST_F( BrownianBridgeTest, ThrowsOnInvalidModulo )
{
    auto config = createValidConfig();
    config.ticksPerPeriod = 1000;
    config.ticksPerStep = 300;

    // 1000 % 300 != 0
    EXPECT_THROW( BrownianBridgePathGenerator gen( config ), ARQException );
}

TEST_F( BrownianBridgeTest, ThrowsOnMissingCallback )
{
    auto config = createValidConfig();
    config.onPeriodRollover = nullptr;

    EXPECT_THROW( BrownianBridgePathGenerator gen( config ), ARQException );
}

TEST_F( BrownianBridgeTest, PathAnchorsToStartAndEndValues )
{
    auto config = createValidConfig();
    config.ticksPerStep = 1;
    config.ticksPerPeriod = 100; // exactly 100 steps

    BrownianBridgePathGenerator gen( config );

    // Step 0 should be exactly the start value
    EXPECT_DOUBLE_EQ( gen.getValueAtTime( 0 ), 1.0 );
    // The final step (tick 99) should cleanly hit the end value
    EXPECT_NEAR( gen.getValueAtTime( 99 ), 2.0, 1e-9 );
}

TEST_F( BrownianBridgeTest, RolloverTriggersCallbackAndAppliesNewConfig )
{
    auto config = createValidConfig();
    config.ticksPerStep = 1;
    config.ticksPerPeriod = 10;

    int64_t callbackFiredCount = 0;
    config.onPeriodRollover = [&] ( int64_t newPeriodIndex )
    {
        callbackFiredCount++;
        // Return distinctly different values for the next period
        return PeriodConfig{ 2.0, 3.0, 0.01, static_cast<uint64_t>( newPeriodIndex ) };
    };

    BrownianBridgePathGenerator gen( config );

    EXPECT_EQ( callbackFiredCount, 0 ) << "Callback should not fire on initialization";

    auto _ = gen.getValueAtTime( 5 );
    EXPECT_EQ( callbackFiredCount, 1 ) << "Callback should fire on first period";
    // Tick 6 is still in the same period as tick 5
    _ = gen.getValueAtTime( 6 );
	EXPECT_EQ( callbackFiredCount, 1 ) << "Callback should not fire again within the same period";

    // Tick 10 is the exact start of Period 1
    double startOfNewPeriod = gen.getValueAtTime( 10 );
    EXPECT_EQ( callbackFiredCount, 2 ) << "Rollover callback should fire exactly once";
    EXPECT_DOUBLE_EQ( startOfNewPeriod, 2.0 ) << "Should apply the new start value";

    // Tick 19 is the exact end of Period 1
    double endOfNewPeriod = gen.getValueAtTime( 19 );
    EXPECT_NEAR( endOfNewPeriod, 3.0, 1e-9 ) << "Should anchor to the new end value";
}

TEST_F( BrownianBridgeTest, CatchUpSkipsEmptyPeriodsFast )
{
    auto config = createValidConfig();
    config.ticksPerStep = 1;
    config.ticksPerPeriod = 10;

    int64_t lastRequestedPeriod = -1;
    config.onPeriodRollover = [&] ( int64_t newPeriodIndex )
    {
        lastRequestedPeriod = newPeriodIndex;
        return PeriodConfig{ 1.0, 1.0, 0.01, 42 };
    };

    BrownianBridgePathGenerator gen( config );

    // Ask for a tick way in the future (Period 50)
    auto _ = gen.getValueAtTime( 505 );

    EXPECT_EQ( lastRequestedPeriod, 50 ) << "Generator should fast-forward directly to period 50";
}

TEST_F( BrownianBridgeTest, IdenticalSeedsProduceIdenticalPaths )
{
    auto config1 = createValidConfig();
    auto config2 = createValidConfig();

    // Ensure both are identical
    config1.ticksPerStep = 1; config1.ticksPerPeriod = 100;
    config2.ticksPerStep = 1; config2.ticksPerPeriod = 100;

    BrownianBridgePathGenerator gen1( config1 );
    BrownianBridgePathGenerator gen2( config2 );

    for( int64_t tick = 0; tick < 100; ++tick )
    {
        EXPECT_DOUBLE_EQ( gen1.getValueAtTime( tick ), gen2.getValueAtTime( tick ) )
            << "Paths diverged at tick " << tick;
    }
}