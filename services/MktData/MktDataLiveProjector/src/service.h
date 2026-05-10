#include <ARQUtils/backoff_policy.h>
#include <ARQUtils/hashers.h>
#include <ARQCore/service_base.h>
#include <ARQCore/streaming_service.h>
#include <ARQCore/stream_offset_source.h>
#include <ARQMarket/market.h>
#include <ARQMarket/mktdata_source.h>
#include <ARQMarket/mktdata_entities.h>
#include <ARQMarket/managed_market.h>

#include <set>
#include <unordered_map>

using namespace ARQ;

class MktDataLiveProjectorService : public ServiceBase
{
public:
	std::string_view serviceName()        override { return "MktDataLiveProjector"; };
	std::string_view serviceDescription() override { return "The core ingestion pipeline for live market updates. It consumes high-throughput streams, "
															"synchronizes current market state and partition offsets to the live store, and publishes "
															"unified, market-specific batches to the messaging service for real-time downstream consumption."; }

	void onStartup()  override;
	void onShutdown() override;

	void run() override;

	void registerConfigOptions( Cfg::ConfigWrangler& cfg ) override;

private:
	struct Config
	{
		std::string streamSvcDSH = "Kafka";
		std::string liveDSH      = "Redis";
		std::string msgSvcDSH    = "NATS";

		std::set<std::string> mkts; // If empty, process updates on all markets
		
		std::set<std::string> entities; // If empty, subscribe to all entities
		std::set<std::string> disabledEntities;

		std::string dbBackoffPolicy = "1s-3-1m-5";
	} m_config;

	static constexpr std::string_view UPDATES_PUB_TOPIC_PFX = "ARQ.MktData.Updates.";

private:
	void processMsgBatch( std::unique_ptr<IStreamConsumerMessageBatch> msgBatch, std::unordered_map<std::string, MD::MarketUpdateBatch>& updateBatches );
	void insertIntoLiveMarketSource( const std::unordered_map<std::string, MD::MarketUpdateBatch>& updateBatches );
	void insertMktData( const std::string_view marketName, const MD::RecordCollection& rcdColl );
	void insertOffsets( const std::string_view marketName, const StreamTopicPartitionOffsets& offsets );
	void publishToMessagingService( const std::unordered_map<std::string, MD::MarketUpdateBatch>& updateBatches );

private:
	std::shared_ptr<Serialiser> m_serialiser;

	std::shared_ptr<MD::IMarketSource> m_liveMarketSource;

	std::shared_ptr<IStreamOffsetSource> m_offsetSource;

	std::shared_ptr<IStreamConsumer> m_updateConsumer;
	std::shared_ptr<IStreamProducer> m_dlqProducer;

	std::shared_ptr<IMessagingService> m_messagingService;

	BackoffPolicy m_backoffPolicy;
};