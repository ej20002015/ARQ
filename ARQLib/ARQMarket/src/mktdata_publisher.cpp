#include <ARQMarket/mktdata_publisher.h>

#include <ARQUtils/logger.h>

namespace ARQ::MD
{

void Publisher::init( const Config& config )
{
	m_config         = config;
	m_streamProducer = StreamingServiceFactory::createProducer( m_config.streamingServiceDSH, StreamProducerOptions( "MD::Publisher", StreamProducerOptions::Preset::LowLatency ) );
}

void Publisher::registerOnErrorCallback( const PublisherErrorCallback& callback )
{
	std::shared_lock<std::shared_mutex> sl( m_globalErrorCallbacksMut );
	m_globalErrorCallbacks.push_back( callback );
}

void Publisher::publishImpl( Buffer&& buf, const std::string key, const std::string_view topic, const std::string_view payloadType, const std::optional<PublisherErrorCallback>& errorCallback )
{
	StreamProducerMessage msg;
	msg.topic               = topic;
	msg.data                = SharedBuffer( std::move( buf ) ); // Get kafka to own the buffer
	msg.key                 = key;
	msg.headers["ARQ_Type"] = payloadType;

	m_streamProducer->send( msg, [this, errorCallback] ( const StreamProducerMessageMetadata& messageMetadata, std::optional<StreamError> error )
	{
		if( error )
			handlePublishError( messageMetadata, *error, errorCallback );
	} );
}

void Publisher::handlePublishError( const StreamProducerMessageMetadata& messageMetadata, StreamError error, const std::optional<PublisherErrorCallback>& errorCallback )
{
	Log( Module::MKT ).error( "MD::Publisher: Failed to publish message to topic {0}: {1} (Fatal: {2}, Retriable: {3}, TransRequiresAbort: {4})",
		messageMetadata.topic,
		error.message,
		error.isFatal,
		error.isRetriable.has_value() ? std::to_string( error.isRetriable.value() ) : "unknown",
		error.transRequiresAbort );

	if( errorCallback )
		errorCallback.value()( messageMetadata, error );

	std::shared_lock<std::shared_mutex> sl( m_globalErrorCallbacksMut );
	for( const PublisherErrorCallback& callback : m_globalErrorCallbacks )
		callback( messageMetadata, error );
}

}