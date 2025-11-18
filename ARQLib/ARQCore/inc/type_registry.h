#pragma once

#include <string>

namespace ARQ
{

template<typename T>
class ARQType
{
	static_assert( false, "Type is not registered. Use REG_ARQ_TYPE( Type ) macro" );
};

#define REG_ARQ_TYPE( T )                                          \
template<>                                                         \
class ARQType<T>                                                   \
{                                                                  \
public:                                                            \
	inline static constexpr std::string_view name() { return #T; } \
};

}