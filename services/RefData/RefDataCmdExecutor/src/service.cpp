#include "service.h"

#include <ARQUtils/types.h>
#include <ARQUtils/algos.h>
#include <ARQCore/refdata_meta.h>

#include <algorithm>
#include <ranges>

void RefDataCmdExecutorService::onStartup()
{
	m_serialiser = SerialiserFactory::inst().create( SerialiserFactory::SerialiserImpl::Protobuf );
	m_msgSvc     = MessagingServiceFactory::inst().create( m_config.msgSvcDSH );

	StreamConsumerOptions opts( "RefDataCmdExecutor::CommandConsumer",
								"ARQ.RefData.CommandExecutors",
								StreamConsumerOptions::FetchPreset::Standard,
								StreamConsumerOptions::AutoCommitOffsets::Disabled,
								StreamConsumerOptions::AutoOffsetReset::Earliest );
	m_commandConsumer = StreamingServiceFactory::inst().createConsumer( m_config.streamSvcDSH, opts );

	const auto commandTopics = getEntities()
		| std::views::transform( [] ( const std::string_view entity ) { return std::string( RD::getCommandTopic( entity ) ); } )
		| std::ranges::to<std::set>();
	m_commandConsumer->subscribe( commandTopics, [this] ( StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& topicPartitions ) {
		onRebalance( eventType, topicPartitions );
	} );

	StreamProducerOptions prodOpts( "RefDataCmdExecutor::UpdateProducer",
									StreamProducerOptions::Preset::HighThroughput );
	prodOpts.setOptionOverride( "transactional.id", ID::UUID::create().toString() );
	m_updateProducer = StreamingServiceFactory::inst().createProducer( m_config.streamSvcDSH, prodOpts );
	m_updateProducer->initTransactions();
}

void RefDataCmdExecutorService::onShutdown()
{
	m_commandConsumer.reset();
	m_updateProducer.reset();
	m_msgSvc.reset();
	m_serialiser.reset();
}

void RefDataCmdExecutorService::run()
{
	while( shouldRun() )
	{
		const auto msgBatch = m_commandConsumer->poll( 100ms );
		if( msgBatch->empty() )
			continue;

		processCommandBatch( *msgBatch );
	}
}

void RefDataCmdExecutorService::processCommandBatch( const IStreamConsumerMessageBatch& msgBatch )
{
	Log( Module::EXE ).debug( "Processing {} reference-data commands", msgBatch.size() );

	BatchOutput batchOutput;
	batchOutput.recordUpdates.reserve( msgBatch.size() );
	batchOutput.responses.reserve( msgBatch.size() );

	try
	{
		StreamTopicPartitionOffsets offsetsToCommit;
		m_updateProducer->beginTransaction();

		for( const StreamConsumerMessageView& msg : msgBatch )
		{
			processCommandMessage( msg, batchOutput );
			offsetsToCommit[StreamTopicPartition{ std::string( msg.topic ), msg.partition }] = msg.offset + 1;
		}

		m_updateProducer->sendOffsetsToTransaction( offsetsToCommit, m_commandConsumer->getGroupMetadata() );
		m_updateProducer->commitTransaction();
	}
	catch( const ARQException& e )
	{
		Log( Module::EXE ).critical( e, "Exception thrown when processing a batch of reference-data commands; aborting the Kafka transaction and stopping the service" );
		m_updateProducer->abortTransaction();
		throw;
	}
	catch( const std::exception& e )
	{
		Log( Module::EXE ).critical( "std::exception thrown when processing a batch of reference-data commands; aborting the Kafka transaction and stopping the service. What: {}", e.what() );
		m_updateProducer->abortTransaction();
		throw;
	}
	catch( ... )
	{
		Log( Module::EXE ).critical( "Unknown exception thrown when processing a batch of reference-data commands; aborting the Kafka transaction and stopping the service" );
		m_updateProducer->abortTransaction();
		throw;
	}

	for( auto& [uuid, storedRecord] : batchOutput.recordUpdates )
		m_records.insert_or_assign( uuid, std::move( storedRecord ) );

	for( const auto& [response, topic] : batchOutput.responses )
		sendCommandResponse( response, topic );
}

