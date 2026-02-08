#include <ARQUtils/sys.h>

#include <ARQUtils/os.h>

namespace ARQ
{

namespace fs = std::filesystem;

namespace Sys
{

static fs::path iTempDir()
{
    return fs::current_path().root_path() / "tmp" / "ARQ";
}

const fs::path& tempDir()
{
    static const fs::path dir = iTempDir();
    return dir;
}

static fs::path iLogDir()
{
    return tempDir() / "log";
}

const fs::path& logDir()
{
    static const fs::path dir = iLogDir();
    return dir;
}

static fs::path iCfgDir()
{
    fs::path currentDir = OS::procPath().parent_path();

    // Walk up through the directory tree looking for config dir
    while( currentDir != currentDir.parent_path() )
    {
        const fs::path candidate = currentDir / "etc" / "ARQLib";
        if( fs::exists( candidate ) )
            return candidate;
        else
            currentDir = currentDir.parent_path();
    }

    throw ARQException( "Could not locate configuration directory 'etc/ARQLib' in any parent directory" );
}

const fs::path& cfgDir()
{
    static const fs::path dir = iCfgDir();
    return dir;
}

static fs::path iUserCfgDir()
{
    return tempDir() / "config";
}

const fs::path& userCfgDir()
{
    static const fs::path dir = iUserCfgDir();
    return dir;
}

}

}

