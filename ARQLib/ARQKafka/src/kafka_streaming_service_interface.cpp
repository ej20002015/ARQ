#include <ARQKafka/kafka_streaming_service_interface.h>

#include <ARQUtils/id.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/str.h>
#include <ARQUtils/logger.h>
#include <ARQCore/data_source_config.h>

#include <kafka/KafkaProducer.h>

namespace ARQ
{

// -------------------------- Helpers ---------------------------

static std::string getBrokerURLListForDSH( const std::string_view dsh )
{
	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );

	std::string brokerURLList;
	brokerURLList.reserve( dsc.connPropsMap.size() * 30 ); // rough estimate
	for( const auto& [_, connProps] : dsc.connPropsMap )
		brokerURLList += std::format( "{}:{},", connProps.hostname, connProps.port );
	brokerURLList.pop_back(); // remove trailing comma

	return brokerURLList;
}

enum class ClientType
{
	PRODUCER,
	CONSUMER
};

static void setCommonConfig( kafka::Properties& props, const std::string_view dsh, const ClientType clientType, const std::string_view clientName )
{
	// Connection URLs
	props.put( "bootstrap.servers", getBrokerURLListForDSH( dsh ) );

	// Set client id to a combo of session ID, client type and client name
	props.put( "client.id", std::format( "ARQKafka.Session-{}.{}.{}", ID::getSessionID(), Enum::enum_name( clientType ), clientName ) );

	// Set callbacks for logging
	props.put( "error_cb", [] ( const kafka::Error& err )
	{
		Log( Module::KAFKA ).error( "Kafka Global Error: {}", err.toString() );
	} );
	props.put( "log_cb", [] ( int level, const char* filename, int lineno, const char* msg )
	{
		switch( level )
		{
			case 0: // EMERG
			case 1: // ALERT
			case 2: // CRITICAL
				Log( Module::KAFKA, { { "provider", "kafka" } } ).critical( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			case 3: // ERROR
				Log( Module::KAFKA, { { "provider", "kafka" } } ).error( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			case 4: // WARNING
				Log( Module::KAFKA, { { "provider", "kafka" } } ).warn( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			case 5: // NOTICE
			case 6: // INFO
				Log( Module::KAFKA, { { "provider", "kafka" } } ).info( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			case 7: // DEBUG
				Log( Module::KAFKA, { { "provider", "kafka" } } ).debug( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
			default: // Any others
				Log( Module::KAFKA, { { "provider", "kafka" } } ).trace( "Kafka Log: ({}:{}) {}", filename, lineno, msg );
				break;
		}
	} );

	// TODO: set stats_cb when got a Prometheus endpoint
}

static StreamMessagePersistedStatus persistedStatusFromKafkaStatus( const kafka::clients::producer::RecordMetadata::PersistedStatus status )
{
	using PS = kafka::clients::producer::RecordMetadata::PersistedStatus;
	switch( status )
	{
		case PS::Not:      return StreamMessagePersistedStatus::NOT_PERSISTED;
		case PS::Done:     return StreamMessagePersistedStatus::PERSISTED;
		case PS::Possibly:
		default:           return StreamMessagePersistedStatus::UNKNOWN;
	}
}

static StreamError kafkaErrorToStreamError( const kafka::Error& err )
{
	return StreamError{
		.code               = err.value(),
		.message            = err.message(),
		.isFatal            = err.isFatal(),
		.isRetriable        = err.isRetriable(),
		.transRequiresAbort = err.transactionRequiresAbort()
	};
}

#pragma region Producer Implementation

/*
*********************************************
*   Implementation of KafkaStreamProducer   *
*********************************************
*/

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
		kafka::Header::Value headerValueObj( headerValue.data(), headerValue.size() );
		record.headers().emplace_back( headerKey, headerValueObj );
	}

	const auto kafkaCallback = [this, callback, keepAliveBuf] ( const RecordMetadata& metadata, const kafka::Error& err )
	{
		if( callback )
		{

			StreamProducerMessageMetadata messageMetadata {
				.messageID       = metadata.recordId(),
				.topic           = std::move( metadata.topic() ),
				.partition       = metadata.partition(),
				.offset          = metadata.offset(),
				.keySize         = metadata.keySize(),
				.valueSize       = metadata.valueSize(),
				.timestamp       = Time::DateTime( std::chrono::system_clock::time_point( metadata.timestamp() ) ),
				.persistedStatus = persistedStatusFromKafkaStatus( metadata.persistedStatus() )
			};

			std::optional<StreamError> streamingError;
			if( err )
				streamingError = kafkaErrorToStreamError( err );

			callback( messageMetadata, streamingError );
		}
		else if( err )
			Log( Module::KAFKA ).error( "KafkaStreamProducer[{}]: Message delivery failed for topic [{}]: {}", m_options.name(), metadata.topic(), err.toString() );
	};

	try
	{
		m_kafkaProducer->send( record, kafkaCallback );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Failed to send message for topic [{}]: {}", m_options.name(), msg.topic, e.what() ) );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Failed to send message for topic [{}]: {}", m_options.name(), msg.topic, e.what() ) );
	}
	catch( ... )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Failed to send message for topic [{}]: unknown error", m_options.name(), msg.topic ) );
	}
}

