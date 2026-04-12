#include <ARQCore/mktdata_source.h>

#include <ARQCore/dynalib_cache.h>
#include <ARQUtils/logger.h>
#include <ARQCore/data_source_config.h>

namespace ARQ
{

std::shared_ptr<IMktDataSource> MktDataSourceFactory::create( const std::string_view dsh )
{
	if( const auto it = m_customSources.find( dsh ); it != m_customSources.end() )
		return it->second;

	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string dynaLibName;
	switch( dsc.type )
	{
		case DataSourceType::ClickHouse: dynaLibName = "ARQClickHouse"; break;
		default:
			ARQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName );

	const auto createFunc = lib.getFunc<MktDataSourceCreateFunc>( "createMktDataSource" ); // TODO: Need to cache the loaded functions?
	return std::shared_ptr<IMktDataSource>( createFunc( dsc.dsh ) );
}

void MktDataSourceFactory::addCustomSource( const std::string_view dsh, const std::shared_ptr<IMktDataSource>& source )
{
	m_customSources.emplace( dsh , source );
}

void MktDataSourceFactory::delCustomSource( const std::string_view dsh )
{
	if( const auto it = m_customSources.find( dsh ); it != m_customSources.end() )
		m_customSources.erase( it );
}

std::shared_ptr<IMktDataSource> GlobalMktDataSource::get()
{
	std::shared_lock<std::shared_mutex> sl( s_mut );
	if( !s_globalSourceCreator )
		s_globalSourceCreator = [] () { return MktDataSourceFactory::inst().create( "ClickHouseDB" ); };
	
	return s_globalSourceCreator();
}

void GlobalMktDataSource::setFunc( const CreatorFunc& creatorFunc )
{
	std::unique_lock<std::shared_mutex> ul( s_mut );
	s_globalSourceCreator = creatorFunc;
}

}