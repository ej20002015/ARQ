#pragma once

#include <ARQUtils/sys.h>

#include <filesystem>
#include <string>

namespace ARQ
{

inline std::filesystem::path getLogFilepathForTests( const std::string& testsNamespace )
{
    return Sys::logDir() / "tests" / ( testsNamespace + ".log" );
}

}