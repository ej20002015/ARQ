#include <gtest/gtest.h>

#include <TMQMarket/market.h>

using namespace TMQ;

TEST( MarketTests, TestAddingObjs )
{
    Mkt::Market mkt;
	
	FXRate fx;
	fx.instrumentID = "GBPUSD";
	fx.rate = 1.2;
	fx.bid  = 1.19;
	fx.ask  = 1.21;
	mkt.set( fx );

	auto fxRet = mkt.get<FXRate>( "GBPUSD" );
	ASSERT_TRUE( fxRet.has_value() );
	ASSERT_EQ( fxRet->get().instrumentID, "GBPUSD" );
	ASSERT_DOUBLE_EQ( fxRet->get().rate, 1.2 );
	ASSERT_DOUBLE_EQ( fxRet->get().bid, 1.19 );
	ASSERT_DOUBLE_EQ( fxRet->get().ask, 1.21 );
}