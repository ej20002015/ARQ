#include <ARQUtils/backoff_policy.h>
#include <ARQUtils/hashers.h>
#include <ARQCore/service_base.h>
#include <ARQCore/serialiser.h>
#include <ARQCore/refdata_source.h>
#include <ARQCore/streaming_service.h>

#include <set>

using namespace ARQ;

class RefDataAuditProjectorService : public ServiceBase
{
public:
	std::string_view serviceName()        override { return "RefDataAuditProjector"; };
	std::string_view serviceDescription() override { return "The RefData Audit Projector Service provides the durable historical projection for reference data entities. "
															"It consumes the update stream, buffers heterogeneous changes for batch efficiency, and reliably writes to the audit store."; }

	void onStartup()  override;
	void onShutdown() override;

	void run() override;

	void registerConfigOptions( Cfg::ConfigWrangler& cfg ) override;

private:
	struct Config
	{
		std::string streamSvcDSH = "Kafka";
		std::string auditDSH     = "ClickHouseDB";

		std::set<std::string> entities; // If empty, subscribe to all entities
		std::set<std::string> disabledEntities;

		std::string dbBackoffPolicy = "1s-3-1m-5";
	} m_config;

private:

	void initTopicToEntityMap();
	std::string_view getEntityFromUpdateTopic( const std::string_view topic );

	void processMsgBatch( std::unique_ptr<IStreamConsumerMessageBatch> msgBatch, RD::RecordCollection& rcdColl );
	bool insertIntoAuditDB( const RD::RecordCollection& rcdColl );

private:
	std::shared_ptr<Serialiser>      m_serialiser;
	std::shared_ptr<RD::Source>      m_auditRDSource;

	std::shared_ptr<IStreamConsumer> m_updateConsumer;
	std::shared_ptr<IStreamProducer> m_dlqProducer;

	std::set<std::string_view> m_entities;

	std::unordered_map<std::string, std::string_view, TransparentStringHash, std::equal_to<>> m_updateTopicToEntity;

	BackoffPolicy m_backoffPolicy;
};