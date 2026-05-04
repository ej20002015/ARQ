#pragma once
#include <ARQClickHouse/dll.h>

#include <ARQMarket/mktdata_source.h>

namespace ARQ::CH::MD
{

extern "C" ARQClickHouse_API ARQ::MD::IMarketSource* createMarketSource( const std::string_view dsh );

class CHMarketSource : public ARQ::MD::IMarketSource
{
public:
	CHMarketSource( const std::string_view dsh )
		: m_dsh( dsh )
	{}

	ARQ::MD::RecordCollection load( const std::string_view marketName, const ARQ::MD::TIDSet& filter )            override;
	void                      save( const std::string_view marketName, const ARQ::MD::RecordCollection& records ) override;

private:
	std::string m_dsh;
};

}