#pragma once

#include <ARQUtils/core.h>

#include <concepts>

namespace ARQ
{

template<typename Tag, typename T>
class StrongType
{
public:
	explicit StrongType( const T val ) noexcept
		: m_val( val )
	{}
	StrongType()
		: m_val( T{} )
	{}

	operator T() const noexcept { return m_val; }
	T val() const noexcept { return m_val; }

private:
	T m_val;
};

template<std::invocable OnDtr>
class Defer
{
public:
	Defer( OnDtr func ) : m_func( func ) {}
	~Defer() { m_func(); }

private:
	OnDtr m_func;
};

#define ARQDefer Defer d([&]()

// Generates classes and helpers to define type categories
#define ARQ_DEFINE_TYPE_CATEGORY( NAME )                                   \
    /* The Trait Struct (Default: False) */                                \
    template<typename T> struct Is##NAME : std::false_type {};             \
                                                                           \
    /* The Value Helper */                                                 \
    template<typename T> constexpr bool is##NAME##_v = Is##NAME<T>::value; \
                                                                           \
    /* The Concept */                                                      \
    template<typename T> concept c_##NAME = is##NAME##_v<T>;

// Macro to register a type as belonging to a category
#define ARQ_REGISTER_CATEGORY( TRAIT, TYPE ) \
    template<> struct Is##TRAIT<TYPE> : std::true_type {};

}