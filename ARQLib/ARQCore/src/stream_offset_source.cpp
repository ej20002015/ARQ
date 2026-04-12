#include <ARQCore/stream_offset_source.h>

#include <ARQCore/data_source_config.h>
#include <ARQCore/dynalib_cache.h>

namespace ARQ
{

std::shared_ptr<IStreamOffsetSource> StreamOffsetSourceFactory::create( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );

	if( const auto it = m_sources.find( dsh ); it != m_sources.end() )
		return it->second;

	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string dynaLibName;
	switch( dsc.type )
	{
		case DataSourceType::Redis: dynaLibName = "ARQRedis"; break;
		default:
			ARQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName );

	const auto createFunc = lib.getFunc<StreamOffsetSourceCreateFunc>( "createStreamOffsetSource" );
	auto newSource = std::shared_ptr<IStreamOffsetSource>( createFunc( dsh ) );

	return m_sources.emplace( dsh, std::move( newSource ) ).first->second;
}

void StreamOffsetSourceFactory::addCustomSource( const std::string_view dsh, const std::shared_ptr<IStreamOffsetSource>& source )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );
	m_sources.emplace( dsh, source );
}

void StreamOffsetSourceFactory::delCustomSource( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );

	if( const auto it = m_sources.find( dsh ); it != m_sources.end() )
		m_sources.erase( it );
}

}