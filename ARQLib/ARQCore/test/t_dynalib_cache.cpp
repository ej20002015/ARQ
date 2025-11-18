#include <ARQCore/dynalib_cache.h>
#include <gtest/gtest.h>

#include <ARQCore/serialiser.h>
#include <ARQCore/refdata_command_manager.h>

using namespace ARQ;

TEST( DynaLibCacheTests, LoadARQClickHouse )
{
    EXPECT_NO_THROW( const auto& t = DynaLibCache::inst().get( "ARQClickHouse" ) );
}

TEST( Temp, temp1 )
{
    std::shared_ptr<Serialiser> serialiser = SerialiserFactory::create( SerialiserFactory::SerialiserImpl::Protobuf );
    RefDataCommandResponse resp;
    resp.status = RefDataCommandResponse::ERROR;
    resp.message = "Hi";
    resp.corrID = ID::UUID::create();
    Buffer buf = serialiser->serialise( resp );
    const auto des = serialiser->deserialise<RefDataCommandResponse>( BufferView( buf ) );
    ASSERT_EQ( resp.status, des.status );
    ASSERT_EQ( resp.message, des.message );
    ASSERT_EQ( resp.corrID, des.corrID );
}
