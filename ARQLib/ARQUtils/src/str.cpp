#include <ARQUtils/str.h>

#include <locale>
#include <codecvt>
#include <algorithm>

namespace ARQ
{

namespace Str
{

std::string wstr2Str( const std::wstring& wstr )
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes( wstr );
}

std::wstring str2Wstr( const std::string& str )
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.from_bytes( str );
}

std::string toUpper( const std::string_view str )
{
    std::string upperStr;
	upperStr.reserve( str.size() );
    std::transform( str.begin(), str.end(), std::back_inserter( upperStr ),
		[] ( unsigned char c ) { return std::toupper( c ); } );
	return upperStr;
}

}

}