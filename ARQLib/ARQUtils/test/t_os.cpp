#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <ARQUtils/os.h>

#include <ARQUtils/error.h>

#include <thread>
#include <iostream>
#include <type_traits>

using ::testing::HasSubstr;
using namespace ARQ;

TEST( OSUtilsTest, GetProcNameTest )
{
    std::string_view p = OS::procName();
    EXPECT_THAT( p, HasSubstr( "t_ARQUtils" ) );
}

TEST( OSUtilsTest, GetProcIDTest )
{
    int32_t pid = OS::procID();
    ASSERT_GT( pid, -1 );
}

TEST( OSUtilsTest, GetThreadID )
{
    int32_t tid = OS::threadID();
    ASSERT_GT( tid, -1 );
}

TEST( OSUtilsTest, GetUnnamedThreadName )
{
    std::string_view threadName = OS::threadName();
    #ifdef _WIN32
        ASSERT_STREQ( threadName.data(), "unnamed_thread" );
    #else
        ASSERT_STREQ( threadName.data(), "t_ARQUtils" ); // Linux defaults thread name to process name
    #endif
}

TEST( OSUtilsTest, SetAndGetThreadName )
{
    OS::setThreadName( "TestThread" );
    std::string_view threadName = OS::threadName();
    ASSERT_EQ( threadName, "TestThread" );
}

TEST( OSUtilsTest, SetThreadNameMutipleTimes )
{
    OS::setThreadName( "TestThread" );
    std::string_view threadName = OS::threadName();
    ASSERT_EQ( threadName, "TestThread" );
    OS::setThreadName( "AnotherThread" );
    threadName = OS::threadName();
    ASSERT_EQ( threadName, "AnotherThread" );
}

TEST( OSUtilsTest, SetTooLongThreadName )
{
    EXPECT_THROW( {
        try
        {
            OS::setThreadName( "A very looooooooong thread name" );
        }
        catch( const ARQException& e )
        {
            EXPECT_THAT( e.what(), HasSubstr ("exceeds maximum" ) );
            throw;
        }
    }, ARQException );
}

TEST( OSUtilsTest, DynaLibNotLoadedOnConstruction )
{
    OS::DynaLib d;
    ASSERT_FALSE( d.isLoaded() );
}

TEST( OSUtilsTest, DynaLibLoadNonExistent )
{
    OS::DynaLib d;
    EXPECT_THROW( {
        try
        {
            d.load( "NotADynaLib" );
        }
        catch( const ARQException& e )
        {
            EXPECT_THAT( e.what(), HasSubstr( "Could not load dynalib" ) );
            throw;
        }
    }, ARQException );
}
