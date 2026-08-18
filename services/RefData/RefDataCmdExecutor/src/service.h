#pragma once

#include <ARQCore/service_base.h>
#include <ARQCore/serialiser.h>
#include <ARQCore/messaging_service.h>
#include <ARQCore/streaming_service.h>
#include <ARQCore/refdata_command_manager.h>
#include <ARQCore/refdata_command_processor.h>

using namespace ARQ;

class RefDataCmdExecutorService : public ServiceBase
{
public:
	std::string_view serviceName()        override { return "RefDataCmdExecutor"; };
	std::string_view serviceDescription() override { return "The RefData Command Executor Service is the write-side authority for reference data entities. "
															"It consumes commands from Kafka, enforces optimistic locking using an in-memory state cache, "
		                                                    "and transactionally commits valid updates to the immutable event stream while notifying requestors of success or failure."; };

	void onStartup()  override;
	void onShutdown() override;

	void run() override;

	void registerConfigOptions( Cfg::ConfigWrangler& cfg ) override;

private:
	struct Config
	{
		std::string streamSvcDSH = "Kafka";
		std::string msgSvcDSH    = "NATS";

		std::set<std::string> entities; // If empty, subscribe to all entities
		std::set<std::string> disabledEntities;
	} m_config;

	struct StoredRecord
	{
		uint32_t     version;
		SharedBuffer serialisedRecord;
	};

	using StoredRecordMap = std::unordered_map<ID::UUID, StoredRecord>;

	struct BatchOutput
	{
		using CommandResponseAndTopic = std::pair<RD::CommandResponse, std::string_view>;

		StoredRecordMap                      recordUpdates;
		std::vector<CommandResponseAndTopic> responses;
	};

private: // Command processing
	template<RD::c_RefData T>
	void processUpsertCmdMessage( const StreamConsumerMessageView& msg, BatchOutput& batchOutput );
	template<RD::c_RefData T>
	void processDeactivateCmdMessage( const StreamConsumerMessageView& msg, BatchOutput& batchOutput );
	template<RD::c_RefData T>
	void stageCommandDecision( const StreamConsumerMessageView& msg, const RD::CommandDecision<T>& decision, BatchOutput& batchOutput );
	template<RD::c_RefData T>
	std::optional<RD::Record<T>> getCurrentRecord( const ID::UUID& targetUUID, const BatchOutput& batchOutput ) const;
	std::optional<uint32_t>  getCurVer( const ID::UUID& targetUUID, const BatchOutput& batchOutput ) const;
	const StoredRecord*      findCurrentStoredRecord( const ID::UUID& targetUUID, const BatchOutput& batchOutput ) const;

private: // Rebalance/hydration
	void                           onRebalance( StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& cmdTPs );
	void                           hydrateState( const std::set<StreamTopicPartition>& cmdTPs );
	std::set<StreamTopicPartition> mapToUpdatePartitions( const std::set<StreamTopicPartition>& cmdTPs );
	StreamTopicPartitionOffsets    getHydrationTargets( IStreamConsumer& consumer, const std::set<StreamTopicPartition>& partitions );
	void                           processHydrationMessage( const StreamConsumerMessageView& msg );
	void                           updateHydrationProgress( IStreamConsumer& consumer, StreamTopicPartitionOffsets& highWatermarks );

private: // Helpers
	void sendCommandResponse( const RD::CommandResponse& cmdRes, std::string_view topic );

	const std::set<std::string_view>& getEntities();
	void                              buildTopicEntityMaps();
	std::string_view                  getEntityFromUpdateTopic( const std::string_view topic );
	std::string_view                  getEntityFromCmdTopic( const std::string_view topic );

private:
	std::shared_ptr<Serialiser>        m_serialiser;
	std::shared_ptr<IMessagingService> m_msgSvc;

	std::shared_ptr<IStreamConsumer> m_commandConsumer;
	std::shared_ptr<IStreamProducer> m_updateProducer;

	StoredRecordMap m_records;

	std::unordered_map<std::string, std::string_view, TransparentStringHash, std::equal_to<>> m_cmdTopicToEntity;
	std::unordered_map<std::string, std::string_view, TransparentStringHash, std::equal_to<>> m_updateTopicToEntity;
};
