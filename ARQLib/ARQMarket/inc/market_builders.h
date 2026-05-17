#pragma once
#include <ARQMarket/dll.h>

#include <ARQMarket/market.h>
#include <ARQMarket/market_live.h>

#include <string>
#include <memory>

namespace ARQ::MD
{

template<typename Derived>
class BaseMarketBuilder
{
public:
	Derived& fromSource( const std::string_view mktSrcDSH, const MarketName& mktName, const TIDSet& tidSet = TIDSet() )
	{
		m_mktSrcDSH     = mktSrcDSH;
		m_mktSrcMktName = mktName;
		m_mktSrcTIDSet  = tidSet;
		return static_cast<Derived&>( *this );
	}

protected:
	std::string m_mktSrcDSH;
	MarketName  m_mktSrcMktName;
	TIDSet      m_mktSrcTIDSet;
};

class LiveMarketBuilder : public BaseMarketBuilder<LiveMarketBuilder>
{
public:
	ARQMarket_API LiveMarketBuilder& withMessagingFeed( const std::string_view msgSvcDSH, const TIDSet& tidSet = TIDSet(), const MarketName& mktName = MarketName() );

	ARQMarket_API std::pair<std::shared_ptr<Market>, std::shared_ptr<LiveMarketUpdater>> build();

private:
	std::string m_msgSvcDSH;
	TIDSet      m_msgTIDSet;
	MarketName  m_msgMktName;
};

}