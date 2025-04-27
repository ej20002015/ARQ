#pragma once

#include <filesystem>
#include <string>

namespace TMQ
{

namespace Sys
{

const std::filesystem::path& tempDir();
const std::filesystem::path& logDir();
const std::filesystem::path& cfgDir();

const std::string_view env();

}

}