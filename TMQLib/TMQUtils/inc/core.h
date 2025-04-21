#pragma once

#include <type_traits>

#ifdef _WIN32
	#define TMQ_BREAK() __debugbreak()
#else
	#define TMQ_BREAK()
#endif

#ifdef NDEBUG
	#define TMQ_ASSERT( x )
#else
	#define TMQ_ASSERT( x ) { if( !( x ) ) { TMQ_BREAK(); } }
#endif

namespace TMQ
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

	_SIZE
};

static constexpr const char* const MODULE_STRS[static_cast<size_t>( Module::_SIZE )] = { "REFDATA", "CORE" };

// Commonly used concepts

template<typename T>
concept c_CStrLiteral = std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, const char[]>;

}