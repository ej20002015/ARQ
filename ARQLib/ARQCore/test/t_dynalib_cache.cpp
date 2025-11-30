#include <ARQCore/dynalib_cache.h>
#include <gtest/gtest.h>

#include <ARQCore/messaging_service.h>

using namespace ARQ;
using namespace std::string_view_literals; //temp

#include <iostream> //temp
#include <ARQUtils/enum.h>

TEST( DynaLibCacheTests, LoadARQClickHouse )
{
    EXPECT_NO_THROW( const auto& t = DynaLibCache::inst().get( "ARQClickHouse" ) );
}

class TestSubHandler : public ISubscriptionHandler
{
public:
	void             onMsg( Message&& msg )                    override
	{
		std::cout << std::format( "TestSubHandler: Received response message on topic {}", msg.topic ) << std::endl;
	}

	void             onEvent( const SubscriptionEvent& event ) override
	{
		std::cout << std::format( "TestSubHandler Received [{}] subscription event for topic [{}]", Enum::enum_name( event.event ), event.topic ) << std::endl;
	}

	std::string_view getDesc() const                           override { return "TestSubHandler"sv; }
};

TEST( Temp, temp1 )
{
    const auto msgServ = MessagingServiceFactory::create( "NATS" );
	const auto subHandler = std::make_shared<TestSubHandler>();
	const auto subscription = msgServ->subscribe( "EVAN.TEST", subHandler );
	int y = 0;
}
