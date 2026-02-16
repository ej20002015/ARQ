#include "service.h"

#include <ARQUtils/types.h>
#include <ARQUtils/algos.h>
#include <ARQCore/refdata_meta.h>

#include <algorithm>
#include <ranges>

void RefDataCmdExecutorService::onStartup()
{
	buildTopicEntityMaps();

	m_serialiser = SerialiserFactory::create( SerialiserFactory::SerialiserImpl::Protobuf );
	m_msgSvc     = MessagingServiceFactory::create( m_config.msgSvcDSH );

	StreamConsumerOptions opts( "RefDataCmdExecutor::CommandConsumer",
								"ARQ.RefData.CommandExecutors",
								StreamConsumerOptions::FetchPreset::Standard,
								StreamConsumerOptions::AutoCommitOffsets::Disabled,
								StreamConsumerOptions::AutoOffsetReset::Earliest );
	m_commandConsumer = StreamingServiceFactory::createConsumer( m_config.streamSvcDSH, opts );

	const auto commandTopics = getEntities()
		| std::views::transform( [] ( const std::string_view entity ) { return std::format( "ARQ.RefData.Commands.{}", entity ); } )
		| std::ranges::to<std::set>();
	m_commandConsumer->subscribe( commandTopics, [this] ( StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& topicPartitions ) { onRebalance( eventType, topicPartitions ); } );

	StreamProducerOptions prodOpts( "RefDataCmdExecutor::UpdateProducer",
									StreamProducerOptions::Preset::HighThroughput );
	prodOpts.setOptionOverride( "transactional.id", ID::UUID::create().toString() );
	m_updateProducer = StreamingServiceFactory::createProducer( m_config.streamSvcDSH, prodOpts );
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

		Log( Module::EXE ).debug( "Processing {} command messages", msgBatch->size() );

		BatchOutput batchOutput;
		batchOutput.versionMapUpdates.reserve( msgBatch->size() );
		batchOutput.responses.reserve( msgBatch->size() );

		try
		{
			StreamTopicPartitionOffsets offsetsToCommit;

			m_updateProducer->beginTransaction();

			for( const StreamConsumerMessageView& msg : *msgBatch )
			{
				Log( Module::EXE ).trace( "Processing command message: Topic={}, Partition={}, Offset={}, Key={}, Timestamp={}",
					msg.topic,
					msg.partition,
					msg.offset,
					msg.key.value_or( "N/A" ),
					msg.timestamp.fmtISO8601()
				);

				ARQ_DO_IN_TRY( arqExc, errMsg );
				{
					const std::string_view entityName = getEntityFromCmdTopic( msg.topic );
					const std::string_view cmdAction  = msg.tryGetHeaderValue( "ARQ_CmdAction" );

					RD::dispatch( entityName, [this, &msg, &cmdAction, &batchOutput] <RD::c_RefData T> ()
					{
						if( cmdAction == "Upsert" )
							processUpsertCmdMessage<T>( msg, batchOutput );
						else if( cmdAction == "Deactivate" )
							processDeactivateCmdMessage<T>( msg, batchOutput );
						else
							throw ARQException( std::format( "Received refdata command with unknown action [{}]", cmdAction ) );
					} );
				}
				ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

				bool toDLQ = false;
				if( arqExc.what().size() )
				{
					Log( Module::EXE ).error( arqExc, "Exception thrown when processing message so sending to DLQ [{}]", msg.idStr() );
					toDLQ = true;
				}
				else if( errMsg.size() )
				{
					Log( Module::EXE ).error( "Exception thrown when processing message so sending to DLQ [{}] - what: ", msg.idStr() );
					toDLQ = true;
				}

				if( toDLQ )
				{
					const std::string key = msg.key.has_value() ? msg.key->data() : "NO_KEY";
					m_updateProducer->send( StreamProducerMessage{
						.topic   = std::format( "{}.DLQ", msg.topic ),
						.id      = msg.offset,
						.key     = key,
						.data    = msg.data
					} );
				}

				offsetsToCommit[StreamTopicPartition{ msg.topic, msg.partition }] = msg.offset + 1;
			}

			m_updateProducer->sendOffsetsToTransaction( offsetsToCommit, m_commandConsumer->getGroupMetadata() );
			m_updateProducer->commitTransaction();
		}
		catch( const ARQException& e )
		{
			Log( Module::EXE ).critical( e, "Exception thrown when processing batch of messages - aborting stream transaction and exiting early!" );
			m_updateProducer->abortTransaction();
			throw;
		}
		catch( const std::exception& e )
		{
			Log( Module::EXE ).critical( "std::exception thrown when processing batch of messages - aborting stream transaction and exiting early! what: {}", e.what() );
			m_updateProducer->abortTransaction();
			throw;
		}
		catch( ... )
		{
			Log( Module::EXE ).critical( "Unknown exception thrown when processing batch of messages - aborting stream transaction and exiting early!" );
			m_updateProducer->abortTransaction();
			throw;
		}

		// After batch successfully commited to update log, apply map updates and send responses

		for( const auto& [uuid, newVer] : batchOutput.versionMapUpdates )
			m_versionMap[uuid] = newVer;
		for( const auto& [uuid, payload] : batchOutput.latestSerialisedRecordUpdates )
			m_latestSerialisedRecordMap[uuid] = payload;

		for( const auto& [resp, topic] : batchOutput.responses )
			sendCommandResponse( resp, topic );
	}
}

