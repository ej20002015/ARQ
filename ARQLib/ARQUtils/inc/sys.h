#pragma once
#include <ARQUtils/dll.h>

#include <filesystem>
#include <string>

namespace ARQ
{

namespace Sys
{

ARQUtils_API const std::filesystem::path& tempDir();
ARQUtils_API const std::filesystem::path& logDir();
ARQUtils_API const std::filesystem::path& cfgDir();

}

}