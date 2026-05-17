#pragma once

#include <string>
#include <ankerl/unordered_dense.h>

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

struct AnkerlCharPtrHash
{
    auto operator()( const char* ptr ) const noexcept
    {
        return ankerl::unordered_dense::hash<std::string_view>{}( ptr );
    }
};

using AnkerlTransparentStringHash = Overload<
    ankerl::unordered_dense::hash<std::string>,
    ankerl::unordered_dense::hash<std::string_view>,
    AnkerlCharPtrHash
>;