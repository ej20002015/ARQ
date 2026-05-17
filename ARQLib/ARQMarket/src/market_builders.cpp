#include <ARQMarket/market_builders.h>

namespace ARQ::MD
{

LiveMarketBuilder& LiveMarketBuilder::withMessagingFeed( const std::string_view msgSvcDSH, const TIDSet& tidSet, const MarketName& mktName )
{
    m_msgSvcDSH  = msgSvcDSH;
    m_msgTIDSet  = tidSet;
    m_msgMktName = mktName;

    return *this;
}

std::pair<std::shared_ptr<Market>, std::shared_ptr<LiveMarketUpdater>> LiveMarketBuilder::build()
{
    if( m_msgSvcDSH.empty() )
        throw ARQException( "Cannot build live market without specifying a messaging feed" );
    if( !m_mktSrcMktName.isSet() && !m_msgMktName.isSet() )
        throw ARQException( "Cannot build live market with no mkt name specified - if fromSource() has not been called then mktName must be provided in withMessagingService() call" );

    // Just make a blank market; LiveMarketUpdater handles population of data
    auto mkt = std::make_shared<Market>();

    auto updater = LiveMarketUpdater::create( LiveMarketUpdater::Params{
        .mkt          = mkt,
        .mktSrcDSH    = m_mktSrcDSH,
        .msgSvcDSH    = m_msgSvcDSH,
        .mktName      = m_mktSrcMktName.isSet() ? m_mktSrcMktName : m_msgMktName,
        .mktSrcTIDSet = m_mktSrcTIDSet,
        .msgTIDSet    = m_msgTIDSet
    } );

    return std::make_pair( std::move( mkt ), std::move( updater ) );
}

}