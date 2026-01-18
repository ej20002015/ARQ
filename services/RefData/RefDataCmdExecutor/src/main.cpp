
#include <ARQUtils/enum.h>
#include <ARQUtils/id.h>
#include <ARQCore/streaming_service.h>
#include <ARQCore/messaging_service.h>
#include <ARQCore/logger.h>
#include <ARQCore/refdata_commands.h>
#include <ARQCore/refdata_command_manager.h>
#include <ARQCore/serialiser.h>

#include <csignal>
#include <atomic>
#include <algorithm>

using namespace ARQ;

std::atomic<bool> running( true );

std::shared_ptr<Serialiser> serialiser;
std::shared_ptr<IMessagingService> msgSvc;
std::shared_ptr<IStreamConsumer> consumer;
std::shared_ptr<IStreamProducer> producer;

using VersionMap = std::unordered_map<ID::UUID, uint32_t>;

VersionMap versionMap;

struct BatchOutput
{
	using CommandResponseAndTopic = std::pair<RD::CommandResponse, std::string_view>;

	VersionMap                           versionMapUpdates;
	std::vector<CommandResponseAndTopic> responses;
};

void handleSIGINT( int signum )
{
	running = false;
}

void onRebalance( StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& topicPartitions )
{
	std::stringstream ss;
	for( const auto& [topic, partition] : topicPartitions )
		ss << std::format( "[Topic: {}, Partition: {}], ", topic, partition );

	Log( Module::REFDATA ).info( "Rebalance event occurred: {} ON {}", Enum::enum_name( eventType ), ss.str() );

	versionMap.clear();

	if( eventType == StreamRebalanceEventType::PARTITIONS_REVOKED || topicPartitions.empty() )
		return;	

	StreamConsumerOptions opts( "RefDataCmdExecutor::UpdateConsumer",
								"ARQ.RefData.CommandExecutors.UpdateHydration",
								StreamConsumerOptions::FetchPreset::Standard,
								StreamConsumerOptions::AutoCommitOffsets::Disabled,
								StreamConsumerOptions::AutoOffsetReset::Earliest );
	std::shared_ptr<IStreamConsumer> updateConsumer = StreamingServiceFactory::createConsumer( "Kafka", opts );

	std::set<StreamTopicPartition> equivalentUpdatePartitions;
	for( const auto& [_, partition] : topicPartitions )
		equivalentUpdatePartitions.insert( std::make_pair( "ARQ.RefData.Updates.Currency", partition ) );

	std::set<StreamTopicPartition> topicPartitionsToConsume;
	std::map<StreamTopicPartition, int64_t> highWatermarks;

	std::map<StreamTopicPartition, int64_t> begOffsets = updateConsumer->beginningOffsets( equivalentUpdatePartitions );
	std::map<StreamTopicPartition, int64_t> endOffsets = updateConsumer->endOffsets( equivalentUpdatePartitions );

	for( const auto& tp : equivalentUpdatePartitions )
	{
		const int64_t beginning = begOffsets.at( tp );
		const int64_t end       = endOffsets.at( tp );

		if( end > beginning ) // There is data
		{
			topicPartitionsToConsume.insert( tp );
			highWatermarks[tp] = end - 1;

			Log( Module::REFDATA ).info( "Partition {} needs hydration up to offset {}", tp, end - 1 );
		}
		else
			Log( Module::REFDATA ).info( "Partition {} is empty - skipping hydration", tp );
	}

	if( topicPartitionsToConsume.empty() )
		return;

	updateConsumer->assign( topicPartitionsToConsume );
	updateConsumer->seekToBeginning();

	bool hydrationComplete = false;
	while( !hydrationComplete && running )
	{
		const auto msgBatch = updateConsumer->poll( 50ms );

		for( const auto& msg : *msgBatch )
		{
			try
			{
				auto record = serialiser->deserialise<RD::Record<RD::Currency>>( msg.data );
				versionMap[record.header.uuid] = record.header.version;
			}
			catch( ... ) { /* Ignore malformed data in history */ }
		}

		auto it = highWatermarks.begin();
		while( it != highWatermarks.end() )
		{
			int32_t partition = it->first.second;
			int64_t targetHW = it->second;

			int64_t nextOffset = updateConsumer->position( it->first );
			if( nextOffset > targetHW )
			{
				Log( Module::REFDATA ).info( "Hydration complete for Partition {}", partition );
				it = highWatermarks.erase( it );
			}
			else
			{
				++it;
			}
		}

		if( highWatermarks.empty() )
			hydrationComplete = true;
	}

	Log( Module::REFDATA ).info( "State Hydration Complete - version map size: {}", versionMap.size() );
}

void sendCommandResponse( const RD::CommandResponse& cmdRes, std::string_view topic )
{
	Buffer buf = serialiser->serialise<RD::CommandResponse>( cmdRes );
	Message msg = {
		.data = std::move( buf )
	};

	msgSvc->publish( topic, msg );
}

