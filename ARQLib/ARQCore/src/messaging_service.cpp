#include <ARQCore/messaging_service.h>

#include <ARQUtils/core.h>
#include <ARQCore/dynalib_cache.h>
#include <ARQCore/data_source_config.h>

namespace ARQ
{

std::shared_ptr<IMessagingService> MessagingServiceFactory::create( const std::string_view dsh )
{
	if( const auto it = m_customServices.find( dsh ); it != m_customServices.end() )
		return it->second;

	const DataSourceConfig& dsc         = DataSourceConfigManager::inst().get( dsh );
	const std::string_view  dynaLibName = dataSourceTypeToDynaLibName( dsc.type );
	const OS::DynaLib&      lib         = DynaLibCache::inst().get( dynaLibName );

	const auto createFunc = lib.getFunc<MessagingServiceCreateFunc>( "createMessagingService" );
	return std::shared_ptr<IMessagingService>( createFunc( dsc.dsh ), [] (IMessagingService* ) { ; } ); // Lifetime handled by dlls so use null deleter
}

void MessagingServiceFactory::addCustomService( const std::string_view dsh, const std::shared_ptr<IMessagingService>& service )
{
	m_customServices.emplace( dsh, service );
}

void MessagingServiceFactory::delCustomService( const std::string_view dsh )
{
	if( const auto it = m_customServices.find( dsh ); it != m_customServices.end() )
		m_customServices.erase( it );
	else
		throw ARQException( std::format( "Cannot find custom messaging service with dsh={} to delete" , dsh ) );
}

}