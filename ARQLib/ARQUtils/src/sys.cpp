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

static fs::path iRootCfgDir()
{
    fs::path currentDir = OS::procPath().parent_path();

    // Walk up through the directory tree looking for config dir
    while( currentDir != currentDir.parent_path() )
    {
        const fs::path candidate = currentDir / "etc" / "ARQ";
        if( fs::exists( candidate ) )
            return candidate;
        else
            currentDir = currentDir.parent_path();
    }

    throw ARQException( "Could not locate configuration directory 'etc/ARQ' in any parent directory" );
}

const fs::path& rootCfgDir()
{
    static const fs::path dir = iRootCfgDir();
    return dir;
}

const std::filesystem::path& libCfgDir()
{
    static const fs::path dir = rootCfgDir() / "ARQLib";
    return dir;
}

const std::filesystem::path& svcCfgDir()
{
	static const fs::path dir = rootCfgDir() / "services";
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

