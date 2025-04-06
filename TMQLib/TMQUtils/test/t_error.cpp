#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <TMQUtils/error.h>

using ::testing::HasSubstr;
using namespace TMQ;

// Test basic exception message
TEST( TMQExceptionTest, BasicMessageTest )
{
    TMQException ex( "Test error message" );
    EXPECT_EQ( ex.what(), "Test error message" );
}

// Test exception with source location
TEST( TMQExceptionTest, SourceLocationTest )
{
    TMQException ex( "Location test" );
    auto loc = ex.where();
    EXPECT_STREQ( loc.file_name(), __FILE__ );
    EXPECT_THAT( loc.function_name(), HasSubstr( "MQExceptionTest_SourceLocationTest_Test" ) );
}

// Test const correctness of what()
TEST( TMQExceptionTest, WhatConstTest )
{
    const TMQException ex( "Const test" );
    EXPECT_EQ( ex.what(), "Const test" );
}

// Ensure exception can be thrown and caught
TEST( TMQExceptionTest, ThrowCatchTest )
{
    try
    {
        throw TMQException( "Exception throw test" );
    }
    catch( const TMQException& ex )
    {
        EXPECT_EQ( ex.what(), "Exception throw test" );
    }
    catch( ... )
    {
        FAIL() << "Unexpected exception type caught";
    }
}
