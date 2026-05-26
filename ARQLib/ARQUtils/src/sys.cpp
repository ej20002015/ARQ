#include <ARQUtils/sys.h>

#include <ARQUtils/os.h>

#include <mutex>

namespace ARQ
{

namespace fs = std::filesystem;

namespace Sys
{

static std::filesystem::path s_ARQHome;
static std::mutex s_ARQHomeMutex;

ARQUtils_API void setARQHome( const std::filesystem::path& path )
{
    std::lock_guard<std::mutex> lock( s_ARQHomeMutex );
    s_ARQHome = path;
}

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
    // First check if ARQHome config variable is set and points to a valid config directory

    std::filesystem::path arqHome;
    {
		std::lock_guard<std::mutex> lock( s_ARQHomeMutex );
		arqHome = s_ARQHome;
    }
    
    if( !arqHome.empty() )
    {
        const fs::path candidate = arqHome / "etc" / "ARQ";
        if( fs::exists( candidate ) )
			return candidate;
		else
			throw ARQException( std::format( "Environment variable ARQ_HOME is set to [{}] but configuration directory 'etc/ARQ' does not exist at that location", arqHome.string() ) );
    }

    // If not, start from the executable's directory and walk up through parent directories looking for 'etc/ARQ'

    fs::path currentDir = OS::procPath().parent_path();

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

