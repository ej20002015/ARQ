#include <ARQMarket/mktdata_live_store.h>

#include <ARQCore/data_source_config.h>
#include <ARQCore/dynalib_cache.h>

namespace ARQ::MD
{

std::shared_ptr<ILiveMarketStore> LiveMarketStoreFactory::create( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lg( m_storesMutex );

	if( const auto it = m_stores.find( dsh ); it != m_stores.end() )
		return it->second;

	const DataSourceConfig& dsc         = DataSourceConfigManager::inst().get( dsh );
	const std::string_view  dynaLibName = dataSourceTypeToDynaLibName( dsc.type );
	const OS::DynaLib&      lib         = DynaLibCache::inst().get( dynaLibName );

	const auto createFunc = lib.getFunc<LiveMarketStoreCreateFunc>( "createLiveMarketStore" );
	auto newStore = std::shared_ptr<ILiveMarketStore>( createFunc( dsh ) );

	return m_stores.emplace( dsh, std::move( newStore ) ).first->second;
}

void LiveMarketStoreFactory::addCustomStore( const std::string_view dsh, const std::shared_ptr<ILiveMarketStore>& store )
{
	std::lock_guard<std::mutex> lg( m_storesMutex );

	if( !m_stores.emplace( dsh, store ).second )
		throw ARQException( std::format( "MD::LiveMarketStoreFactory: Cannot add custom store with dsh={} as it already exists", dsh ) );
}

void LiveMarketStoreFactory::delCustomStore( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lg( m_storesMutex );

	if( const auto it = m_stores.find( dsh ); it != m_stores.end() )
		m_stores.erase( it );
	else
		throw ARQException( std::format( "MD::LiveMarketStoreFactory: Cannot find custom store with dsh={} to delete", dsh ) );
}

}
