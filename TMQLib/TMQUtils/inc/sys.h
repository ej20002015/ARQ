#pragma once

#include <filesystem>

namespace TMQ
{

namespace Sys
{

const std::filesystem::path& tempDir();
const std::filesystem::path& logDir();

}

}