#pragma once
#include <ARQUtils/dll.h>

#include <ARQUtils/core.h>
#include <ARQUtils/error.h>

#include <string>
#include <cstdint>
#include <filesystem>

namespace ARQ
{

namespace OS
{

[[nodiscard]] ARQUtils_API const std::filesystem::path& procPath();
[[nodiscard]] ARQUtils_API std::string_view             procName();
[[nodiscard]] ARQUtils_API int32_t                      procID();

[[nodiscard]] ARQUtils_API std::string_view threadName();
              ARQUtils_API void             setThreadName( const std::string_view name );
[[nodiscard]] ARQUtils_API int32_t          threadID();

class DynaLib
{
public:
	ARQUtils_API DynaLib() = default;
	ARQUtils_API explicit DynaLib( const std::string_view name );
	ARQUtils_API ~DynaLib();

	ARQUtils_API void load( const std::string_view name );
	ARQUtils_API void unload();

	[[nodiscard]] ARQUtils_API bool isLoaded() const noexcept { return m_nativeHandle != nullptr; }

	template<c_FuncPtr FuncPtr>
	[[nodiscard]] FuncPtr getFunc( const std::string_view funcName ) const;

private:
	void setName( const std::string_view name );

	void resetError() const;
	[[nodiscard]] std::string getLastError() const;

	[[nodiscard]] ARQUtils_API void* iGetFunc( const std::string_view funcName ) const;

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