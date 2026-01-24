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

/**
* @brief Computes a compile-time FNV-1a hash for a given string view.
* @param str The input string view to hash.
* @return The computed 64-bit hash value.
 */
constexpr uint64_t constexprHash( const std::string_view str )
{
	// FNV-1a 64-bit hash
    uint64_t hash = 14695981039346656037ull;
    for( char c : str )
    {
        hash ^= static_cast<uint64_t>( c );
        hash *= 1099511628211ull;
    }
    return hash;
}

}

}