void RefDataCmdExecutorService::processCommandMessage( const StreamConsumerMessageView& msg, BatchOutput& batchOutput )
{
	Log( Module::EXE ).trace( "Processing command message: Topic={}, Partition={}, Offset={}, Key={}, Timestamp={}",
		msg.topic,
		msg.partition,
		msg.offset,
		msg.key.value_or( "N/A" ),
		msg.timestamp.fmtISO8601()
	);

	if( !msg.key || msg.key->empty() )
	{
		Log( Module::EXE ).error( "Reference-data command {} has no routing key; sending it to the DLQ", msg.idStr() );
		sendToDLQ( msg );
		return;
	}

	std::string_view entityName;
	std::string_view commandAction;
	std::string_view responseTopic;
	ID::UUID         corrID;
	try
	{
		entityName    = RD::getEntityNameFromCommandTopic( msg.topic );
		commandAction = msg.tryGetHeaderValue( "ARQ_CmdAction" );
		responseTopic = msg.tryGetHeaderValue( "ARQ_ResponseTopic" );
		corrID        = ID::uuidFromStr( msg.tryGetHeaderValue( "ARQ_CorrID" ) );
		if( responseTopic.empty() )
			throw ARQException( "Reference-data command response topic is empty" );
	}
	catch( const ARQException& e )
	{
		Log( Module::EXE ).error( e, "Invalid reference-data command envelope {}; sending it to the DLQ", msg.idStr() );
		sendToDLQ( msg );
		return;
	}

	if( commandAction != "Upsert" && commandAction != "Deactivate" )
	{
		Log( Module::EXE ).error( "Reference-data command {} has unknown action [{}]; sending it to the DLQ", msg.idStr(), commandAction );
		sendToDLQ( msg );
		return;
	}

	RD::dispatch( entityName, [this, &msg, &corrID, &responseTopic, &commandAction, &batchOutput] <RD::c_RefData T> ()
	{
		if( commandAction == "Upsert" )
			processUpsertCmdMessage<T>( msg, corrID, responseTopic, batchOutput );
		else
			processDeactivateCmdMessage<T>( msg, corrID, responseTopic, batchOutput );
	} );
}

void RefDataCmdExecutorService::sendToDLQ( const StreamConsumerMessageView& msg )
{
	m_updateProducer->send( StreamProducerMessage{
		.topic = std::format( "{}.DLQ", msg.topic ),
		.id    = msg.offset,
		.key   = msg.key ? std::string( *msg.key ) : "NO_KEY",
		.data  = SharedBuffer( msg.data.data, msg.data.size )
	} );
}

void RefDataCmdExecutorService::registerConfigOptions( Cfg::ConfigWrangler& cfg )
{
	cfg.add( m_config.streamSvcDSH,     "--streamServiceDSH", "The DSH of the streaming service to use" );
	cfg.add( m_config.msgSvcDSH,        "--msgSvcDSH",        "The DSH of the messaging service to use" );
	cfg.add( m_config.entities,         "--entities",         "The set of reference data entities to process commands for. If empty, subscribes to all entities." );
	cfg.add( m_config.disabledEntities, "--disabledEntities", "The set of reference data entities to NOT process commands for." );
}

template<RD::Cmd::c_Command T>
std::optional<T> RefDataCmdExecutorService::tryReadCommand( const StreamConsumerMessageView& msg )
{
	std::optional<T> command;
	try
	{
		command.emplace( m_serialiser->deserialise<T>( msg.data ) );
	}
	catch( const ARQException& e )
	{
		Log( Module::EXE ).error( e, "Unable to deserialise reference-data command {}; sending it to the DLQ", msg.idStr() );
		sendToDLQ( msg );
		return std::nullopt;
	}

	return command;
}

template<RD::c_RefData T>
void RefDataCmdExecutorService::processUpsertCmdMessage( const StreamConsumerMessageView& msg, const ID::UUID& corrID,
															std::string_view responseTopic, BatchOutput& batchOutput )
{
	const std::optional<RD::Cmd::Upsert<T>> command = tryReadCommand<RD::Cmd::Upsert<T>>( msg );
	if( !command )
		return;

	const std::optional<std::uint32_t> currentVersion = getCurVer( command->targetUUID, batchOutput );
	const RD::CommandDecision<T> decision = RD::RefDataCommandProcessor::process( *command, currentVersion, Time::DateTime::nowUTC() );
	stageCommandDecision( msg, corrID, responseTopic, decision, batchOutput );
}

