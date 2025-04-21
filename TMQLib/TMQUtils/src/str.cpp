#include <TMQUtils/str.h>

#include <locale>
#include <codecvt>

namespace TMQ
{

namespace Str
{

std::string Str::wstr2Str( const std::wstring& wstr )
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes( wstr );
}

std::wstring Str::str2Wstr( const std::string& str )
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.from_bytes( str );
}

}

}