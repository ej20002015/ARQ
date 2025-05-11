#include <TMQMarket/market_test.h>

#include <TMQCore/logger.h>

namespace TMQ
{

namespace Mkt
{

int32_t add( const int32_t x, const int32_t y )
{
	Log( Module::MKT ).info( "I'm running an add command with x={} and y={}", x, y );
	Log::flush();
	return x + y;
}

}

}