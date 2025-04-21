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
    static const std::filesystem::path tempDir = iTempDir();
    return tempDir;
}

std::filesystem::path iLogDir()
{
    return tempDir() / "log";
}

const std::filesystem::path& logDir()
{
    static const std::filesystem::path tempDir = iLogDir();
    return tempDir;
}

}

}

