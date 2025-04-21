#pragma once
#include <TMQUtils/dll.h>

#include <string>
#include <cstdint>

namespace TMQ
{

namespace OS
{

TMQUtils_API std::string_view procName();
TMQUtils_API int32_t procID();

TMQUtils_API std::string_view threadName();
TMQUtils_API void setThreadName( const std::string_view name );
TMQUtils_API int32_t threadID();
}

}