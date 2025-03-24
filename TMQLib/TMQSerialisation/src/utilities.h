#pragma once

#include <chrono>
#include <cstdint>

namespace TMQ
{

uint64_t tpToLong( const std::chrono::system_clock::time_point tp );
std::chrono::system_clock::time_point longToTp( const uint64_t lng );

}