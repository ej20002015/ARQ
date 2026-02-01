#pragma once
#include <ARQUtils/dll.h>

#include <array>
#include <string>
#include <format>
#include <cstdint>

namespace ARQ
{
namespace ID
{
struct UUID;

// Get ID that is unique to this session/process
ARQUtils_API UUID        getSessionID();

ARQUtils_API UUID        uuidCreate();
ARQUtils_API UUID        uuidFromStr( const std::string_view str );
ARQUtils_API std::string uuidToStr( const UUID& uuid );

// UUID v7 where the first few bytes are the unix timestamp, making them ideal for indexing in a DB and sorting
// More details: https://medium.com/@dinesharney/understanding-uuid-v4-uuid-v7-snowflake-id-and-nano-id-in-simple-terms-c50acf185b00#:~:text=UUID%20v7%20%E2%80%94%20%E2%80%9CTime%20%2B%20Random%E2%80%9D%20ID
struct UUID
{
    std::array<uint8_t, 16> bytes{ 0 };

    // C++ provides a byte-wise spaceship operator for std::array
    auto operator<=>( const UUID& other ) const = default;

    static UUID create() { return uuidCreate(); }
    static UUID fromString( const std::string_view str ) { return uuidFromStr( str ); }
    std::string toString() const { return uuidToStr( *this ); }
};

ARQUtils_API std::ostream& operator<<( std::ostream& os, const UUID& uuid );

}
}

namespace std
{

template <>
struct formatter<ARQ::ID::UUID> : formatter<string>
{
    auto format( ARQ::ID::UUID uuid, format_context& ctx ) const
    {
        return formatter<string>::format( std::format( "{}", uuid.toString() ), ctx );
    }
};

template<>
struct hash<ARQ::ID::UUID>
{
    std::size_t operator()( const ARQ::ID::UUID& uuid ) const noexcept
    {
        uint64_t p[2];
        std::memcpy( p, uuid.bytes.data(), 16 );

        std::size_t h1 = std::hash<uint64_t>{}( p[0] );
        std::size_t h2 = std::hash<uint64_t>{}( p[1] );
        // Combine them using the standard "Hash Combine" magic number
        // (0x9e3779b9 is the inverse of the golden ratio, spreading bits nicely)
        return h1 ^ ( h2 + 0x9e3779b97f4a7c15ULL + ( h1 << 6 ) + ( h1 >> 2 ) );
    }
};

}