void RefDataCmdExecutorService::registerConfigOptions( Cfg::ConfigWrangler& cfg )
{
	cfg.add( m_config.streamSvcDSH,     "--streamServiceDSH", "The DSH of the streaming service to use" );
	cfg.add( m_config.msgSvcDSH,        "--msgSvcDSH",        "The DSH of the messaging service to use" );
	cfg.add( m_config.entities,         "--entities",         "The set of reference data entities to process commands for. If empty, subscribes to all entities." );
	cfg.add( m_config.disabledEntities, "--disabledEntities", "The set of reference data entities to NOT process commands for." );
}

template<RD::c_RefData T>
void RefDataCmdExecutorService::processUpsertCmdMessage( const StreamConsumerMessageView& msg, BatchOutput& batchOutput )
{
	processCmdMessage<RD::Cmd::Upsert<T>>( msg, batchOutput,
		[] ( std::optional<uint32_t> curVer, const uint32_t expected ) -> bool
		{
			return ( !curVer && expected == 0 ) ||     // Valid new entity
				   (  curVer && expected == *curVer ); // Or existing entity with correct expected version
		},
		[] ( const RD::Cmd::Upsert<T>& cmd, const uint32_t newVer ) -> RD::Record<T>
		{
			RD::Record<T> newRecord;
			newRecord.data                 = cmd.data;
			newRecord.header.isActive      = true;
			newRecord.header.lastUpdatedBy = cmd.updatedBy;
			newRecord.header.lastUpdatedTs = Time::DateTime::nowUTC();
			newRecord.header.version       = newVer;
			newRecord.header.uuid          = cmd.targetUUID;
			return newRecord;
		}
	);
}

template<RD::c_RefData T>
void RefDataCmdExecutorService::processDeactivateCmdMessage( const StreamConsumerMessageView& msg, BatchOutput& batchOutput )
{
	processCmdMessage<RD::Cmd::Deactivate<T>>( msg, batchOutput,
		[] ( std::optional<uint32_t> curVer, const uint32_t expected ) -> bool
		{
			return curVer && expected == *curVer; // Existing entity with correct expected version
		},
		[this, &batchOutput] ( const RD::Cmd::Deactivate<T>& cmd, const uint32_t newVer ) -> RD::Record<T>
		{
			RD::Record<T> newRecord;
			LatestSerialisedRecordMap::iterator it;
			if( it = batchOutput.latestSerialisedRecordUpdates.find( cmd.targetUUID ); it != batchOutput.latestSerialisedRecordUpdates.end() )
				newRecord.data = m_serialiser->deserialise<RD::Record<T>>( it->second ).data;
			else if( it = m_latestSerialisedRecordMap.find( cmd.targetUUID ); it != m_latestSerialisedRecordMap.end() )
				newRecord.data = m_serialiser->deserialise<RD::Record<T>>( it->second ).data;
			else
				throw ARQException( std::format( "Unable to find latest record for existing {} with UUID {}", RD::Traits<T>::name(), cmd.targetUUID ) );
			newRecord.header.isActive      = false;
			newRecord.header.lastUpdatedBy = cmd.updatedBy;
			newRecord.header.lastUpdatedTs = Time::DateTime::nowUTC();
			newRecord.header.version       = newVer;
			newRecord.header.uuid          = cmd.targetUUID;
			return newRecord;
		}
	);
}

