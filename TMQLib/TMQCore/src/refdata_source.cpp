#include <TMQCore/refdata_source.h>

#include <TMQCore/dynalib_cache.h>
#include <TMQCore/logger.h>

namespace TMQ
{

std::shared_ptr<RefDataSource> TMQ::RefDataSourceRepo::get( const Type type )
{
	{
		std::shared_lock<std::shared_mutex> sl( s_mut );
		if( s_sources[type] )
			return s_sources[type];
	}

	{
		std::unique_lock<std::shared_mutex> ul( s_mut );
		// Need to check again as source may have been added between unlocking and re-locking
		if( s_sources[type] )
			return s_sources[type];

		std::string dynaLibName;
		switch( type )
		{
			case Type::ClickHouse: dynaLibName = "TMQClickhouse"; break;
			default:
				TMQ_ASSERT( false );
		}

		Log( Module::REFDATA ).info( "Loading dynalib {} to get RefDataSource", dynaLibName );
		const OS::DynaLib& lib = DynaLibCache::inst().get( "TMQClickHouse" );

		const auto createFunc = lib.getFunc<RefDataSourceCreateFunc>( "createRefDataSource" );
		RefDataSource* source = createFunc();
		s_sources[type] = std::shared_ptr<RefDataSource>( source );
		return s_sources[type];
	}
}

std::shared_ptr<RefDataSource> GlobalRefDataSource::get()
{
	std::shared_lock<std::shared_mutex> sl( s_mut );
	if( !s_globalSourceCreator )
		s_globalSourceCreator = [] () { return RefDataSourceRepo::get( RefDataSourceRepo::ClickHouse ); };

	return s_globalSourceCreator();	
}

void GlobalRefDataSource::setFunc( const CreatorFunc& creatorFunc )
{
	std::unique_lock<std::shared_mutex> ul( s_mut );
	s_globalSourceCreator = creatorFunc;
}

}