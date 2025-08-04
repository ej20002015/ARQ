#include <TMQCore/mktdata_source.h>
#include <gtest/gtest.h>

#include <TMQUtils/time.h>

#include <TMQMarket/managed_market.h>
#include <TMQCore/refdata_manager.h>

using namespace TMQ;

TEST( MktDataSourceTesting, testSelect )
{
	auto mktDataSource = MktDataSourceFactory::create( "ClickHouseDB" );

	const auto rates = mktDataSource->fetchFXRates( "LIVE" );
	for( const MDEntities::FXRate rate : rates )
	{
		std::cout << std::format( "{} ({}) asof {}: bid: {}, ask {}, mid {}\n", rate.ID, rate.source, Time::tpToISO8601Str( rate.asofTs ), rate.bid, rate.ask, rate.mid );
	}

	std::cout.flush();


	//MDEntities::FXRate newRate = rates[0];

	//newRate.source = "Evan";
	//newRate._lastUpdatedBy = "ejames";
	//newRate.asofTs = newRate.asofTs + std::chrono::seconds( 20 );
	//newRate.bid = 1.2;
	//newRate.ask = 1.3;
	//newRate.mid = 1.25;

	//mktDataSource->insertFXRates( { newRate }, "LIVE" );

	RefDataManager rdm;
	auto all = rdm.Users();


	int y = 0;
}