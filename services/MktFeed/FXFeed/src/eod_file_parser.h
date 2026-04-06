#pragma once

#include <unordered_map>
#include <filesystem>

struct EODRecord 
{
    double rate;
    double volatility;
};

class EODParser 
{
public:
    static std::unordered_map<int64_t, EODRecord> load( const std::filesystem::path& filepath );
};