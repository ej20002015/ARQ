#include <TMQUtils/sys.h>

namespace TMQ
{

namespace fs = std::filesystem;

namespace Sys
{

std::filesystem::path iTempDir()
{
    return fs::current_path().root_path() / "tmp" / "TMQ";
}

const std::filesystem::path& tempDir()
{
    static const std::filesystem::path dir = iTempDir();
    return dir;
}

std::filesystem::path iLogDir()
{
    return tempDir() / "log";
}

const std::filesystem::path& logDir()
{
    static const std::filesystem::path dir = iLogDir();
    return dir;
}

std::filesystem::path iCfgDir()
{
    // TODO: Will be a place near the shipped binaries, but for now just put in temp dir
    return tempDir() / "config";
}

const std::filesystem::path& cfgDir()
{
    static const std::filesystem::path dir = iCfgDir();
    return dir;
}

const std::string_view env()
{
    // TODO: Read from environment
    return "PROD";
}

}

}