void KafkaStreamProducer::flush( const std::chrono::milliseconds timeout )
{
	Log( Module::KAFKA ).debug( "KafkaStreamProducer[{}]: Flushing producer with timeout of {} ms", m_options.name(), timeout.count() );

	if( const kafka::Error err = m_kafkaProducer->flush( timeout ) )
		Log( Module::KAFKA ).error( "KafkaStreamProducer[{}]: Error when flushing: {}", m_options.name(), err.toString() );
}

void KafkaStreamProducer::initTransactions( std::chrono::milliseconds timeout )
{
	Log( Module::KAFKA ).debug( "KafkaStreamProducer[{}]: Calling initTransaction with timeout of {} ms", m_options.name(), timeout.count() );

	try
	{
		m_kafkaProducer->initTransactions( timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Error when initializing transactions: {}", m_options.name(), e.what() ) );
	}
}

void KafkaStreamProducer::beginTransaction()
{
	Log( Module::KAFKA ).trace( "KafkaStreamProducer[{}]: Calling beginTransaction", m_options.name() );

	try
	{
		m_kafkaProducer->beginTransaction();
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Error when beginning transaction: {}", m_options.name(), e.what() ) );
	}
}

void KafkaStreamProducer::commitTransaction( std::chrono::milliseconds timeout )
{
	Log( Module::KAFKA ).trace( "KafkaStreamProducer[{}]: Calling commitTransaction with timeout of {} ms", m_options.name(), timeout.count() );

	try
	{
		m_kafkaProducer->commitTransaction();
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Error when committing transaction: {}", m_options.name(), e.what() ) );
	}
}

void KafkaStreamProducer::abortTransaction( std::chrono::milliseconds timeout )
{
	Log( Module::KAFKA ).trace( "KafkaStreamProducer[{}]: Calling abortTransaction with timeout of {} ms", m_options.name(), timeout.count() );

	try
	{
		m_kafkaProducer->abortTransaction();
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Error when aborting transaction: {}", m_options.name(), e.what() ) );
	}
}

void KafkaStreamProducer::sendOffsetsToTransaction( const std::map<StreamTopicPartition, int64_t>& offsets, const StreamGroupMetadata& groupMetadata, std::chrono::milliseconds timeout )
{
	try
	{
		kafka::TopicPartitionOffsets list;
		for( const auto& [tp, offset] : offsets )
			list.emplace( std::make_pair( tp.first, tp.second ), offset );

		// Grab the concrete Kafka consumer group metadata
		const auto* const meta = std::any_cast<std::shared_ptr<kafka::clients::consumer::ConsumerGroupMetadata>>( &groupMetadata.impl );
		if( !meta )
			throw ARQException( std::format( "KafkaStreamProducer[{}]: Invalid StreamGroupMetadata implementation passed to sendOffsetsToTransaction", m_options.name() ) );
		
		m_kafkaProducer->sendOffsetsToTransaction( list, *meta->get(), timeout);
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Error when sending offsets to transaction: {}", m_options.name(), e.what() ) );
	}
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
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Failed to create kafka producer for dsh [{}]: {}", m_options.name(), m_dsh, e.what() ) );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Failed to create kafka producer for dsh [{}]: {}", m_options.name(), m_dsh, e.what() ) );
	}
	catch( ... )
	{
		throw ARQException( std::format( "KafkaStreamProducer[{}]: Failed to create kafka producer for dsh [{}]: unknown error", m_options.name(), m_dsh ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamProducer[{}]: Created kafka producer for dsh [{}]", m_options.name(), m_dsh );
}

kafka::Properties KafkaStreamProducer::buildProperties()
{
	kafka::Properties props;

	setCommonConfig( props, m_dsh, ClientType::PRODUCER, m_options.name() );

	// Ensure exactly-once delivery
	props.put( "enable.idempotence", "true" );

	// Apply preset configurations
	switch( m_options.preset() )
	{
		case StreamProducerOptions::Preset::LowLatency:
			props.put( "linger.ms", "0" );
			props.put( "batch.size", "16384" ); // 16KB
			props.put( "compression.type", "none" );
			props.put( "socket.nagle.disable", "true" );
			break;
		case StreamProducerOptions::Preset::HighThroughput:
			props.put( "linger.ms", "20" );
			props.put( "batch.size", "131072" ); // 128KB
			props.put( "compression.type", "lz4" );
		case StreamProducerOptions::Preset::Standard:
			props.put( "linger.ms", "5" );
			props.put( "batch.size", "16384" ); // 16KB
			props.put( "compression.type", "lz4" );
		default:
			// use defaults
			break;
	}

	// Apply any option overrides
	for( const auto& [option, value] : m_options.optionOverrides() )
		props.put( option, value );

	return props;
}

#pragma endregion

#pragma region Consumer Implementation

/*
*****************************************************
* Implementation of KafkaStreamConsumerMessageBatch *
*****************************************************
*/

ARQKafka_API StreamConsumerMessageView KafkaStreamConsumerMessageBatch::at( const size_t index ) const
{
	const kafka::clients::consumer::ConsumerRecord& record = m_records.at( index );

	std::optional<std::string_view> key;
	if( record.key().data() )
		key = std::string_view( static_cast<const char*>( record.key().data() ), record.key().size() );

	m_topicsCache[index] = record.topic();

	StreamHeaderMapView headersMap;
	if( m_readHeaders == StreamConsumerReadHeaders::READ_HEADERS )
	{
		m_headersCache[index] = record.headers();
		for( const auto& [key, val] : m_headersCache[index] )
			headersMap.emplace( key, std::string_view( static_cast<const char*>( val.data() ), val.size() ) );
	}

	return StreamConsumerMessageView {
		.topic        = m_topicsCache[index],
		.partition    = record.partition(),
		.offset       = record.offset(),
		.key          = std::move( key ),
		.data         = BufferView( record.value().data(), record.value().size() ),
		.timestamp    = Time::DateTime( std::chrono::system_clock::time_point( record.timestamp() ) ),
		.headers      = headersMap,
		.error        = record.error() ? std::make_optional( kafkaErrorToStreamError( record.error() ) ) : std::nullopt
	};
}

/*
*********************************************
*   Implementation of KafkaStreamConsumer   *
*********************************************
*/

// ------------------------ Constructor -------------------------

KafkaStreamConsumer::KafkaStreamConsumer( const std::string_view dsh, const StreamConsumerOptions& options )
	: m_dsh( dsh )
	, m_options( options )
{
	connect();
}

// ------------------------ IStreamConsumer implementation -------------------------

// Standard Consumption

void KafkaStreamConsumer::subscribe( const std::set<std::string>& topics, const StreamConsumerRebalanceCallbackFunc& callback, const std::chrono::milliseconds timeout )
{
	using namespace kafka::clients;

	consumer::RebalanceCallback kafkaCallback = consumer::NullRebalanceCallback;
	if( callback )
	{
		kafkaCallback = [callback] ( consumer::RebalanceEventType eventType, const kafka::TopicPartitions& topicPartitions )
		{
			StreamRebalanceEventType srEventType;
			switch( eventType )
			{
				case consumer::RebalanceEventType::PartitionsAssigned: srEventType = StreamRebalanceEventType::PARTITIONS_ASSIGNED; break;
				case consumer::RebalanceEventType::PartitionsRevoked:  srEventType = StreamRebalanceEventType::PARTITIONS_REVOKED;  break;
				default:
					ARQ_ASSERT( false );
			}

			std::set<StreamTopicPartition> srTopicPartitions;
			for( const auto& [topic, partition] : topicPartitions )
				srTopicPartitions.emplace( topic, partition );

			callback( srEventType, srTopicPartitions );
		};
	}

	try
	{
		m_kafkaConsumer->subscribe( topics, kafkaCallback, timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to subscribe to topics [{}]: {}", m_options.name(), Str::join( topics ), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Subscribed to topics [{}]", m_options.name(), Str::join( topics ) );
}

void KafkaStreamConsumer::unsubscribe( const std::chrono::milliseconds timeout )
{
	try
	{
		m_kafkaConsumer->unsubscribe( timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to unsubscribe: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Unsubscribed", m_options.name() );
}

std::unique_ptr<IStreamConsumerMessageBatch> KafkaStreamConsumer::poll( std::chrono::milliseconds timeout, const StreamConsumerReadHeaders readHeaders )
{
	try
	{
		return std::make_unique<KafkaStreamConsumerMessageBatch>( std::move( m_kafkaConsumer->poll( timeout ) ), readHeaders );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to poll messages: {}", m_options.name(), e.what() ) );
	}
}

// Offset Management

void KafkaStreamConsumer::commitOffsets()
{
	try
	{
		return m_kafkaConsumer->commitSync();
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to commit offsets: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).trace( "KafkaStreamConsumer[{}]: Committed offsets", m_options.name() );
}

void KafkaStreamConsumer::commitOffsetsAsync( const StreamConsumerOffsetCommitCallbackFunc& callback )
{
	try
	{
		return m_kafkaConsumer->commitAsync( [callback] ( const kafka::TopicPartitionOffsets& topicPartitionOffsets, const kafka::Error& err ) {
			std::optional<StreamError> streamingError;
			if( err )
				streamingError = kafkaErrorToStreamError( err );

			callback( topicPartitionOffsets, streamingError );
		} );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to commit offsets asynchronously: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).trace( "KafkaStreamConsumer[{}]: Committed offsets asynchronously", m_options.name() );
}

void KafkaStreamConsumer::commitOffset( const StreamConsumerMessageView& msg )
{
	try
	{
		StreamTopicPartitionOffsets tpos = {
			{   StreamTopicPartition( msg.topic, msg.partition ), msg.offset + 1  } // committed offset should be "current-received-offset + 1"
		};

		return m_kafkaConsumer->commitSync( tpos );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to commit offset for specific message on topic {}: {}", m_options.name(), msg.topic, e.what() ) );
	}

	Log( Module::KAFKA ).trace( "KafkaStreamConsumer[{}]: Committed offsets for specific message on topic {}", m_options.name(), msg.topic );
}

void KafkaStreamConsumer::commitOffsetAsync( const StreamConsumerMessageView& msg, const StreamConsumerOffsetCommitCallbackFunc& callback )
{
	try
	{
		StreamTopicPartitionOffsets tpos = {
			{   StreamTopicPartition( msg.topic, msg.partition ), msg.offset + 1  } // committed offset should be "current-received-offset + 1"
		};

		return m_kafkaConsumer->commitAsync( tpos, [callback] ( const kafka::TopicPartitionOffsets& topicPartitionOffsets, const kafka::Error& err )
		{
			std::optional<StreamError> streamingError;
			if( err )
				streamingError = kafkaErrorToStreamError( err );

			callback( topicPartitionOffsets, streamingError );
		} );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to commit offsets asynchronously for specific message on topic {}: {}", m_options.name(), msg.topic, e.what() ) );
	}

	Log( Module::KAFKA ).trace( "KafkaStreamConsumer[{}]: Committed offsets asynchronously for specific message on topic {}", m_options.name(), msg.topic );
}

void KafkaStreamConsumer::commitOffsets( const StreamTopicPartitionOffsets& topicPartitionOffsets )
{
	try
	{
		return m_kafkaConsumer->commitSync( topicPartitionOffsets );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to commit offsets on specific topic partitions: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).trace( "KafkaStreamConsumer[{}]: Committed offsets on specific topic partitions", m_options.name() );
}

void KafkaStreamConsumer::commitOffsetsAsync( const StreamTopicPartitionOffsets& topicPartitionOffsets, const StreamConsumerOffsetCommitCallbackFunc& callback )
{
	try
	{
		return m_kafkaConsumer->commitAsync( topicPartitionOffsets, [callback] ( const kafka::TopicPartitionOffsets& topicPartitionOffsets, const kafka::Error& err )
		{
			std::optional<StreamError> streamingError;
			if( err )
				streamingError = kafkaErrorToStreamError( err );

			callback( topicPartitionOffsets, streamingError );
		} );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to commit offsets asynchronously: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).trace( "KafkaStreamConsumer[{}]: Committed offsets asynchronously on specific topic partitions", m_options.name() );
}

// Flow Control

void KafkaStreamConsumer::resume()
{
	try
	{
		m_kafkaConsumer->resume();
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to resume consumption: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Resumed consumption", m_options.name() );
}

void KafkaStreamConsumer::resume( const std::set<StreamTopicPartition>& partitions )
{
	try
	{
		m_kafkaConsumer->resume( partitions );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to resume consumption on specified topic partitions: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Resumed consumption on specified topic partitions", m_options.name() );
}

void KafkaStreamConsumer::pause()
{
	try
	{
		m_kafkaConsumer->pause();
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to pause consumption: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Paused consumption", m_options.name() );
}

void KafkaStreamConsumer::pause( const std::set<StreamTopicPartition>& partitions )
{
	try
	{
		m_kafkaConsumer->pause( partitions );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to pause consumption on specified topic partitions: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Paused consumption on specified topic partitions", m_options.name() );
}

// Manual Assignment & Seeking (State Restoration)

void KafkaStreamConsumer::assign( const std::set<StreamTopicPartition>& partitions )
{
	try
	{
		m_kafkaConsumer->assign( partitions );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to assign partitions: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Assigned partitions", m_options.name() );
}

std::set<StreamTopicPartition> KafkaStreamConsumer::getAssignment()
{
	try
	{
		return m_kafkaConsumer->assignment();
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to get partition assignment: {}", m_options.name(), e.what() ) );
	}
}

void KafkaStreamConsumer::seek( const StreamTopicPartition& partition, int64_t offset, std::chrono::milliseconds timeout )
{
	try
	{
		m_kafkaConsumer->seek( partition, offset, timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to seek to offset {} on partition {}: {}", m_options.name(), offset, partition, e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Seeked to offset {} on partition {}", m_options.name(), offset, partition );
}

void KafkaStreamConsumer::seekToBeginning( std::chrono::milliseconds timeout )
{
	try
	{
		m_kafkaConsumer->seekToBeginning( timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to seek to beginning on current assigned partitions: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Seeked to beginning on current assigned partitions", m_options.name() );
}

void KafkaStreamConsumer::seekToBeginning( const std::set<StreamTopicPartition>& partitions, std::chrono::milliseconds timeout )
{
	try
	{
		m_kafkaConsumer->seekToBeginning( partitions, timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to seek to beginning of specified partitions: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Seeked to beginning on specified partitions", m_options.name() );
}

void KafkaStreamConsumer::seekToEnd( std::chrono::milliseconds timeout )
{
	try
	{
		m_kafkaConsumer->seekToEnd( timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to seek to end on current assigned partitions: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Seeked to end on current assigned partitions", m_options.name() );
}

void KafkaStreamConsumer::seekToEnd( const std::set<StreamTopicPartition>& partitions, std::chrono::milliseconds timeout )
{
	try
	{
		m_kafkaConsumer->seekToEnd( partitions, timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to seek to end of specified partitions: {}", m_options.name(), e.what() ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Seeked to end on specified partitions", m_options.name() );
}

int64_t KafkaStreamConsumer::position( const StreamTopicPartition& partition )
{
	try
	{
		return m_kafkaConsumer->position( partition );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to get position of topic partition {}: {}", m_options.name(), partition, e.what() ) );
	}
}

std::map<StreamTopicPartition, int64_t> KafkaStreamConsumer::beginningOffsets( const std::set<StreamTopicPartition>& partitions, const std::chrono::milliseconds timeout )
{
	try
	{
		return m_kafkaConsumer->beginningOffsets( partitions, timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to get beginning offsets for specified partitions: {}", m_options.name(), e.what() ) );
	}
}

std::map<StreamTopicPartition, int64_t> KafkaStreamConsumer::endOffsets( const std::set<StreamTopicPartition>& partitions, const std::chrono::milliseconds timeout )
{
	try
	{
		return m_kafkaConsumer->endOffsets( partitions, timeout );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to get end offsets for specified partitions: {}", m_options.name(), e.what() ) );
	}
}

// Group Metadata

StreamGroupMetadata KafkaStreamConsumer::getGroupMetadata() const
{
	// Have to wrap in shared_ptr because ConsumerGroupMetadata is not copyable
	return StreamGroupMetadata{ .impl = std::make_shared<kafka::clients::consumer::ConsumerGroupMetadata>( m_kafkaConsumer->groupMetadata() ) };
}

// ------------------------ Connection -------------------------

void KafkaStreamConsumer::connect()
{
	try
	{
		kafka::Properties props = buildProperties();
		m_kafkaConsumer = std::make_unique<kafka::clients::consumer::KafkaConsumer>( props );
	}
	catch( const kafka::KafkaException& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to create kafka consumer for dsh [{}]: {}", m_options.name(), m_dsh, e.what() ) );
	}
	catch( const std::exception& e )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to create kafka consumer for dsh [{}]: {}", m_options.name(), m_dsh, e.what() ) );
	}
	catch( ... )
	{
		throw ARQException( std::format( "KafkaStreamConsumer[{}]: Failed to create kafka consumer for dsh [{}]: unknown error", m_options.name(), m_dsh ) );
	}

	Log( Module::KAFKA ).info( "KafkaStreamConsumer[{}]: Created kafka consumer for dsh [{}]", m_options.name(), m_dsh );
}

kafka::Properties KafkaStreamConsumer::buildProperties()
{
	kafka::Properties props;

	setCommonConfig( props, m_dsh, ClientType::CONSUMER, m_options.name() );

	// Set groupID
	if( m_options.groupID().empty() )
		throw ARQException( "Consumer Group ID cannot be empty" );
	props.put( "group.id", m_options.groupID() );

	// Disable EOF error spam (e.g., "Reached end of partition") - usually not useful
	props.put( "enable.partition.eof", "false" );

	// Offset reset strategy - what to do when there is no committed offset
	std::string offResStr;
	switch( m_options.autoOffsetReset() )
	{
		case StreamConsumerOptions::AutoOffsetReset::Latest:   offResStr = "latest";   break;
		case StreamConsumerOptions::AutoOffsetReset::Earliest: offResStr = "earliest"; break;
		case StreamConsumerOptions::AutoOffsetReset::None:     offResStr = "error";    break;
		default:
			ARQ_ASSERT( false ); // Unknown option
			offResStr = "latest";
			break;
	}
	props.put( "auto.offset.reset", offResStr );

	// Commit Strategy
	props.put( "enable.auto.commit", m_options.autoCommitOffsets() == StreamConsumerOptions::AutoCommitOffsets::Enabled ? "true" : "false" );

	// Fetch Preset - tuning for latency vs throughput
	switch( m_options.fetchPreset() )
	{
		case StreamConsumerOptions::FetchPreset::Standard:
			props.put( "fetch.wait.max.ms", "20" ); // Get broker to wait up to 20ms for batch fill
			props.put( "fetch.min.bytes", "1" );    // Don't care about batch size
			props.put( "socket.nagle.disable", "true" );
			break;
		case StreamConsumerOptions::FetchPreset::LowLatency:
			props.put( "fetch.wait.max.ms", "1" ); // Get broker to not wait at all for batch fill
			props.put( "fetch.min.bytes", "1" );   // Don't care about batch size
			props.put( "socket.nagle.disable", "true" );
			break;
		case StreamConsumerOptions::FetchPreset::HighThroughput:
			props.put( "fetch.wait.max.ms", "500" );  // Get broker to wait up to 500ms for batch fill
			props.put( "fetch.min.bytes", "102400" ); // Don't send unless batch size > ~100KB (or timeout)
			break;
		default:
			ARQ_ASSERT( false ); // Unknown option
			break;
	}

	// Apply any option overrides
	for( const auto& [option, value] : m_options.optionOverrides() )
		props.put( option, value );

	return props;
}

#pragma endregion

}