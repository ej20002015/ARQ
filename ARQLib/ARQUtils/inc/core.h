#pragma once

// TODO: make this a pch

#include <type_traits>
#include <cstddef>

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
	// ARQ Components
	CORE,
	REFDATA,
	MKT,

	// External data systems
	CLICKHOUSE,
	GRPC,
	NATS,
	KAFKA,

	_SIZE
};

// Commonly used concepts

template<typename T>
concept c_CStrLiteral = std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, const char[]>;

template <typename T>
concept c_FuncPtr = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;

}