#pragma once

#include <format>
#include <string>
#include <string_view>

namespace ARQ::Redis::Keys
{

inline std::string market( const std::string_view marketName )
{
	return std::format( "ARQ:Markets:{}", marketName );
}

inline std::string streamOffsets( const std::string_view key )
{
	return std::format( "ARQ:StreamOffsets:{}", key );
}

inline std::string liveMarketOffsets( const std::string_view marketName )
{
	return streamOffsets( std::format( "Markets:{}", marketName ) );
}

}
