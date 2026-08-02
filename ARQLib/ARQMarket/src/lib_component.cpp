#include <ARQCore/lib_component.h>
#include <ARQMarket/mktdata_live_store.h>
#include <ARQMarket/mktdata_source.h>
#include <ARQUtils/error.h>

#include <memory>

namespace ARQ::MD
{

namespace
{

class MarketLibComponent : public ILibComponent
{
public:
	MarketLibComponent()
	{
		MarketSourceFactory::setGlobalInst( &m_marketSourceFactory );
		LiveMarketStoreFactory::setGlobalInst( &m_liveMarketStoreFactory );
	}

	~MarketLibComponent() override
	{
		LiveMarketStoreFactory::setGlobalInst( nullptr );
		MarketSourceFactory::setGlobalInst( nullptr );
	}

private:
	MarketSourceFactory    m_marketSourceFactory;
	LiveMarketStoreFactory m_liveMarketStoreFactory;
};

const LibComponentRegistry::Reg MARKET_LIB_COMPONENT_REGISTRATION( LibComponentRegistration{
	.name    = "ARQMarket",
	.order   = {
		.phase    = LibComponentPhase::FinancialFoundation,
		.position = 100,
	},
	.factory = [] () { return std::make_unique<MarketLibComponent>(); },
} );

}

}
