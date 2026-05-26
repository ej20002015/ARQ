#include <ARQAdapt/ARQCore/ad_lib.h>

#include <ARQCore/lib.h>

namespace ARQ
{

LibGuard_Adapter::LibGuard_Adapter( const std::vector<std::string>& args )
{
	libInit( args );
}

LibGuard_Adapter::~LibGuard_Adapter()
{
	libShutdown();
}

}