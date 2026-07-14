#pragma once
#include <ARQRedis/dll.h>

#include <ARQCore/serialiser.h>
#include <ARQMarket/mktdata_live_store.h>

namespace ARQ::Redis::MD
{

extern "C" ARQRedis_API ARQ::MD::ILiveMarketStore* createLiveMarketStore( const std::string_view dsh );

class RedisLiveMarketStore : public ARQ::MD::ILiveMarketStore
{
public:
	RedisLiveMarketStore( const std::string_view dsh )
		: m_dsh( dsh )
	{
		m_serialiser = SerialiserFactory::inst().create( SerialiserFactory::SerialiserImpl::Protobuf );
	}

	void apply( const ARQ::MD::MarketUpdateBatch& updateBatch ) override;

private:
	std::string                 m_dsh;
	std::shared_ptr<Serialiser> m_serialiser;
};

}
