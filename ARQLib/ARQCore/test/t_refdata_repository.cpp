#include <ARQCore/refdata_repository.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ARQ;

//TEST( RefDataTemp, Temp )
//{
//	try
//	{
//		RD::Repository repo( "ClickHouseDB" );
//
//		auto cache = repo.get<RD::User>();
//
//		int y = 0;
//
//		auto usr = cache->getByIndex( "userID", "ejames" );
//		if( usr )
//			std::cout << usr->email << std::endl;
//	}
//	catch( ARQException& e )
//	{
//		throw std::runtime_error( e.what() );
//	}
//}


// TODO