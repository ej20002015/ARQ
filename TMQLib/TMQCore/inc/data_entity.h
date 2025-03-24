#pragma once

#include <string>
#include <chrono>

namespace TMQ
{

struct DataEntity
{
	std::string _lastUpdatedBy;
	std::chrono::system_clock::time_point _lastUpdatedTm;
};

}