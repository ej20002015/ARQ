#pragma once

#include <ARQUtils/core.h>
#include <ARQUtils/error.h>

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

/*
 * @brief A simple optional const reference wrapper. It can either contain a reference to an object of type T, or be empty (nullopt).
*/
template<typename T>
class OptConstRef
{
public:
	OptConstRef( const T* ptr = nullptr )
		: m_ptr( ptr )
	{}

	operator      bool()           const noexcept { return m_ptr != nullptr; }
	[[nodiscard]] bool has_value() const noexcept { return m_ptr != nullptr; }

	[[nodiscard]] const T& value() const
	{
		if( m_ptr )
			return *m_ptr;
		else
			throw ARQException( "Attempted to access value of an OptConstRef that does not contain a value." );
	}

	const T& operator*() const noexcept  { return *m_ptr; }
	const T* operator->() const noexcept { return m_ptr;  }
	
private:
	const T* m_ptr;
};

enum class DoThrow
{
	NO,
	YES
};

struct Empty
{
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
#define ARQ_REG_CATEGORY( TRAIT, TYPE ) \
    template<> struct Is##TRAIT<TYPE> : std::true_type {};

}