void processMessage( const StreamConsumerMessageView& msg, BatchOutput& batchOutput )
{
 	const auto upsertCmd = serialiser->deserialise<RD::Cmd::Upsert<RD::Currency>>( msg.data );

	std::optional<uint32_t> curVer;
	VersionMap::iterator it;
	if( it = batchOutput.versionMapUpdates.find( upsertCmd.targetUUID ); it != batchOutput.versionMapUpdates.end() )
		curVer = it->second;
	else if( it = versionMap.find( upsertCmd.targetUUID ); it != versionMap.end() )
		curVer = it->second;

	RD::CommandResponse resp;
	resp.corrID = ID::UUID::fromString( msg.headers.at( "ARQ_CorrID" ) );

	if( ( !curVer && upsertCmd.expectedVersion == 0 ) ||      // Valid new entity
		( curVer && upsertCmd.expectedVersion == *curVer ) )  // Or existing entity with correct expected version
	{
		const uint32_t newVersion = curVer.value_or( 0 ) + 1;
		batchOutput.versionMapUpdates[upsertCmd.targetUUID] = newVersion;

		RD::Record<RD::Currency> newCurrencyRecord;
		newCurrencyRecord.data = upsertCmd.data;
		newCurrencyRecord.header.isActive = true;
		newCurrencyRecord.header.lastUpdatedBy = upsertCmd.updatedBy;
		newCurrencyRecord.header.lastUpdatedTs = Time::DateTime::nowUTC();
		newCurrencyRecord.header.version = newVersion;
		newCurrencyRecord.header.uuid = upsertCmd.targetUUID;

		Buffer payload = serialiser->serialise<RD::Record<RD::Currency>>( newCurrencyRecord );

		producer->send( StreamProducerMessage{
			.topic = "ARQ.RefData.Updates.Currency",
			.id = msg.offset,
			.key = newCurrencyRecord.header.uuid.toString(),
			.data = SharedBuffer( std::move( payload ) )
		} );

		resp.status = RD::CommandResponse::SUCCESS;
	}
	else
	{
		std::string msg = std::format( "Incorrect expectedVersion given in command - curVer={}, expectedVersion={}", curVer.value_or( 0 ), upsertCmd.expectedVersion );
		Log( Module::REFDATA ).warn( "{}", msg );
		resp.status = RD::CommandResponse::REJECTED;
		resp.message = msg;
	}

	const std::string_view respTopic = msg.headers.at( "ARQ_ResponseTopic" );
	batchOutput.responses.push_back( std::make_pair( resp, respTopic ) );
}

int main()
{
	// Startup
	std::signal( SIGINT, handleSIGINT );

	serialiser = SerialiserFactory::create( SerialiserFactory::SerialiserImpl::Protobuf );

	msgSvc = MessagingServiceFactory::create( "NATS" );

	StreamConsumerOptions opts( "RefDataCmdExecutor::CommandConsumer",
								"ARQ.RefData.CommandExecutors",
								StreamConsumerOptions::FetchPreset::Standard,
								StreamConsumerOptions::AutoCommitOffsets::Disabled,
								StreamConsumerOptions::AutoOffsetReset::Earliest );
	consumer = StreamingServiceFactory::createConsumer( "Kafka", opts );
	consumer->subscribe( { "ARQ.RefData.Commands.Currency" }, onRebalance );

	StreamProducerOptions prodOpts( "RefDataCmdExecutor::UpdateProducer",
									StreamProducerOptions::Preset::HighThroughput );
	prodOpts.setOptionOverride( "transactional.id", ID::UUID::create().toString() );
	producer = StreamingServiceFactory::createProducer( "Kafka", prodOpts );
	producer->initTransactions();

	// Temp
	/*consumer->seekToBeginning();*/

	// Run loop
	while( running )
	{
		const auto msgBatch = consumer->poll( 100ms );
		if( msgBatch->empty() )
			continue;

		Log( Module::REFDATA ).debug( "Processing {} command messages", msgBatch->size() );

		try
		{
			BatchOutput batchOutput;
			batchOutput.versionMapUpdates.reserve( msgBatch->size() );
			batchOutput.responses.reserve( msgBatch->size() );

			StreamTopicPartitionOffsets offsetsToCommit;

			producer->beginTransaction();

			for( const StreamConsumerMessageView& msg : *msgBatch )
			{
				std::string headersStr;
				/*for( const auto& [key, val] : msg.headers )
					headersStr += std::format( "{}: {}, ", key, val );
				Log( Module::REFDATA ).info( "Message consumed: "
					"Topic={}, Partition={}, Offset={}, Key={}, Timestamp={}, Headers=[{}], Error={}",
					msg.topic,
					msg.partition,
					msg.offset,
					msg.key.value_or( "N/A" ),
					msg.timestamp.fmtISO8601(),
					headersStr,
					( msg.error ? msg.error->message : "N/A" )
				)*/;

				try
				{
					processMessage( msg, batchOutput );
				}
				catch( ... )
				{
					Log( Module::REFDATA ).error( "Error when processing message - sending to DMQ" );
					producer->send( StreamProducerMessage{
						.topic = "ARQ.RefData.Commands.Currency.DLQ",
						.id = msg.offset,
						.key = msg.key->data(),
						.data = msg.data
					} );
				}

				offsetsToCommit[StreamTopicPartition{ msg.topic, msg.partition }] = msg.offset + 1;
			}

			producer->sendOffsetsToTransaction( offsetsToCommit, consumer->getGroupMetadata() );

			producer->commitTransaction();

			for( const auto& [uuid, newVer] : batchOutput.versionMapUpdates )
				versionMap[uuid] = newVer;

			for( const auto& [resp, topic] : batchOutput.responses )
				sendCommandResponse( resp, topic );
		}
		catch( ... )
		{
			Log( Module::REFDATA ).critical( "Error when processing batch of messages - aborting stream transaction and exiting!" );
			producer->abortTransaction();
			std::exit( 1 );
		}
	}

	// Shutdown

	return 0;
}