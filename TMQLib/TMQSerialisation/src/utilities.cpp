#include "utilities.h"

namespace TMQ
{

uint64_t tpToLong( const std::chrono::system_clock::time_point tp )
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>( tp.time_since_epoch() ).count();
}

std::chrono::system_clock::time_point longToTp( const uint64_t lng )
{
	return std::chrono::system_clock::time_point(
		std::chrono::duration_cast<std::chrono::system_clock::duration>( std::chrono::nanoseconds( lng ) )
	);
}

}