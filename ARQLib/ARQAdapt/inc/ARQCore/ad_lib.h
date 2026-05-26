#pragma once
#include <ARQAdapt/dll.h>

#include <vector>
#include <string>

namespace ARQ
{

class LibGuard_Adapter
{
public:
	ARQAdapt_API LibGuard_Adapter( const std::vector<std::string>& args );
	ARQAdapt_API ~LibGuard_Adapter();
};

}