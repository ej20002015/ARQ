#pragma once
#include <ARQRedis/dll.h>

#include <ARQCore/serialiser.h>
#include <ARQMarket/mktdata_source.h>


namespace ARQ::Redis::MD
{

extern "C" ARQRedis_API ARQ::MD::IMarketSource* createMarketSource( const std::string_view dsh );

class RedisMarketSource : public ARQ::MD::IMarketSource
{
public:
	RedisMarketSource( const std::string_view dsh )
		: m_dsh( dsh )
	{
		m_serialiser = SerialiserFactory::inst().create( SerialiserFactory::SerialiserImpl::Protobuf );
	}

	ARQ::MD::RecordCollection load( const std::string_view marketName, const ARQ::MD::TIDSet& filter )            override;
	void                      save( const std::string_view marketName, const ARQ::MD::RecordCollection& records ) override;

private:
	std::string                  m_dsh;
	std::shared_ptr<Serialiser>  m_serialiser;
};


}