template<RD::Cmd::c_Command T>
void RefDataCmdExecutorService::processCmdMessage( const StreamConsumerMessageView& msg, BatchOutput& batchOutput,
							std::function<bool( std::optional<uint32_t> version, const uint32_t expected )> versionCheckFunc,
							std::function<RD::Record<typename RD::Cmd::Traits<T>::EntityType>( const T& cmd, const uint32_t newVer )> recordBuilderFunc )
{
	using EntityType = typename RD::Cmd::Traits<T>::EntityType;

	const auto cmd = m_serialiser->deserialise<T>( msg.data );

	std::optional<uint32_t> curVer = getCurVer( cmd.targetUUID, batchOutput );
	const bool isValid = versionCheckFunc( curVer, cmd.expectedVersion );

	RD::CommandResponse resp;
	resp.corrID = ID::uuidFromStr( msg.tryGetHeaderValue( "ARQ_CorrID" ) );

	if( isValid )
	{
		const uint32_t newVersion = curVer.value_or( 0 ) + 1;
		RD::Record<EntityType> newRecord = recordBuilderFunc( cmd, newVersion );
		batchOutput.versionMapUpdates[cmd.targetUUID] = newVersion;

		SharedBuffer payload = m_serialiser->serialise<RD::Record<EntityType>>( newRecord );
		batchOutput.latestSerialisedRecordUpdates[cmd.targetUUID] = payload;

		m_updateProducer->send( StreamProducerMessage{
			.topic = std::format( "ARQ.RefData.Updates.{}", RD::Traits<EntityType>::name() ),
			.id    = msg.offset,
			.key   = newRecord.header.uuid.toString(),
			.data  = payload
		} );

		resp.status = RD::CommandResponse::SUCCESS;
	}
	else
	{
		resp.status = RD::CommandResponse::REJECTED;
		resp.message = std::format( "Version mismatch for UUID {}: CurrentVersion={}, VersionExpectedByCommand={}",
									cmd.targetUUID, curVer ? std::to_string( *curVer ) : "None", cmd.expectedVersion );

		Log( Module::EXE ).warn( "Rejecting {} command from message {}: {}", RD::Cmd::Traits<T>::name(), msg.idStr(), *resp.message );
	}

	const std::string_view respTopic = msg.tryGetHeaderValue( "ARQ_ResponseTopic" );
	batchOutput.responses.push_back( std::make_pair( resp, respTopic ) );
}

std::optional<uint32_t> RefDataCmdExecutorService::getCurVer( const ID::UUID& targetUUID, const BatchOutput& batchOutput ) const
{
	std::optional<uint32_t> curVer;
	VersionMap::const_iterator it;
	if( it = batchOutput.versionMapUpdates.find( targetUUID ); it != batchOutput.versionMapUpdates.end() )
		curVer = it->second;
	else if( it = m_versionMap.find( targetUUID ); it != m_versionMap.end() )
		curVer = it->second;

	return curVer;
}

void RefDataCmdExecutorService::onRebalance( StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& cmdTPs )
{
	setReady( false );
	ARQDefer{ 
		setReady( true ); 
	} );

	Log( Module::EXE ).info( "Rebalance event occurred: {} ON {}", Enum::enum_name( eventType ), Str::join( cmdTPs ) );

	m_versionMap.clear();
	m_latestSerialisedRecordMap.clear();

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
								StreamConsumerOptions::AutoOffsetReset::Earliest );
	std::shared_ptr<IStreamConsumer> updateConsumer = StreamingServiceFactory::createConsumer( "Kafka", opts );

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

	Log( Module::EXE ).info( "Finished hydration of update partitions. Loaded {} entities.", m_versionMap.size() );
}

std::set<StreamTopicPartition> RefDataCmdExecutorService::mapToUpdatePartitions( const std::set<StreamTopicPartition>& cmdTPs )
{
	return cmdTPs
		| std::views::transform( [this] ( const auto& tp )
		  {
		      return std::make_pair( std::format( "ARQ.RefData.Updates.{}", getEntityFromCmdTopic( tp.first ) ), tp.second );
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
		const std::string_view entityName = getEntityFromUpdateTopic( msg.topic );
		RD::dispatch( entityName, [this, &msg] <RD::c_RefData T> ( )
		{
			auto record = m_serialiser->deserialise<RD::Record<T>>( msg.data );
			m_versionMap[record.header.uuid] = record.header.version;
			m_latestSerialisedRecordMap[record.header.uuid] = Buffer( msg.data.data, msg.data.size );
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

void RefDataCmdExecutorService::buildTopicEntityMaps()
{
	for( const auto& meta : RD::Meta::getAll() )
	{
		std::string cmdTopic = std::format( "ARQ.RefData.Commands.{}", meta.name.data() );
		m_cmdTopicToEntity[cmdTopic] = meta.name;
		std::string updateTopic = std::format( "ARQ.RefData.Updates.{}", meta.name.data() );
		m_updateTopicToEntity[updateTopic] = meta.name;
	}
}

static std::string_view getEntityFromTopic( const std::unordered_map<std::string, std::string_view, TransparentStringHash, std::equal_to<>>& topicToEntity, const std::string_view topic )
{
	auto it = topicToEntity.find(  topic );
	if( it != topicToEntity.end() )
		return it->second;
	else
		throw ARQException( std::format( "Unknown RefData topic: {}", topic ) );
}

std::string_view RefDataCmdExecutorService::getEntityFromUpdateTopic( const std::string_view topic )
{
	return getEntityFromTopic( m_updateTopicToEntity, topic );
}

std::string_view RefDataCmdExecutorService::getEntityFromCmdTopic( const std::string_view topic )
{
	return getEntityFromTopic( m_cmdTopicToEntity, topic );
}
