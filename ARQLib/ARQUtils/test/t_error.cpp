#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <ARQUtils/error.h>

using ::testing::HasSubstr;
using namespace ARQ;

// Test basic exception message
TEST( ARQExceptionTest, BasicMessageTest )
{
    ARQException ex( "Test error message" );
    EXPECT_EQ( ex.what(), "Test error message" );
}

// Test exception with source location
TEST( ARQExceptionTest, SourceLocationTest )
{
    ARQException ex( "Location test" );
    auto loc = ex.where();
    EXPECT_STREQ( loc.file_name(), __FILE__ );
    EXPECT_THAT( loc.function_name(), HasSubstr( "ARQExceptionTest_SourceLocationTest_Test" ) );
}

// Test const correctness of what()
TEST( ARQExceptionTest, WhatConstTest )
{
    const ARQException ex( "Const test" );
    EXPECT_EQ( ex.what(), "Const test" );
}

// Ensure exception can be thrown and caught
TEST( ARQExceptionTest, ThrowCatchTest )
{
    try
    {
        throw ARQException( "Exception throw test" );
    }
    catch( const ARQException& ex )
    {
        EXPECT_EQ( ex.what(), "Exception throw test" );
    }
    catch( ... )
    {
        FAIL() << "Unexpected exception type caught";
    }
}
