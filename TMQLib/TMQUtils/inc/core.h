// TODO: Rename file and create pre-compiled header

#pragma once

#include <type_traits>

#include "dll.h"

TMQ_API int multiply( const int x, const int y );

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