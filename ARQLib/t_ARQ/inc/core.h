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

inline std::vector<std::string> getLibArgs( int argc, char** argv, const std::string& testsNamespace )
{
    std::vector<std::string> args;

	for( int i = 1; i < argc; ++i )
        args.emplace_back( argv[i] );

    args.emplace_back( "--log.dest2" );
    args.emplace_back( ARQ::getLogFilepathForTests( testsNamespace ).string() );

    return args;
}

}