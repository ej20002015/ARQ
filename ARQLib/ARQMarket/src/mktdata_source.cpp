#include <ARQMarket/mktdata_source.h>

#include <ARQCore/dynalib_cache.h>
#include <ARQCore/data_source_config.h>

namespace ARQ::MD
{

std::shared_ptr<IMarketSource> MarketSourceFactory::create( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );

	if( const auto it = m_sources.find( dsh ); it != m_sources.end() )
		return it->second;

	const DataSourceConfig& dsc         = DataSourceConfigManager::inst().get( dsh );
	const std::string_view  dynaLibName = dataSourceTypeToDynaLibName( dsc.type );
	const OS::DynaLib&      lib         = DynaLibCache::inst().get( dynaLibName );

	const auto createFunc = lib.getFunc<MarketSourceCreateFunc>( "createMarketSource" );
	auto newSource = std::shared_ptr<IMarketSource>( createFunc( dsh ) );

	return m_sources.emplace( dsh, std::move( newSource ) ).first->second;
}

void MarketSourceFactory::addCustomSource( const std::string_view dsh, const std::shared_ptr<IMarketSource>& source )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );

	if( !m_sources.emplace( dsh, source ).second )
		throw ARQException( std::format( "MD::MarketSourceFactory: Cannot add custom source with dsh={} as it already exists", dsh ) );
}

void MarketSourceFactory::delCustomSource( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );

	if( const auto it = m_sources.find( dsh ); it != m_sources.end() )
		m_sources.erase( it );
	else
		throw ARQException( std::format( "MD::MarketSourceFactory: Cannot find custom source with dsh={} to delete", dsh ) );
}

}