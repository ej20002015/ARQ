#include "current_market_selector.h"

#include <gtest/gtest.h>

using namespace ARQ;

namespace
{

Time::DateTime at( const int64_t microseconds )
{
	return Time::DateTime( Time::Microseconds( microseconds ) );
}

}

TEST( CurrentMarketSelectorTest, AcceptsFirstObservationForARecord )
{
	CurrentMarketSelector selector;

	EXPECT_TRUE( selector.push( MD::Type::FXR, "GBP", at( 100 ) ) );
}

TEST( CurrentMarketSelectorTest, RejectsAnOlderEffectiveObservation )
{
	CurrentMarketSelector selector;
	ASSERT_TRUE( selector.push( MD::Type::FXR, "GBP", at( 200 ) ) );

	EXPECT_FALSE( selector.push( MD::Type::FXR, "GBP", at( 100 ) ) );
}

TEST( CurrentMarketSelectorTest, AcceptsANewerEffectiveObservation )
{
	CurrentMarketSelector selector;
	ASSERT_TRUE( selector.push( MD::Type::FXR, "GBP", at( 100 ) ) );

	EXPECT_TRUE( selector.push( MD::Type::FXR, "GBP", at( 200 ) ) );
}

TEST( CurrentMarketSelectorTest, AcceptsLaterInvocationAtEqualEffectiveTime )
{
	CurrentMarketSelector selector;
	ASSERT_TRUE( selector.push( MD::Type::FXR, "GBP", at( 100 ) ) );

	EXPECT_TRUE( selector.push( MD::Type::FXR, "GBP", at( 100 ) ) );
}

TEST( CurrentMarketSelectorTest, TracksTypesAndIDsIndependently )
{
	CurrentMarketSelector selector;
	ASSERT_TRUE( selector.push( MD::Type::FXR, "GBP", at( 200 ) ) );

	EXPECT_TRUE( selector.push( MD::Type::FXR, "EUR", at( 100 ) ) );
	EXPECT_TRUE( selector.push( MD::Type::EQP, "GBP", at( 100 ) ) );
}
