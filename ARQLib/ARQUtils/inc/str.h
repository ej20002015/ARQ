#pragma once

#include <string>
#include <vector>
#include <span>

namespace ARQ
{

namespace Str
{

std::string wstr2Str( const std::wstring& wstr );
std::wstring str2Wstr( const std::string& str );

/**
* @brief Joins a range of strings into a single string with a specified delimiter.
* @param range The range of strings to join. Each element must be convertible to std::string_view.
* @param delimiter The delimiter to use between strings.
* @return A single string containing all joined strings.
 */
template<std::ranges::input_range R>
    requires std::convertible_to<std::ranges::range_reference_t<R>, std::string_view>
std::string join( R&& range, std::string_view delimiter = "," )
{
    std::string result;
    bool first = true;

    for( auto&& s : range )
    {
        if( !first )
            result += delimiter;
        first = false;

        result += std::string_view( s );
    }

    return result;
}

}

}