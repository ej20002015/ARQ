#pragma once

#include <ranges>
#include <set>
#include <concepts>

namespace ARQ::Algos
{

/**
 * @brief Builds an effective set from include/default/disabled ranges.
 *
 * Uses @p include if non-empty, otherwise @p defaultRange.
 * Removes any elements present in @p disabled.
 *
 * All ranges must have value types convertible to a common type.
 *
 * @tparam IncludeRange  Range providing explicitly included elements.
 * @tparam DefaultRange  Range providing fallback elements.
 * @tparam DisabledRange Range providing elements to exclude.
 *
 * @param include       Elements to include if non-empty.
 * @param defaultRange  Elements used when include is empty.
 * @param disabled      Elements to remove from the resulting set.
 *
 * @return A std::set containing the effective elements.
 */
template<
    std::ranges::input_range IncludeRange,
    std::ranges::input_range DefaultRange,
    std::ranges::input_range DisabledRange>
    requires requires
{
    typename std::common_type_t<
        std::ranges::range_value_t<IncludeRange>,
        std::ranges::range_value_t<DefaultRange>,
        std::ranges::range_value_t<DisabledRange>>;
}
auto makeEffectiveSet(
    const IncludeRange& include,
    const DefaultRange& defaultRange,
    const DisabledRange& disabled )
{
    using T = std::common_type_t<
        std::ranges::range_value_t<IncludeRange>,
        std::ranges::range_value_t<DefaultRange>,
        std::ranges::range_value_t<DisabledRange>>;

    std::set<T> result;

    if( !include.empty() )
    {
        for( const auto& v : include )
            result.insert( static_cast<T>( v ) );
    }
    else
    {
        for( const auto& v : defaultRange )
            result.insert( static_cast<T>( v ) );
    }

    for( const auto& d : disabled )
        result.erase( d );

    return result;
}

}