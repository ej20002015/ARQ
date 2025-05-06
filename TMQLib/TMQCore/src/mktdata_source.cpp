#include <TMQCore/mktdata_source.h>

#include <TMQCore/dynalib_cache.h>
#include <TMQCore/logger.h>

namespace TMQ
{

std::shared_ptr<MktDataSource> TMQ::MktDataSourceRepo::get( const Type type )
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

		Log( Module::REFDATA ).info( "Loading dynalib {} to get MktDataSource", dynaLibName );
		const OS::DynaLib& lib = DynaLibCache::inst().get( "TMQClickHouse" );

		const auto createFunc = lib.getFunc<MktDataSourceCreateFunc>( "createMktDataSource" );
		MktDataSource* source = createFunc();
		s_sources[type] = std::shared_ptr<MktDataSource>( source );
		return s_sources[type];
	}
}

std::shared_ptr<MktDataSource> GlobalMktDataSource::get()
{
	std::shared_lock<std::shared_mutex> sl( s_mut );
	if( !s_globalSourceCreator )
		s_globalSourceCreator = [] () { return MktDataSourceRepo::get( MktDataSourceRepo::ClickHouse ); };
	
	return s_globalSourceCreator();
}

void GlobalMktDataSource::setFunc( const CreatorFunc& creatorFunc )
{
	std::unique_lock<std::shared_mutex> ul( s_mut );
	s_globalSourceCreator = creatorFunc;
}

}