#pragma once

// TODO: make this a pch

#include <type_traits>
#include <cstddef>
#include <sstream>

namespace ARQ
{

enum class Module
{
	// Special exe module that shows the proc name in the logs
	EXE,

	// ARQ Components
	CORE,
	REFDATA,
	MKT,

	// External data systems
	CLICKHOUSE,
	GRPC,
	NATS,
	KAFKA,
	HTTP,

	_SIZE
};

// Commonly used concepts

template<typename T>
concept c_CStrLiteral = std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, const char[]>;

template <typename T>
concept c_FuncPtr = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;

template<typename T>
concept c_Enum = std::is_enum_v<T>;

template<typename T>
concept OstreamWritable =
	requires( std::ostream& os, T&& value )
{
	os << std::forward<T>( value );
};

}