template<RD::c_RefData T>
void RefDataCmdExecutorService::processDeactivateCmdMessage( const StreamConsumerMessageView& msg, const ID::UUID& corrID,
																std::string_view responseTopic, BatchOutput& batchOutput )
{
	const std::optional<RD::Cmd::Deactivate<T>> command = tryReadCommand<RD::Cmd::Deactivate<T>>( msg );
	if( !command )
		return;

	const std::optional<RD::Record<T>> currentRecord = getCurrentRecord<T>( command->targetUUID, batchOutput );
	const RD::CommandDecision<T> decision = RD::RefDataCommandProcessor::process( *command, currentRecord, Time::DateTime::nowUTC() );
	stageCommandDecision( msg, corrID, responseTopic, decision, batchOutput );
}

template<RD::c_RefData T>
void RefDataCmdExecutorService::stageCommandDecision( const StreamConsumerMessageView& msg, const ID::UUID& corrID,
														 std::string_view responseTopic, const RD::CommandDecision<T>& decision, BatchOutput& batchOutput )
{
	RD::CommandResponse resp;
	resp.corrID = corrID;

	if( const auto* newRecord = std::get_if<RD::Record<T>>( &decision ) )
	{
		SharedBuffer payload = m_serialiser->serialise<RD::Record<T>>( *newRecord );

		m_updateProducer->send( StreamProducerMessage{
			.topic = std::string( RD::Topics<T>::updateTopic() ),
			.id    = msg.offset,
			.key   = newRecord->header.uuid.toString(),
			.data  = payload
		} );

		batchOutput.recordUpdates.insert_or_assign( newRecord->header.uuid, StoredRecord{
			.version          = newRecord->header.version,
			.serialisedRecord = std::move( payload )
		} );
		resp.status = RD::CommandResponse::SUCCESS;
	}
	else
	{
		const RD::CommandRejection& rejection = std::get<RD::CommandRejection>( decision );
		resp.status = RD::CommandResponse::REJECTED;
		resp.message = rejection.message;

		Log( Module::EXE ).warn( "Rejecting reference-data command from message {}: {}", msg.idStr(), *resp.message );
	}

	batchOutput.responses.push_back( std::make_pair( resp, responseTopic ) );
}

template<RD::c_RefData T>
std::optional<RD::Record<T>> RefDataCmdExecutorService::getCurrentRecord( const ID::UUID& targetUUID, const BatchOutput& batchOutput ) const
{
	const StoredRecord* storedRecord = findCurrentStoredRecord( targetUUID, batchOutput );
	if( !storedRecord )
		return std::nullopt;

	RD::Record<T> record = m_serialiser->deserialise<RD::Record<T>>( storedRecord->serialisedRecord );
	// Minor validation
	if( record.header.uuid != targetUUID || record.header.version != storedRecord->version )
		throw ARQException( std::format( "Inconsistent command state for existing {} with UUID {}", RD::Traits<T>::name(), targetUUID ) );

	return record;
}

std::optional<uint32_t> RefDataCmdExecutorService::getCurVer( const ID::UUID& targetUUID, const BatchOutput& batchOutput ) const
{
	const StoredRecord* storedRecord = findCurrentStoredRecord( targetUUID, batchOutput );
	return storedRecord ? std::optional<uint32_t>( storedRecord->version ) : std::nullopt;
}

const RefDataCmdExecutorService::StoredRecord* RefDataCmdExecutorService::findCurrentStoredRecord( const ID::UUID& targetUUID, const BatchOutput& batchOutput ) const
{
	if( const auto it = batchOutput.recordUpdates.find( targetUUID ); it != batchOutput.recordUpdates.end() )
		return &it->second;
	else if( const auto it = m_records.find( targetUUID ); it != m_records.end() )
		return &it->second;
	else
		return nullptr;
}

void RefDataCmdExecutorService::onRebalance( StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& cmdTPs )
{
	setReady( false );
	ARQDefer{ 
		setReady( true ); 
	} );

	Log( Module::EXE ).info( "Rebalance event occurred: {} ON {}", Enum::enum_name( eventType ), Str::join( cmdTPs ) );

	m_records.clear();

	if( eventType == StreamRebalanceEventType::PARTITIONS_REVOKED || cmdTPs.empty() )
		return;

	hydrateState( cmdTPs );
}

