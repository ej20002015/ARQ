#include <ARQMarket/market.h>

namespace ARQ::MD
{

// -------------- MarketName implementation --------------

const MarketName MarketName::LIVE = { "LIVE", Time::Date() };

MarketName MarketName::fromStr( const std::string_view nameStr )
{
    size_t delimPos = nameStr.find( '|' );
    if( delimPos == std::string_view::npos )
        return MarketName( std::string( nameStr ) );

    std::string tag( nameStr.substr( 0, delimPos ) );
    std::string dateStr( nameStr.substr( delimPos + 1 ) );
    Time::Date date = Time::Date( dateStr, "%Y%m%d" );
    return MarketName( tag, date );
}

std::string MarketName::str() const
{
    return tag + ( date.isSet() ? std::format( "|{:%Y%m%d}", date ) : "" );
}

}