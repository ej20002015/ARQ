#include <TMQUtils/time.h>

#include <sstream>
#include "time.h"

namespace TMQ
{

namespace Time
{

TMQUtils_API std::string tpToISO8601Str( const std::chrono::system_clock::time_point tp )
{
	using namespace std::chrono;

	auto now_time_t = system_clock::to_time_t( tp );
	auto micros = duration_cast<microseconds>( tp.time_since_epoch() ) % 1'000'000;

	std::ostringstream oss;
	oss << std::put_time( std::gmtime( &now_time_t ), "%FT%T" );
	oss << '.' << std::setw( 6 ) << std::setfill( '0' ) << micros.count() << 'Z';
	return oss.str();
}

std::string getTmNowAsISO8601Str()
{
	using namespace std::chrono;

	auto now = system_clock::now();
	return tpToISO8601Str( now );
}

}

}