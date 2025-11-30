#pragma once

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

template<typename OnDtr>
class Defer
{
public:
	Defer( OnDtr func ) : m_func( func ) {}
	~Defer() { m_func(); }

private:
	OnDtr m_func;
};

#define ARQDefer    Defer d([&]()

}