#pragma once

#include <string>
#include <chrono>

namespace TMQ
{

struct DataEntity
{
	std::chrono::system_clock::time_point _lastUpdatedTs;
	std::string _lastUpdatedBy;
};

}