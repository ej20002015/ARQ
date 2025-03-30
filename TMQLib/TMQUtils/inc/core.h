#pragma once
#include <TMQCore/dll.h>

#include <type_traits>

TMQCore_API int multiply( const int x, const int y );

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

// Commonly used concepts

template<typename T>
concept c_CStrLiteral = std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, const char[]>;

}