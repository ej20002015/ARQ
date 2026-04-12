#include <ARQCore/refdata_source.h>

#include <ARQCore/dynalib_cache.h>
#include <ARQUtils/logger.h>
#include <ARQCore/data_source_config.h>

namespace ARQ::RD
{

void ARQ::RD::Source::setDSH( const std::string_view dsh )
{
	m_dsh = dsh;
	for( auto& [_, entitySource] : m_entitySources )
		entitySource->setDSH( dsh );
}

std::shared_ptr<Source> SourceFactory::create( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );

	if( const auto it = m_sources.find( dsh ); it != m_sources.end() )
		return it->second;

	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string dynaLibName;
	switch( dsc.type )
	{
		case DataSourceType::ClickHouse: dynaLibName = "ARQClickHouse"; break;
		case DataSourceType::gRPC:       dynaLibName = "ARQGrpc";       break;
		default:
			ARQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName );

	const auto registerSourcesFunc = lib.getFunc<RegisterEntitySourcesFunc>( "registerEntitySources" );
	auto newSource = std::make_shared<Source>();
	registerSourcesFunc( newSource.get() );
	newSource->setDSH( dsh );

	return m_sources.emplace( dsh, std::move( newSource ) ).first->second;
}

void SourceFactory::addCustomSource( const std::string_view dsh, const std::shared_ptr<Source>& source )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );

	if( !m_sources.emplace( dsh, source ).second )
		throw ARQException( std::format( "RD::SourceFactory: Cannot add custom source with dsh={} as it already exists", dsh ) );
}

void SourceFactory::delCustomSource( const std::string_view dsh )
{
	std::lock_guard<std::mutex> lg( m_sourcesMutex );

	if( const auto it = m_sources.find( dsh ); it != m_sources.end() )
		m_sources.erase( it );
	else
		throw ARQException( std::format( "RD::SourceFactory: Cannot find custom source with dsh={} to delete", dsh ) );
}

}