#pragma once
#include <ARQUtils/dll.h>

#include <ARQUtils/core.h>

#include <string>
#include <vector>
#include <span>
#include <concepts>
#include <format>
#include <cstdint>
#include <memory>

namespace ARQ
{

namespace Str
{

/**
* @brief A fixed-size string wrapper that can be used in compile-time contexts.
 */
template <size_t N>
struct FixedString
{
    char value[N];

    constexpr FixedString( const char( &str )[N] )
    {
        std::copy_n( str, N, value );
    }

    constexpr std::string_view view() const
    {
        return std::string_view( value, N - 1 );
    }
};

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
 * @brief Splits a string into substrings separated by a delimiter.
 *
 * Empty fields are preserved. The returned string_views reference the
 * original input string.
 *
 * @param input     The string to split.
 * @param delimiter The delimiter character.
 *
 * @return A vector of std::string_view tokens.
 *
 * @note The returned string_views are only valid as long as @p input remains alive.
 */
ARQUtils_API std::vector<std::string_view> split( std::string_view input, char delimiter = ',' );

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
 * @brief Joins a range of elements, applying a specific format string to each element.
 * @param range The range of elements to join.
 * @param elementFmt The format string to apply to each element (e.g., "'{}'").
 * @param delimiter The delimiter to use between formatted elements.
 * @return A single string containing all formatted and joined elements.
 */
template<std::ranges::input_range R>
    requires std::formattable<std::ranges::range_reference_t<R>, char>
std::string joinFmt( R&& range, std::string_view elementFmt, std::string_view delimiter = "," )
{
    std::string result;

	if constexpr( std::ranges::sized_range<R> ) // If range has accessible size, reserve capacity to avoid multiple reallocations
        result.reserve( std::ranges::size( range ) * ( 16 + elementFmt.size() + delimiter.size() ) );

    bool first = true;

    for( auto&& v : range )
    {
        if( !first )
            result.append( delimiter );
        first = false;

        std::vformat_to( std::back_inserter( result ), elementFmt, std::make_format_args( v ) );
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
