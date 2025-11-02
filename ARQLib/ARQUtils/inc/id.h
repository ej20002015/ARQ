#pragma once
#include <ARQUtils/dll.h>

#include <array>
#include <string>
#include <format>

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

std::ostream& operator<<( std::ostream& os, const UUID& uuid )
{
    os << uuid.toString();
    return os;
}

}
}

namespace std
{

template <>
struct std::formatter<ARQ::ID::UUID> : std::formatter<std::string>
{
    auto format( ARQ::ID::UUID uuid, format_context& ctx ) const
    {
        return formatter<string>::format( std::format( "{}", uuid.toString() ), ctx );
    }
};

}