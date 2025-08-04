#include <TMQCore/mktdata_source.h>

#include <TMQCore/dynalib_cache.h>
#include <TMQCore/logger.h>
#include <TMQCore/data_source_config.h>

namespace TMQ
{

std::shared_ptr<MktDataSource> MktDataSourceFactory::create( const std::string_view dsh )
{
	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string dynaLibName;
	switch( dsc.type )
	{
		case DataSourceType::ClickHouse: dynaLibName = "TMQClickhouse"; break;
		default:
			TMQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName ); // TODO: Log when dll is being loaded for the first time?

	const auto createFunc = lib.getFunc<MktDataSourceCreateFunc>( "createMktDataSource" ); // TODO: Need to cache the loaded functions?
	return std::shared_ptr<MktDataSource>( createFunc( dsc.dsh ) );
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