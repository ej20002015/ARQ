#include "service.h"

#include <ARQUtils/algos.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/id.h>
#include <ARQUtils/logger.h>
#include <ARQUtils/str.h>
#include <ARQMarket/mktdata_meta.h>
#include <ARQMarket/mktdata_topics.h>

#include <optional>
#include <ranges>

void MktDataCurrentProcessorService::onStartup()
{
	m_serialiser = SerialiserFactory::inst().create( SerialiserFactory::SerialiserImpl::Protobuf );

	StreamProducerOptions producerOptions( "MktDataCurrentProcessor::CurrentProducer", StreamProducerOptions::Preset::HighThroughput );
	producerOptions.setOptionOverride( "transactional.id", ID::UUID::create().toString() );
	m_currentProducer = StreamingServiceFactory::inst().createProducer( m_config.streamSvcDSH, producerOptions );
	m_currentProducer->initTransactions();

	StreamConsumerOptions consumerOptions( "MktDataCurrentProcessor::ObservationConsumer",
		                                     "ARQ.MktData.CurrentProcessors",
		                                     StreamConsumerOptions::FetchPreset::HighThroughput,
		                                     StreamConsumerOptions::AutoCommitOffsets::Disabled,
		                                     StreamConsumerOptions::AutoOffsetReset::Earliest,
		                                     StreamConsumerOptions::IsolationLevel::ReadCommitted );
	m_observationConsumer = StreamingServiceFactory::inst().createConsumer( m_config.streamSvcDSH, consumerOptions );

	const std::set<std::string_view> entities = Algos::makeEffectiveSet( m_config.entities, MD::Meta::getAllNames(), m_config.disabledEntities );
	const auto observationTopics = entities
		| std::views::transform( [] ( const std::string_view name ) { return MD::getObservationTopic( name ); } )
		| std::ranges::to<std::set<std::string>>();
	m_observationConsumer->subscribe( observationTopics, [this] ( const StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& partitions ) {
		onRebalance( eventType, partitions );
	} );
}

void MktDataCurrentProcessorService::onShutdown()
{
	m_observationConsumer.reset();
	m_currentProducer.reset();
	m_serialiser.reset();
	m_selectors.clear();
}

void MktDataCurrentProcessorService::run()
{
	while( shouldRun() )
	{
		const auto msgBatch = m_observationConsumer->poll( 100ms, StreamConsumerReadHeaders::SKIP_HEADERS );
		if( msgBatch->empty() )
			continue;

		processObservationBatch( *msgBatch );
	}
}

void MktDataCurrentProcessorService::registerConfigOptions( Cfg::ConfigWrangler& cfg )
{
	cfg.add( m_config.streamSvcDSH,     "--streamServiceDSH", "The DSH of the streaming service used for observations and current-state output" );
	cfg.add( m_config.entities,         "--entities",         "The set of market-data entities to process. If empty, process all entities." );
	cfg.add( m_config.disabledEntities, "--disabledEntities", "The set of market-data entities not to process." );
}

void MktDataCurrentProcessorService::processObservationBatch( const IStreamConsumerMessageBatch& msgBatch )
{
	Log( Module::EXE ).debug( "Processing {} market-data observations", msgBatch.size() );

	try
	{
		StreamTopicPartitionOffsets offsetsToCommit;
		m_currentProducer->beginTransaction();

		for( const StreamConsumerMessageView& msg : msgBatch )
		{
			processObservationMessage( msg );
			offsetsToCommit[StreamTopicPartition{ std::string( msg.topic ), msg.partition }] = msg.offset + 1;
		}

		m_currentProducer->sendOffsetsToTransaction( offsetsToCommit, m_observationConsumer->getGroupMetadata() );
		m_currentProducer->commitTransaction();
	}
	catch( const ARQException& e )
	{
		Log( Module::EXE ).critical( e, "Exception thrown when processing a batch of observations; aborting the Kafka transaction and stopping the service" );
		m_currentProducer->abortTransaction();
		throw;
	}
	catch( const std::exception& e )
	{
		Log( Module::EXE ).critical( "std::exception thrown when processing a batch of observations; aborting the Kafka transaction and stopping the service. What: {}", e.what() );
		m_currentProducer->abortTransaction();
		throw;
	}
	catch( ... )
	{
		Log( Module::EXE ).critical( "Unknown exception thrown when processing a batch of observations; aborting the Kafka transaction and stopping the service" );
		m_currentProducer->abortTransaction();
		throw;
	}
}

void MktDataCurrentProcessorService::processObservationMessage( const StreamConsumerMessageView& msg )
{
	if( !msg.key.has_value() )
	{
		Log( Module::EXE ).error( "Market-data observation {} has no routing key; sending it to the DLQ", msg.idStr() );
		sendToDLQ( msg );
		return;
	}

	std::optional<MD::Type> entityType;
	try
	{
		entityType = MD::getTypeFromObservationTopic( msg.topic );
	}
	catch( const ARQException& e )
	{
		Log( Module::EXE ).error( e, "Unable to determine the type of market-data observation {}; sending it to the DLQ", msg.idStr() );
		sendToDLQ( msg );
		return;
	}

	MD::dispatch( *entityType, [this, &msg, entityType] <MD::c_MktData T> ()
	{
		std::optional<MD::RecordMessage<T>> recordMessage;
		try
		{
			recordMessage.emplace( m_serialiser->deserialise<MD::RecordMessage<T>>( msg.data ) );
		}
		catch( const ARQException& e )
		{
			Log( Module::EXE ).error( e, "Unable to deserialise market-data observation {}; sending it to the DLQ", msg.idStr() );
			sendToDLQ( msg );
			return;
		}

		auto& selector = m_selectors[recordMessage->mktName];
		if( !selector.push( *entityType, recordMessage->record.header.id, recordMessage->record.header.asofTs ) )
			return;

		m_currentProducer->send( StreamProducerMessage{
			.topic     = std::string( MD::getCurrentTopic( *entityType ) ),
			.key       = std::string( *msg.key ),
			.partition = msg.partition,
			.data      = SharedBuffer( msg.data.data, msg.data.size )
		} );
	} );
}

