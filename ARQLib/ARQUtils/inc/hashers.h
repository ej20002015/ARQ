#pragma once

#include <string>

template<typename ... Bases>
struct Overload : Bases ...
{
    using is_transparent = void;
    using Bases::operator() ...;
};


struct CharPtrHash
{
    auto operator()( const char* ptr ) const noexcept
    {
        return std::hash<std::string_view>{}( ptr );
    }
};

using TransparentStringHash = Overload<
    std::hash<std::string>,
    std::hash<std::string_view>,
    CharPtrHash>;