#include <ARQCore/refdata_repository.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ARQ;

// TEST( RefDataTemp, Temp )
// {
// 	try
// 	{
// 		ARQ::RD::Repository repo( "ClickHouseDB" );

// 		auto cache = repo.get<RD::User>();

// 		int y = 0;

// 		auto usr = cache->getByIndex( "userID", "ejames" );
// 		if( usr )
// 			std::cout << usr->email << std::endl;
// 	}
// 	catch( ARQException& e )
// 	{
// 		throw std::runtime_error( e.what() );
// 	}
// }

// TEST( HitCHDB, Temp )
// {
// 	try
// 	{
// 		std::shared_ptr<RD::Source> rds = RD::SourceFactory::inst().create( "ClickHouseDB" );
// 		RD::User usr;
// 		usr.email = "evan.james@thebest.com";
// 		usr.fullName = "bob-nulled-again";
// 		usr.tradingDesk = std::nullopt;
// 		usr.userID = "ebob";
// 		usr.uuid = ID::UUID::create();
// 		RD::Record<RD::User> usrRec;
// 		usrRec.data = usr;
// 		usrRec.header.isActive = true;
// 		usrRec.header.lastUpdatedBy = "ejames";
// 		usrRec.header.lastUpdatedTs = Time::DateTime::nowUTC();
// 		usrRec.header.uuid = usr.uuid;
// 		usrRec.header.version = 1;
// 		std::vector<RD::Record<RD::User>> items;
// 		items.push_back( usrRec );
// 		rds->insert<RD::User>( items );
// 		//auto list = rds->fetch<RD::User>();
// 	}
// 	catch( ARQException& e )
// 	{
// 		throw std::runtime_error( e.what() );
// 	}
// }


// TODO