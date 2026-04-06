#pragma once
#include <ARQUtils/dll.h>

#include <filesystem>

namespace ARQ
{

namespace Sys
{

ARQUtils_API const std::filesystem::path& tempDir();
ARQUtils_API const std::filesystem::path& logDir();
ARQUtils_API const std::filesystem::path& rootCfgDir();
ARQUtils_API const std::filesystem::path& libCfgDir();
ARQUtils_API const std::filesystem::path& svcCfgDir();
ARQUtils_API const std::filesystem::path& userCfgDir();

}

}