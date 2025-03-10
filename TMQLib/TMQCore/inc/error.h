#pragma once

#include <exception>
#include <format>
#include <string>
#include <cstdint>

namespace TMQ
{

enum class ErrCode : uint16_t // TODO: Make a generic enum generator
{
	UNSPECIFIED = 0,
	FILE_NOT_FOUND,
	INVALID_INPUT,
	OUT_OF_BOUNDS,
};

// TODO: Change to the OmegaException class from cppcon talk

class TMQException : public std::exception
{
  public:
	TMQException() = default;

	TMQException( const std::string_view str, ErrCode code = ErrCode::UNSPECIFIED ) : m_code( code )
	{
		m_str = std::format( "TMQException: [{}]: {}", static_cast<uint16_t>( m_code ), str );
	}

	char const* what() const noexcept override { return m_str.c_str(); }

	[[nodiscard]] ErrCode code() const noexcept { return m_code; }

  private:
	std::string m_str;
	ErrCode		m_code;
};

} // namespace TMQ