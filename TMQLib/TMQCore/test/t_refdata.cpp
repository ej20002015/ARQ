#include <TMQCore/refdata.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace TMQ
{

struct TestRefDataEntity : public RDEntity
{
	std::string x;
	uint32_t y;
};

template<>
static std::vector<TestRefDataEntity> TypedRefDataSource<TestRefDataEntity>::fetchLatest( RefDataSource& source )
{
    return {
        { "bob", std::chrono::system_clock::now(), "bob", 3 }
    };
}

template<>
static std::vector<TestRefDataEntity> TypedRefDataSource<TestRefDataEntity>::fetchAsOf( RefDataSource& source, const std::chrono::system_clock::time_point ts )
{
    return {
        { "bob", std::chrono::system_clock::now(), "bob", 3 }
    };
}

}

using namespace TMQ;

// Mock RefDataSource for Testing
class MockRefDataSource : public RefDataSource
{
public:
    /*MOCK_METHOD( std::vector<OwningBuffer>, fetchLatest, ( ), ( override ) );
    MOCK_METHOD( std::vector<OwningBuffer>, fetchAsOf, ( const std::chrono::system_clock::time_point ts ), ( override ) );*/

    static constexpr const char* DATA = "HELLO";
};

TEST( RefDataTests, GeneralUse )
{
    LiveRDManager<TestRefDataEntity>::onReload();

    RefData<TestRefDataEntity> rd;
    auto tmp = rd.get( "bob" );
    int x = 0;
}

//TEST( RefDataSourceTests, FetchLatestReturnsData )
//{
//    auto mockSource = std::make_shared<MockRefDataSource>();
//    EXPECT_CALL( *mockSource, fetchLatest() ).WillOnce( ::testing::Return( std::vector<OwningBuffer>{ OwningBuffer( "test data" ) } ) );
//
//    auto data = mockSource->fetchLatest();
//    ASSERT_FALSE( data.empty() );
//    EXPECT_EQ( data[0].toString(), "test data" );
//}
//
//TEST( LiveRDCacheTests, ReloadsDataFromSource )
//{
//    auto mockSource = std::make_shared<MockRefDataSource>();
//    LiveRDCache<User> cache( mockSource );
//
//    EXPECT_CALL( *mockSource, fetchLatest() ).WillOnce( ::testing::Return( std::vector<OwningBuffer>{OwningBuffer( "mock user data" ) } ) );
//
//    cache.reload();
//    auto data = cache.getData().lock();
//    EXPECT_TRUE( data );  // Data should be present
//}
//
//TEST( RefDataTests, RetrieveExistingEntity )
//{
//    LiveRDCache<User> cache;
//    auto& userCache = LiveRDManager<User>::get();
//
//    userCache.reload();
//    RefData<User> refData;
//    auto user = refData.get( "Evan" );
//    EXPECT_TRUE( user.has_value() );
//}
//
//TEST( LiveRDManagerTests, CallbacksAreTriggeredOnUpdate )
//{
//    bool callbackTriggered = false;
//    LiveRDManager<User>::registerUpdateCallback( [&] () { callbackTriggered = true; } );
//
//    LiveRDManager<User>::onReload();
//    EXPECT_TRUE( callbackTriggered );
//}
