#pragma once
#include <ARQUtils/dll.h>

#include <ARQUtils/core.h>

#include <string>
#include <vector>
#include <span>
#include <concepts>
#include <format>
#include <cstdint>

namespace ARQ
{

namespace Str
{

ARQUtils_API std::string wstr2Str( const std::wstring& wstr );
ARQUtils_API std::wstring str2Wstr( const std::string& str );

/**
 * @brief Checks if a substring exists within a string.
 * @param str The main string view to search within.
 * @param subStr The substring to search for.
 * @return True if the substring is found, false otherwise.
 */
constexpr bool contains( std::string_view str, std::string_view subStr ) noexcept
{
    return str.find( subStr ) != std::string_view::npos;
}

/**
 * @brief Converts a string to uppercase.
 * @param str The input string view to convert.
 * @return A new string with all characters converted to uppercase.
 */
ARQUtils_API std::string toUpper( const std::string_view str );

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
 * @brief Joins a range of formattable elements into a single string with a specified delimiter.
 * @param range The range of elements to join. Each element must be formattable with std::format.
 * @param delimiter The delimiter to use between elements.
 * @return A single string containing all joined elements.
 */
template<std::ranges::input_range R>
    requires std::formattable<std::ranges::range_reference_t<R>, char>
             && ( !std::convertible_to<std::ranges::range_reference_t<R>, std::string_view> )
std::string join( R&& range, std::string_view delimiter = "," )
{
    std::string result;
    bool first = true;

    for( auto&& v : range )
    {
        if( !first )
            result.append( delimiter );
        first = false;

        std::format_to( std::back_inserter( result ), "{}", v );
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
