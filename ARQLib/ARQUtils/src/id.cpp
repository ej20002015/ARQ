#include <ARQUtils/id.h>

#include "uuidv7.h"

#include <ARQUtils/time.h>
#include <ARQUtils/error.h>

#include <random>
#include <cstring>

namespace ARQ
{
namespace ID
{

UUID getSessionID()
{
	static const UUID sessionID = uuidCreate();
	return sessionID;
}

static void fillRandomBytes( uint8_t* buffer, size_t len )
{
	static thread_local std::mt19937_64 t_rng{ std::random_device{}() };
	for( size_t i = 0; i < len; i += 8 )
	{
		uint64_t r = t_rng();
		size_t chunk = std::min<size_t>( 8, len - i );
		std::memcpy( buffer + i, &r, chunk );
	}
}

static UUID iuuidCreate( const uint8_t* uuidPrev = nullptr )
{
	static constexpr auto RAND_LEN = 10;

	const uint64_t unixTs = Time::unixTimestampMillis();
	uint8_t randBuf[RAND_LEN] = { 0 };
	fillRandomBytes( randBuf, RAND_LEN );
	UUID uuidOut;
	uuidv7_generate( uuidOut.bytes.data(), unixTs, randBuf, uuidPrev );
	return uuidOut;
}

UUID uuidCreate()
{
	static thread_local UUID t_uuidPrev = iuuidCreate();
	t_uuidPrev = iuuidCreate( t_uuidPrev.bytes.data() );
	return t_uuidPrev;
}

UUID uuidFromStr( const std::string_view str )
{
	UUID out;
	auto failure = uuidv7_from_string( str.data(), out.bytes.data() );
	if( failure )
		throw ARQException( std::format( "Cannot convert string=\"{}\" to uuid", str ) );
	return out;
}

std::string uuidToStr( const UUID& uuid )
{
	static constexpr auto STR_LEN = 36;

	char strBuf[STR_LEN + 1] = { '\0' };
	uuidv7_to_string( uuid.bytes.data(), strBuf );
	return std::string( strBuf, STR_LEN );
}

std::ostream& operator<<( std::ostream& os, const UUID& uuid )
{
	os << uuid.toString();
	return os;
}

}
}
