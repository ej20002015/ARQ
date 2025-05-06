#pragma once
#include <TMQUtils/dll.h>

#include <string>
#include <chrono>
#include <concepts>

namespace TMQ
{

namespace Time
{

TMQUtils_API std::string tpToISO8601Str( const std::chrono::system_clock::time_point tp );
TMQUtils_API std::string getTmNowAsISO8601Str();

template<typename Dur>
concept c_Duration = requires {
	typename Dur::rep;
	typename Dur::period;
};

template<c_Duration Duration = std::chrono::nanoseconds>
inline uint64_t tpToLong( const std::chrono::system_clock::time_point tp )
{
	return std::chrono::duration_cast<Duration>( tp.time_since_epoch() ).count();
}

template<c_Duration Duration = std::chrono::nanoseconds>
inline std::chrono::system_clock::time_point longToTp( const uint64_t lng )
{
	return std::chrono::system_clock::time_point(
		std::chrono::duration_cast<std::chrono::system_clock::duration>( Duration( lng ) )
	);
}

}

}