#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/types.h>
#include <ARQUtils/error.h>

#include <chrono>
#include <optional>
#include <string>
#include <variant>

namespace ARQ
{

class Variant;

class VariantArr
{
public:
	VariantArr() = default;

	ARQCore_API VariantArr( const uint32_t rows, const uint32_t cols = 1 );

	ARQCore_API VariantArr( const VariantArr& other );
	ARQCore_API VariantArr& operator=( const VariantArr& other );

	ARQCore_API VariantArr( VariantArr&& other ) noexcept;
	ARQCore_API VariantArr& operator=( VariantArr&& other ) noexcept;

	[[nodiscard]] ARQCore_API const Variant& at( const uint32_t row, const uint32_t col = 0 ) const;
	[[nodiscard]] ARQCore_API Variant&       at( const uint32_t row, const uint32_t col = 0 );

	[[nodiscard]] uint32_t rows() const noexcept { return m_nRows; }
	[[nodiscard]] uint32_t cols() const noexcept { return m_nCols; }

private:
	std::unique_ptr<Variant[]> m_data;
	uint32_t m_nRows = 0, m_nCols = 0;
};

template<typename T>
concept c_SupportedVariantType = std::same_as<T, Empty> ||
							     std::same_as<T, int32_t> ||
							     std::same_as<T, int64_t> ||
							     std::same_as<T, double> ||
							     std::same_as<T, bool> ||
							     std::same_as<T, std::string> ||
							     std::same_as<T, std::chrono::system_clock::time_point> ||
							     std::same_as<T, ARQException> ||
							     std::same_as<T, VariantArr>;

class Variant
{
public:

	// Supported types
	using ValueType = std::variant
					  <Empty,					             // Empty - default constructor
					  int32_t,							     // 32-bit integer
					  int64_t,							     // 64-bit integer
					  double,								 // Double-precision float
					  bool,								     // Boolean
					  std::string,						     // ASCII string
					  std::chrono::system_clock::time_point, // DateTime (time_point)
					  ARQException,                          // Exception
					  VariantArr>;					         // 2D array of Variant

	// Constructors
	Variant() = default;
	Variant( const ValueType& val )
		: m_val( val )
	{
	}

	// Universal constructor to handle any supported type
	template<typename T>
	Variant( T&& val )
	{
		// Special case for C-string literals (const char[] or const char*)
		if constexpr( c_CStrLiteral<T> )
		{
			m_val = std::string( std::forward<T>( val ) ); // Convert to std::string
		}
		else if constexpr( c_SupportedVariantType<std::decay_t<T>> )
		{
			m_val = std::forward<T>( val );
		}
		else
		{
			static_assert( false, "Unsupported type for Variant" );
		}
	}

	// Accessors
	const ValueType& get() const { return m_val; }

	// Strict type-safe accessor
	template<c_SupportedVariantType T>
	[[nodiscard]] const T& as() const
	{
		if( std::holds_alternative<T>( m_val ) )
			return std::get<T>( m_val );
		else
			throw ARQException( "Variant: Type mismatch" );
	}

	// Strict type-safe accessor
	template<c_SupportedVariantType T>
	[[nodiscard]] T& as()
	{
		return const_cast< T& >( const_cast< const Variant* >( this )->as<T>() );
	}

	// Try to access the value as a specific type
	template<c_SupportedVariantType T>
	[[nodiscard]] std::optional<std::reference_wrapper<const T>> tryAs() const
	{
		if( std::holds_alternative<T>( m_val ) )
			return std::cref( std::get<T>( m_val ) ); // Use std::cref to create a const reference wrapper
		else
			return std::nullopt;
	}

	template<c_SupportedVariantType T>
	[[nodiscard]] std::optional<std::reference_wrapper<T>> tryAs()
	{
		if( std::holds_alternative<T>( m_val ) )
			return std::ref( std::get<T>( m_val ) ); // Use std::ref to create a mutable reference wrapper
		else
			return std::nullopt;
	}

	// Helper function to check the type
	template<c_SupportedVariantType T>
	[[nodiscard]] bool is() const
	{
		return std::holds_alternative<T>( m_val );
	}

private:
	ValueType m_val;
};

} // namespace ARQ