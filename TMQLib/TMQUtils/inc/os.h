#pragma once
#include <TMQUtils/dll.h>

#include <TMQUtils/core.h>
#include <TMQUtils/error.h>

#include <string>
#include <cstdint>

namespace TMQ
{

namespace OS
{

[[nodiscard]] TMQUtils_API std::string_view procName();
[[nodiscard]] TMQUtils_API int32_t procID();

[[nodiscard]] TMQUtils_API std::string_view threadName();
TMQUtils_API void setThreadName( const std::string_view name );
[[nodiscard]] TMQUtils_API int32_t threadID();

class DynaLib
{
public:
	TMQUtils_API DynaLib() = default;
	TMQUtils_API explicit DynaLib( const std::string_view name );
	~DynaLib();

	void load( const std::string_view name );
	void unload();

	[[nodiscard]] TMQUtils_API bool isLoaded() const noexcept { return m_nativeHandle != nullptr; }

	template<c_FuncPtr FuncPtr>
	[[nodiscard]] FuncPtr getFunc( const std::string_view funcName ) const;

private:
	void setName( const std::string_view name );

	void resetError() const;
	[[nodiscard]] std::string getLastError() const;

	[[nodiscard]] TMQUtils_API void* iGetFunc( const std::string_view funcName ) const;

private:
	void* m_nativeHandle = nullptr;
	std::string m_name;
};

template<c_FuncPtr FuncPtr>
inline FuncPtr DynaLib::getFunc( const std::string_view funcName ) const
{
	return reinterpret_cast<FuncPtr>( iGetFunc( funcName ) );
}

}

}