#pragma once

// TODO: make this a pch

#include <type_traits>
#include <cstddef>

#ifdef _WIN32
	#define ARQ_BREAK() __debugbreak()
#else
	#define ARQ_BREAK()
#endif

#ifdef NDEBUG
	#define ARQ_ASSERT( x )
#else
	#define ARQ_ASSERT( x ) { if( !( x ) ) { ARQ_BREAK(); } }
#endif

namespace ARQ
{

enum class DoThrow
{
	NO,
	YES
};

struct Empty
{
};

enum class Module
{
	REFDATA,
	CORE,
	CLICKHOUSE,
	MKT,

	_SIZE
};

static constexpr const char* const MODULE_STRS[static_cast<size_t>( Module::_SIZE )] = { "REFDATA", "CORE", "CLICKHOUSE", "MKT" };

// Commonly used concepts

template<typename T>
concept c_CStrLiteral = std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, const char[]>;

template <typename T>
concept c_FuncPtr = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;

}