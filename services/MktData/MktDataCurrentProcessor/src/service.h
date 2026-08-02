#pragma once

#include "current_market_selector.h"

#include <ARQUtils/hashers.h>
#include <ARQCore/serialiser.h>
#include <ARQCore/service_base.h>
#include <ARQCore/streaming_service.h>

#include <set>
#include <unordered_map>

using namespace ARQ;

class MktDataCurrentProcessorService : public ServiceBase
{
public:
	std::string_view serviceName()        override { return "MktDataCurrentProcessor"; }
	std::string_view serviceDescription() override { return "Selects the current record for each market-data object from authoritative observations "
		                                                          "and transactionally publishes rebuildable current-state changes to Kafka."; }

	void onStartup()  override;
	void onShutdown() override;

	void run() override;

	void registerConfigOptions( Cfg::ConfigWrangler& cfg ) override;

private:
	struct Config
	{
		std::string streamSvcDSH = "Kafka";

		std::set<std::string> entities;
		std::set<std::string> disabledEntities;
	} m_config;

	using SelectorMap = std::unordered_map<std::string, CurrentMarketSelector, TransparentStringHash, std::equal_to<>>;

private: // Observation processing
	void processObservationBatch( const IStreamConsumerMessageBatch& msgBatch );
	void processObservationMessage( const StreamConsumerMessageView& msg );
	void sendToDLQ( const StreamConsumerMessageView& msg );

private: // Rebalance and hydration
	void                           onRebalance( StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& observationPartitions );
	void                           hydrateState( const std::set<StreamTopicPartition>& observationPartitions );
	std::set<StreamTopicPartition> mapToCurrentPartitions( const std::set<StreamTopicPartition>& observationPartitions ) const;
	StreamTopicPartitionOffsets    getHydrationTargets( IStreamConsumer& consumer, const std::set<StreamTopicPartition>& currentPartitions ) const;
	void                           processHydrationMessage( const StreamConsumerMessageView& msg );
	void                           updateHydrationProgress( IStreamConsumer& consumer, StreamTopicPartitionOffsets& highWatermarks ) const;

private:
	std::shared_ptr<Serialiser>      m_serialiser;
	std::shared_ptr<IStreamConsumer> m_observationConsumer;
	std::shared_ptr<IStreamProducer> m_currentProducer;

	SelectorMap m_selectors;
};