void MktDataCurrentProcessorService::sendToDLQ( const StreamConsumerMessageView& msg )
{
	m_currentProducer->send( StreamProducerMessage{
		.topic = std::format( "{}.DLQ", msg.topic ),
		.id    = msg.offset,
		.key   = msg.key.has_value() ? std::string( *msg.key ) : "NO_KEY",
		.data  = SharedBuffer( msg.data.data, msg.data.size )
	} );
}

void MktDataCurrentProcessorService::onRebalance( const StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& observationPartitions )
{
	setReady( false );

	Log( Module::EXE ).info( "Observation rebalance event {} for [{}]", Enum::enum_name( eventType ), Str::join( observationPartitions ) );

	m_selectors.clear();

	if( eventType == StreamRebalanceEventType::PARTITIONS_ASSIGNED && !observationPartitions.empty() )
		hydrateState( observationPartitions );

	setReady( true );
}

void MktDataCurrentProcessorService::hydrateState( const std::set<StreamTopicPartition>& observationPartitions )
{
	StreamConsumerOptions consumerOptions( "MktDataCurrentProcessor::CurrentHydrationConsumer",
		                                     "ARQ.MktData.CurrentProcessors.CurrentHydration",
		                                     StreamConsumerOptions::FetchPreset::HighThroughput,
		                                     StreamConsumerOptions::AutoCommitOffsets::Disabled,
		                                     StreamConsumerOptions::AutoOffsetReset::Earliest,
		                                     StreamConsumerOptions::IsolationLevel::ReadCommitted );
	const std::shared_ptr<IStreamConsumer> currentConsumer = StreamingServiceFactory::inst().createConsumer( m_config.streamSvcDSH, consumerOptions );

	const std::set<StreamTopicPartition> currentPartitions = mapToCurrentPartitions( observationPartitions );
	StreamTopicPartitionOffsets highWatermarks = getHydrationTargets( *currentConsumer, currentPartitions );
	if( highWatermarks.empty() )
		return;

	currentConsumer->assign( highWatermarks | std::views::keys | std::ranges::to<std::set>() );
	currentConsumer->seekToBeginning();

	Log( Module::EXE ).info( "Hydrating current market selection state from {} partitions", highWatermarks.size() );

	while( !highWatermarks.empty() && shouldRun() )
	{
		const auto msgBatch = currentConsumer->poll( 50ms, StreamConsumerReadHeaders::SKIP_HEADERS );
		for( const StreamConsumerMessageView& msg : *msgBatch )
			processHydrationMessage( msg );

		updateHydrationProgress( *currentConsumer, highWatermarks );
	}

	if( !highWatermarks.empty() )
		throw ARQException( "Current market state hydration stopped before reaching its captured high watermarks" );

	Log( Module::EXE ).info( "Finished current market state hydration for {} markets", m_selectors.size() );
}

std::set<StreamTopicPartition> MktDataCurrentProcessorService::mapToCurrentPartitions( const std::set<StreamTopicPartition>& observationPartitions ) const
{
	return observationPartitions
		| std::views::transform( [] ( const StreamTopicPartition& observationPartition )
		  {
			  const MD::Type type = MD::getTypeFromObservationTopic( observationPartition.first );
			  return StreamTopicPartition{ std::string( MD::getCurrentTopic( type ) ), observationPartition.second };
		  } )
		| std::ranges::to<std::set>();
}

StreamTopicPartitionOffsets MktDataCurrentProcessorService::getHydrationTargets( IStreamConsumer& consumer, const std::set<StreamTopicPartition>& currentPartitions ) const
{
	StreamTopicPartitionOffsets targets;

	const StreamTopicPartitionOffsets beginningOffsets = consumer.beginningOffsets( currentPartitions );
	const StreamTopicPartitionOffsets endOffsets       = consumer.endOffsets( currentPartitions );

	for( const StreamTopicPartition& tp : currentPartitions )
	{
		const int64_t beg = beginningOffsets.at( tp );
		const int64_t end = endOffsets.at( tp );

		if( end > beg )
		{
			targets[tp] = end - 1; // We need to read up to (end - 1)
			Log( Module::EXE ).debug( "Partition {} needs hydration (up to offset {})", tp, end - 1 );
		}
	}

	return targets;
}

void MktDataCurrentProcessorService::processHydrationMessage( const StreamConsumerMessageView& msg )
{
	const MD::Type entityType = MD::getTypeFromCurrentTopic( msg.topic );
	MD::dispatch( entityType, [this, &msg, entityType] <MD::c_MktData T> ()
	{
		const MD::RecordMessage<T> recordMessage = m_serialiser->deserialise<MD::RecordMessage<T>>( msg.data );
		auto& selector = m_selectors[recordMessage.mktName];
		selector.push( entityType, recordMessage.record.header.id, recordMessage.record.header.asofTs );
	} );
}

void MktDataCurrentProcessorService::updateHydrationProgress( IStreamConsumer& consumer, StreamTopicPartitionOffsets& highWatermarks ) const
{
	for( auto it = highWatermarks.begin(); it != highWatermarks.end(); )
	{
		const StreamTopicPartition& tp = it->first;
		const int64_t               targetHW = it->second;

		if( consumer.position( tp ) > targetHW )
		{
			Log( Module::EXE ).debug( "Hydration complete for {}", tp );
			it = highWatermarks.erase( it );
		}
		else
			++it;
	}
}
