#include <ARQKafka/kafka_streaming_service_interface.h>

#include <ARQUtils/id.h>
#include <ARQCore/data_source_config.h>
#include <ARQCore/logger.h>

#include <kafka/KafkaProducer.h>

namespace ARQ
{

// ------------------------ Constructor -------------------------

KafkaStreamProducer::KafkaStreamProducer( const std::string_view dsh, const StreamProducerOptions& options )
	: m_dsh( dsh )
	, m_options( options )
{
	connect();
}

// ------------------------ IStreamProducer implementation -------------------------

void KafkaStreamProducer::send( const StreamProducerMessage& msg, const StreamProducerDeliveryCallbackFunc& callback )
{
	using namespace kafka::clients::producer;

	SharedBuffer keepAliveBuf;
	kafka::Value value;

	if( const SharedBuffer* sharedBuf = std::get_if<SharedBuffer>( &msg.data ) )
	{
		// Extend lifetime of SharedBuffer
		value        = kafka::Value( sharedBuf->getDataPtrAs<const char*>(), sharedBuf->size );
		keepAliveBuf = *sharedBuf;
	}
	else if( const BufferView* view = std::get_if<BufferView>( &msg.data ) )
    {
        // Only given a view - No ownership taken.
        value = kafka::Value(view->data, view->size);
    }
	else
		ARQ_ASSERT( false );

	const kafka::Key key = msg.key ? kafka::Key( msg.key->data(), msg.key->size() ) : kafka::NullKey;
	
	std::optional<ProducerRecord> recordOpt;
	if( msg.partition )
		recordOpt = msg.id ? ProducerRecord( msg.topic.data(), *msg.partition, key, value, *msg.id )
		                   : ProducerRecord( msg.topic.data(), *msg.partition, key, value );
	else
	    recordOpt = msg.id ? ProducerRecord( msg.topic.data(), key, value, *msg.id )
		                   : ProducerRecord( msg.topic.data(), key, value );

	ProducerRecord& record = *recordOpt;
	for( const auto& [headerKey, headerValue] : msg.headers )
	{
		kafka::Header::Value headerValueObj( headerValue.data(), headerValue.size() + 1 ); // +1 to include null terminator
		record.headers().emplace_back( headerKey, headerValueObj );
	}

	const auto kafkaCallback = [callback, keepAliveBuf] ( const RecordMetadata& metadata, const kafka::Error& err )
	{
		if( callback )
		{
			StreamProducerMessageMetadata messageMetadata {
				.messageID       = metadata.recordId(),
				.topic           = metadata.topic(),
				.partition       = metadata.partition(),
				.offset          = metadata.offset(),
				.keySize         = metadata.keySize(),
				.valueSize       = metadata.valueSize(),
				.timestamp       = Time::DateTime( std::chrono::system_clock::time_point( metadata.timestamp() ) ),
				.persistedStatus = StreamMessagePersistedStatus::UNKNOWN
			};

			std::optional<std::string> errorStr = err ? std::make_optional<std::string>( err.message() ) : std::nullopt;

			callback( messageMetadata, errorStr );
		}
		else if( err )
			Log( Module::KAFKA ).error( "KafkaStreamProducer: Message delivery failed for topic [{}]: {}", metadata.topic(), err.toString() );
	};

	try
	{
		m_kafkaProducer->send( record, kafkaCallback );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer: Failed to send message for topic [{}]: {}", msg.topic, e.what() ) );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer: Failed to send message for topic [{}]: {}", msg.topic, e.what() ) );
	}
	catch( ... )
	{
		throw ARQException( std::format( "KafkaStreamProducer: Failed to send message for topic [{}]: unknown error", msg.topic ) );
	}
}

void KafkaStreamProducer::flush( const std::chrono::milliseconds timeout )
{
	Log( Module::KAFKA ).debug( "KafkaStreamProducer: Flushing producer with timeout of {} ms", timeout.count() );

	if( const kafka::Error err = m_kafkaProducer->flush( timeout ) )
		Log( Module::KAFKA ).error( "KafkaStreamProducer: Error when flushing: {}", err.toString() );
}

// ------------------------ Connection -------------------------

void KafkaStreamProducer::connect()
{
	try
	{
		kafka::Properties props = buildProperties();
		m_kafkaProducer = std::make_unique<kafka::clients::producer::KafkaProducer>( props );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer: Failed to create kafka producer for dsh [{}]: {}", m_dsh, e.what() ) );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer: Failed to create kafka producer for dsh [{}]: {}", m_dsh, e.what() ) );
	}
	catch( ... )
	{
		throw ARQException( std::format( "KafkaStreamProducer: Failed to create kafka producer for dsh [{}]: unknown error", m_dsh ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamProducer: Created kafka producer for dsh [{}]", m_dsh );
}

kafka::Properties KafkaStreamProducer::buildProperties()
{
	kafka::Properties props;

	// Connection urls
	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( m_dsh );
	std::string brokerURLList;
	brokerURLList.reserve( dsc.connPropsMap.size() * 30 ); // rough estimate
	for( const auto& [_, connProps] : dsc.connPropsMap )
		brokerURLList += std::format( "{}:{},", connProps.hostname, connProps.port );
	brokerURLList.pop_back(); // remove trailing comma
	props.put( "bootstrap.servers", brokerURLList );

	// Ensure exactly-once delivery
	props.put( "enable.idempotence", "true" );

	// Set client id to session ID
	props.put( "client.id", std::format( "ARQKafka.Session-{}", ID::getSessionID() ) );

	// Set callbacks for logging
	props.put( "error_cb", [] ( const kafka::Error& err )
	{
		Log( Module::KAFKA ).error( "Kafka Global Error: {}", err.toString() );
	} );
	props.put( "log_cb", [] ( int level, const char* filename, int lineno, const char* msg )
	{
		switch( level )
		{
			case 0:
			case 1:
			case 2:
				Log( Module::KAFKA, { { "provider", "kafka" } } ).critical( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			case 3:
				Log( Module::KAFKA, { { "provider", "kafka" } } ).error( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			case 4:
				Log( Module::KAFKA, { { "provider", "kafka" } } ).warn( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			case 5:
			case 6:
				Log( Module::KAFKA, { { "provider", "kafka" } } ).info( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			case 7:
				Log( Module::KAFKA, { { "provider", "kafka" } } ).debug( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			default:
				Log( Module::KAFKA, { { "provider", "kafka" } } ).trace( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
		}
	} );
	// TODO: set stats_cb when got a Prometheus endpoint

	// Apply preset configurations
	if( m_options.getPreset() == StreamProducerOptions::Preset::OPTIMISE_LATENCY )
	{
		props.put( "linger.ms", "0" );
		props.put( "batch.size", "16384" ); // 16KB
		props.put( "compression.type", "none" );
		props.put( "socket.nagle.disable", "true" );
	}
	else if( m_options.getPreset() == StreamProducerOptions::Preset::OPTIMISE_THROUGHPUT )
	{
		props.put( "linger.ms", "20" );
		props.put( "batch.size", "131072" ); // 128KB
		props.put( "compression.type", "lz4" );
	}
	// else use defaults

	// Apply any option overrides
	for( const auto& [option, value] : m_options.getOptionOverrides() )
		props.put( option, value );

	return props;
}

}