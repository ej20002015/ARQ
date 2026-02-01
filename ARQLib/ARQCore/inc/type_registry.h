#pragma once

#include <string>

namespace ARQ
{

template<typename T>
struct ARQType
{
	static_assert( false, "Type is not registered. Use ARQ_REG_TYPE( Type ) macro" );
};

}

#define ARQ_REG_TYPE( T )                                          \
template<>                                                         \
struct ARQ::ARQType<T>                                             \
{                                                                  \
	inline static constexpr std::string_view name() { return #T; } \
};
