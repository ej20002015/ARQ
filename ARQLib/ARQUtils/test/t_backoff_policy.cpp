#include <gtest/gtest.h>
#include <ARQUtils/backoff_policy.h>

#include <ARQUtils/error.h>

using namespace ARQ;
using namespace std::chrono_literals;

class BackoffPolicyTest : public ::testing::Test
{
protected:
	bool isDurationEqual( std::optional<BackoffPolicy::Duration> actual, BackoffPolicy::Duration expected )
	{
		return actual.has_value() && actual.value() == expected;
	}
};

TEST_F( BackoffPolicyTest, ParsesValidStringAndComputesGrowth )
{
	// 10ms start, double every time, cap at 50ms
	BackoffPolicy policy( "10ms-2.0-50ms" );

	// Attempt 1: Initial (10ms)
	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 10ms ) );
	
	// Attempt 2: 10 * 2 = 20ms
	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 20ms ) );

	// Attempt 3: 20 * 2 = 40ms
	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 40ms ) );

	// Attempt 4: 40 * 2 = 80ms -> Capped at 50ms
	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 50ms ) );

	// Attempt 5: Still capped at 50ms
	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 50ms ) );
}

TEST_F( BackoffPolicyTest, HandlesConstantStrategy )
{
	// 100ms fixed, never changes
	BackoffPolicy policy( "100ms-CONSTANT-1s" );

	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 100ms ) );
	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 100ms ) );
	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 100ms ) );
}

TEST_F( BackoffPolicyTest, EnforcesMaxAttempts )
{
	// 10ms start, 2x growth, 1s cap, STOP after 3 tries
	BackoffPolicy policy( "10ms-2.0-1s-3" );

	// Attempt 1
	EXPECT_TRUE( policy.nextDelay().has_value() );
	
	// Attempt 2
	EXPECT_TRUE( policy.nextDelay().has_value() );

	// Attempt 3
	EXPECT_TRUE( policy.nextDelay().has_value() );

	// Attempt 4 -> Should return nullopt (Stop)
	EXPECT_FALSE( policy.nextDelay().has_value() );
}

TEST_F( BackoffPolicyTest, ResetsCorrectly )
{
	BackoffPolicy policy( "10ms-2.0-100ms" );

	// Run a few times
	auto _ = policy.nextDelay(); // 10
	_ =      policy.nextDelay(); // 20

	policy.reset();

	// Should be back to initial
	EXPECT_TRUE( isDurationEqual( policy.nextDelay(), 10ms ) );
}

TEST_F( BackoffPolicyTest, ThrowsOnInvalidStrings )
{
	// Missing parts
	EXPECT_THROW( BackoffPolicy( "100ms" ), ARQException );
	EXPECT_THROW( BackoffPolicy( "100ms-2.0" ), ARQException );

    // Bad duration
	EXPECT_THROW( BackoffPolicy( "garbage-input-str" ), ARQException ); 
}