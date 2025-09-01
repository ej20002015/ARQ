#include <ARQCore/mktdata_source.h>

#include <ARQCore/dynalib_cache.h>
#include <ARQCore/logger.h>
#include <ARQCore/data_source_config.h>

namespace ARQ
{

std::unordered_map<std::string, std::shared_ptr<MktDataSource>, TransparentStringHash, std::equal_to<>> MktDataSourceFactory::s_customSources;

std::shared_ptr<MktDataSource> MktDataSourceFactory::create( const std::string_view dsh )
{
	if( const auto it = s_customSources.find( dsh ); it != s_customSources.end() )
		return it->second;

	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string dynaLibName;
	switch( dsc.type )
	{
		case DataSourceType::ClickHouse: dynaLibName = "ARQClickhouse"; break;
		default:
			ARQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName ); // TODO: Log when dll is being loaded for the first time?

	const auto createFunc = lib.getFunc<MktDataSourceCreateFunc>( "createMktDataSource" ); // TODO: Need to cache the loaded functions?
	return std::shared_ptr<MktDataSource>( createFunc( dsc.dsh ) );
}

void MktDataSourceFactory::addCustomSource( const std::string_view dsh, const std::shared_ptr<MktDataSource>& source )
{
	s_customSources.emplace( dsh , source );
}

void MktDataSourceFactory::delCustomSource( const std::string_view dsh )
{
	if( const auto it = s_customSources.find( dsh ); it != s_customSources.end() )
		s_customSources.erase( it );
}

std::shared_ptr<MktDataSource> GlobalMktDataSource::get()
{
	std::shared_lock<std::shared_mutex> sl( s_mut );
	if( !s_globalSourceCreator )
		s_globalSourceCreator = [] () { return MktDataSourceFactory::create( "ClickHouseDB" ); };
	
	return s_globalSourceCreator();
}

void GlobalMktDataSource::setFunc( const CreatorFunc& creatorFunc )
{
	std::unique_lock<std::shared_mutex> ul( s_mut );
	s_globalSourceCreator = creatorFunc;
}

}