void RefDataCmdExecutorService::hydrateState( const std::set<StreamTopicPartition>& cmdTPs )
{
	StreamConsumerOptions opts( "RefDataCmdExecutor::UpdateConsumer",
								"ARQ.RefData.CommandExecutors.UpdateHydration",
								StreamConsumerOptions::FetchPreset::Standard,
								StreamConsumerOptions::AutoCommitOffsets::Disabled,
								StreamConsumerOptions::AutoOffsetReset::Earliest,
								StreamConsumerOptions::IsolationLevel::ReadCommitted );
	std::shared_ptr<IStreamConsumer> updateConsumer = StreamingServiceFactory::inst().createConsumer( "Kafka", opts );

	const std::set<StreamTopicPartition> equivUpdateTPs = mapToUpdatePartitions( cmdTPs );
	      StreamTopicPartitionOffsets    highWatermarks = getHydrationTargets( *updateConsumer, equivUpdateTPs );

	if( highWatermarks.empty() )
		return;

	updateConsumer->assign( highWatermarks | std::views::keys | std::ranges::to<std::set>() );
	updateConsumer->seekToBeginning();

	Log( Module::EXE ).info( "Starting hydration for {} partitions...", highWatermarks.size() );

	while( !highWatermarks.empty() && shouldRun() )
	{
		const auto msgBatch = updateConsumer->poll( 50ms );

		for( const auto& msg : *msgBatch )
			processHydrationMessage( msg );

		updateHydrationProgress( *updateConsumer, highWatermarks );
	}

	Log( Module::EXE ).info( "Finished hydration of update partitions. Loaded {} entities.", m_records.size() );
}

std::set<StreamTopicPartition> RefDataCmdExecutorService::mapToUpdatePartitions( const std::set<StreamTopicPartition>& cmdTPs )
{
	return cmdTPs
		| std::views::transform( [] ( const auto& tp )
		  {
		      const std::string_view entityName = RD::getEntityNameFromCommandTopic( tp.first );
		      return std::make_pair( std::string( RD::getUpdateTopic( entityName ) ), tp.second );
		  } )
		| std::ranges::to<std::set>();
}

StreamTopicPartitionOffsets RefDataCmdExecutorService::getHydrationTargets( IStreamConsumer& consumer, const std::set<StreamTopicPartition>& partitions )
{
	StreamTopicPartitionOffsets targets;

	const auto begOffsets = consumer.beginningOffsets( partitions );
	const auto endOffsets = consumer.endOffsets( partitions );

	for( const auto& tp : partitions )
	{
		const int64_t end = endOffsets.at( tp );
		const int64_t beg = begOffsets.at( tp );

		if( end > beg )
		{
			targets[tp] = end - 1; // We need to read up to (end - 1)
			Log( Module::EXE ).debug( "Partition {} needs hydration (up to offset {})", tp, end - 1 );
		}
	}

	return targets;
}

void RefDataCmdExecutorService::processHydrationMessage( const StreamConsumerMessageView& msg )
{
	ARQ_DO_IN_TRY( arqExc, errMsg );
	{
		const std::string_view entityName = RD::getEntityNameFromUpdateTopic( msg.topic );
		RD::dispatch( entityName, [this, &msg] <RD::c_RefData T> ( )
		{
			const auto record = m_serialiser->deserialise<RD::Record<T>>( msg.data );
			m_records.insert_or_assign( record.header.uuid, StoredRecord{
				.version          = record.header.version,
				.serialisedRecord = Buffer( msg.data.data, msg.data.size )
			} );
		} );
	}
	ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

	if( arqExc.what().size() )
		Log( Module::EXE ).error( arqExc, "Exception thrown when processing hydration message so skipping [{}]", msg.idStr() );
	else if( errMsg.size() )
		Log( Module::EXE ).error( "Exception thrown when processing hydration message so skipping [{}] - what: ", msg.idStr() );
}

void RefDataCmdExecutorService::updateHydrationProgress( IStreamConsumer& consumer, StreamTopicPartitionOffsets& highWatermarks )
{
	auto it = highWatermarks.begin();
	while( it != highWatermarks.end() )
	{
		const StreamTopicPartition& tp = it->first;
		const int64_t               targetHW = it->second;

		int64_t nextOffset = consumer.position( it->first );
		if( nextOffset > targetHW )
		{
			Log( Module::EXE ).debug( "Hydration complete for {}", tp );
			it = highWatermarks.erase( it );
		}
		else
			++it;
	}
}

void RefDataCmdExecutorService::sendCommandResponse( const RD::CommandResponse& cmdRes, std::string_view topic )
{
	Buffer buf = m_serialiser->serialise<RD::CommandResponse>( cmdRes );
	Message msg = {
		.data = std::move( buf )
	};

	m_msgSvc->publish( topic, msg );
}

const std::set<std::string_view>& RefDataCmdExecutorService::getEntities()
{
	static std::set<std::string_view> entities;
	if( entities.empty() )
		entities = Algos::makeEffectiveSet( m_config.entities, RD::Meta::getAllNames(), m_config.disabledEntities );

	return entities;
}
