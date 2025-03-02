#include <TMQCore/error.h>
#include <gtest/gtest.h>

TEST( TMQErrorTests, TMQExceptionTest )
{
	TMQ::TMQException e( "Test exception", TMQ::ErrCode::UNSPECIFIED );
	EXPECT_EQ( e.code(), TMQ::ErrCode::UNSPECIFIED );
	EXPECT_STREQ( e.what(), "TMQException: [0]: Test exception" );
}

TEST( TMQErrorTests, TMQExceptionTest2 )
{
	TMQ::TMQException e( "Test exception 2", TMQ::ErrCode::FILE_NOT_FOUND );
	EXPECT_EQ( e.code(), TMQ::ErrCode::FILE_NOT_FOUND );
	EXPECT_STREQ( e.what(), "TMQException: [1]: Test exception 2" );
}