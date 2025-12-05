#include <ARQCore/messaging_service.h>

#include <ARQUtils/core.h>
#include <ARQCore/dynalib_cache.h>
#include <ARQCore/data_source_config.h>

namespace ARQ
{

std::unordered_map<std::string, std::shared_ptr<IMessagingService>, TransparentStringHash, std::equal_to<>> MessagingServiceFactory::s_customServices;

std::shared_ptr<IMessagingService> MessagingServiceFactory::create( const std::string_view dsh )
{
	if( const auto it = s_customServices.find( dsh ); it != s_customServices.end() )
		return it->second;

	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string dynaLibName;
	switch( dsc.type )
	{
		case DataSourceType::NATS: dynaLibName = "ARQNats"; break;
		default:
			ARQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName );

	const auto createFunc = lib.getFunc<MessagingServiceCreateFunc>( "createMessagingService" );
	return std::shared_ptr<IMessagingService>( createFunc( dsc.dsh ), [] (IMessagingService* ) { ; } ); // Lifetime handled by dlls so use null deleter
}

void MessagingServiceFactory::addCustomService( const std::string_view dsh, const std::shared_ptr<IMessagingService>& service )
{
	s_customServices.emplace( dsh, service );
}

void MessagingServiceFactory::delCustomService( const std::string_view dsh )
{
	if( const auto it = s_customServices.find( dsh ); it != s_customServices.end() )
		s_customServices.erase( it );
	else
		throw ARQException( std::format( "Cannot find custom messaging service with dsh={} to delete" , dsh ) );
}

}