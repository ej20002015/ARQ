#include <ARQCore/streaming_service.h>

#include <ARQUtils/error.h>
#include <ARQCore/data_source_config.h>
#include <ARQCore/dynalib_cache.h>

namespace ARQ
{

std::unordered_map<std::string, std::shared_ptr<IStreamProducer>, TransparentStringHash, std::equal_to<>> StreamingServiceFactory::s_customStreamProducers;
std::unordered_map<std::string, std::shared_ptr<IStreamConsumer>, TransparentStringHash, std::equal_to<>> StreamingServiceFactory::s_customStreamConsumers;

std::shared_ptr<IStreamProducer> StreamingServiceFactory::createProducer( const std::string_view dsh, const StreamProducerOptions& options )
{
	if( const auto it = s_customStreamProducers.find( dsh ); it != s_customStreamProducers.end() )
		return it->second;

	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string dynaLibName;
	switch( dsc.type )
	{
		case DataSourceType::Kafka: dynaLibName = "ARQKafka"; break;
		default:
			ARQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName );

	const auto createFunc = lib.getFunc<CreateStreamProducerFunc>( "createStreamProducer" );
	return std::shared_ptr<IStreamProducer>( createFunc( dsc.dsh, options ) );
}

std::shared_ptr<IStreamConsumer> StreamingServiceFactory::createConsumer( const std::string_view dsh, const StreamConsumerOptions& options )
{
	if( const auto it = s_customStreamConsumers.find( dsh ); it != s_customStreamConsumers.end() )
		return it->second;

	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string dynaLibName;
	switch( dsc.type )
	{
		case DataSourceType::Kafka: dynaLibName = "ARQKafka"; break;
		default:
			ARQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName );

	const auto createFunc = lib.getFunc<CreateStreamConsumerFunc>( "createStreamConsumer" );
	return std::shared_ptr<IStreamConsumer>( createFunc( dsc.dsh, options ) );
}

void StreamingServiceFactory::addCustomStreamProducer( const std::string_view dsh, const std::shared_ptr<IStreamProducer>& streamProducer )
{
	s_customStreamProducers.emplace( dsh, streamProducer );
}

void StreamingServiceFactory::delCustomStreamProducer( const std::string_view dsh )
{
	if( const auto it = s_customStreamProducers.find( dsh ); it != s_customStreamProducers.end() )
		s_customStreamProducers.erase( it );
	else
		throw ARQException( std::format( "Cannot find custom stream producer with dsh={} to delete", dsh ) );
}

ARQCore_API void StreamingServiceFactory::addCustomStreamConsumer( const std::string_view dsh, const std::shared_ptr<IStreamConsumer>& streamConsumer )
{
	s_customStreamConsumers.emplace( dsh, streamConsumer );
}

ARQCore_API void StreamingServiceFactory::delCustomStreamConsumer( const std::string_view dsh )
{
	if( const auto it = s_customStreamConsumers.find( dsh ); it != s_customStreamConsumers.end() )
		s_customStreamConsumers.erase( it );
	else
		throw ARQException( std::format( "Cannot find custom stream consumer with dsh={} to delete", dsh ) );
}

}
