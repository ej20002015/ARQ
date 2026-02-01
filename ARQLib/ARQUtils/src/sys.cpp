#include <ARQUtils/sys.h>

namespace ARQ
{

namespace fs = std::filesystem;

namespace Sys
{

fs::path iTempDir()
{
    return fs::current_path().root_path() / "tmp" / "ARQ";
}

const fs::path& tempDir()
{
    static const fs::path dir = iTempDir();
    return dir;
}

fs::path iLogDir()
{
    return tempDir() / "log";
}

const fs::path& logDir()
{
    static const fs::path dir = iLogDir();
    return dir;
}

fs::path iCfgDir()
{
    // TODO: Will be a place near the shipped binaries, but for now just put in temp dir
    return tempDir() / "config";
}

const fs::path& cfgDir()
{
    static const fs::path dir = iCfgDir();
    return dir;
}

}

}

