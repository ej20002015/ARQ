#include <gtest/gtest.h>
#include <ARQUtils/str.h>

using namespace ARQ;

TEST( StrUtilsTest, ConvertWStr2Str )
{
    const std::wstring wstr = L"hello";
    const std::string  str  = Str::wstr2Str( wstr );
    ASSERT_EQ( str, "hello" );
}

TEST( StrUtilsTest, ConvertStr2WStr )
{
    const std::string  str  = "hello";
    const std::wstring wstr = Str::str2Wstr( str );
    ASSERT_EQ( wstr, L"hello" );
}