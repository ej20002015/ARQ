#include <ARQUtils/str.h>

#include <locale>
#include <codecvt>

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

}

}