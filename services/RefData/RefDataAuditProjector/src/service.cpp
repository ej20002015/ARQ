#include "service.h"

#include <ARQUtils/algos.h>
#include <ARQCore/refdata_meta.h>

void RefDataAuditProjectorService::onStartup()
{
	m_entities = Algos::makeEffectiveSet( m_config.entities, RD::Meta::getAllNames(), m_config.disabledEntities );
	initTopicToEntityMap();

	m_backoffPolicy = BackoffPolicy( m_config.dbBackoffPolicy );

	m_serialiser    = SerialiserFactory::create( SerialiserFactory::SerialiserImpl::Protobuf );
	m_auditRDSource = RD::SourceFactory::create( m_config.auditDSH );

	StreamConsumerOptions opts( "RefDataAuditProjector::UpdateConsumer",
								"ARQ.RefData.AuditProjectors",
								StreamConsumerOptions::FetchPreset::HighThroughput,
								StreamConsumerOptions::AutoCommitOffsets::Disabled,
								StreamConsumerOptions::AutoOffsetReset::Earliest );
	m_updateConsumer = StreamingServiceFactory::createConsumer( m_config.streamSvcDSH, opts );

	const auto updateTopics = m_updateTopicToEntity | std::views::keys | std::ranges::to<std::set>();
	m_updateConsumer->subscribe( updateTopics );

	StreamProducerOptions prodOpts( "RefDataAuditProjector::DLQProducer",
									StreamProducerOptions::Preset::HighThroughput );
	m_dlqProducer = StreamingServiceFactory::createProducer( m_config.streamSvcDSH, prodOpts );
}

void RefDataAuditProjectorService::onShutdown()
{
	m_updateConsumer.reset();
	m_auditRDSource.reset();
	m_serialiser.reset();
}

void RefDataAuditProjectorService::run()
{
	RD::RecordCollection rcdColl;
	
	while( shouldRun() )
	{
		rcdColl.clear();

		auto msgBatch = m_updateConsumer->poll( 2s, StreamConsumerReadHeaders::SKIP_HEADERS );
		if( msgBatch->empty() )
			continue;

		processMsgBatch( std::move( msgBatch ), rcdColl );

		const bool allInserted = insertIntoAuditDB( rcdColl );

		if( allInserted )
			m_updateConsumer->commitOffsets();
	}
}

void RefDataAuditProjectorService::registerConfigOptions( Cfg::ConfigWrangler& cfg )
{
	cfg.add( m_config.streamSvcDSH,     "--streamServiceDSH", "The DSH of the streaming service to use" );
	cfg.add( m_config.auditDSH,         "--auditDSH",         "The DSH of the audit DB to write updates to" );
	cfg.add( m_config.entities,         "--entities",         "The set of reference data entities to process updates for. If empty, subscribes to all entities." );
	cfg.add( m_config.disabledEntities, "--disabledEntities", "The set of reference data entities to NOT process updates for." );
	cfg.add( m_config.dbBackoffPolicy,  "--dbBackoffPolicy",  "The backoff policy to use when retrying saves to the audit DB\n" + std::string( BackoffPolicy::HelpText ) );
}

void RefDataAuditProjectorService::initTopicToEntityMap()
{
	for( const std::string_view entityName : m_entities )
	{
		const std::string updateTopic = std::format( "ARQ.RefData.Updates.{}", entityName );
		m_updateTopicToEntity[std::move( updateTopic )] = entityName;
	}
}

std::string_view RefDataAuditProjectorService::getEntityFromUpdateTopic( const std::string_view topic )
{
	auto it = m_updateTopicToEntity.find( topic );
	if( it != m_updateTopicToEntity.end() )
		return it->second;
	else
		throw ARQException( std::format( "Unknown RefData topic: {}", topic ) );
}

void RefDataAuditProjectorService::processMsgBatch( std::unique_ptr<IStreamConsumerMessageBatch> msgBatch, RD::RecordCollection& rcdColl )
{
	Log( Module::EXE ).debug( "Processing {} update messages", msgBatch->size() );

	bool anyDLQ = false;
	for( const StreamConsumerMessageView& msg : *msgBatch )
	{
		ARQ_DO_IN_TRY( arqExc, errMsg );
		{
			const std::string_view entityName = getEntityFromUpdateTopic( msg.topic );
			RD::dispatch( entityName, [this, &msg, &rcdColl] <RD::c_RefData T> ( )
			{
				auto rcd = m_serialiser->deserialise<RD::Record<T>>( msg.data );
				rcdColl.get<RD::Record<T>>().push_back( std::move( rcd ) );
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
			anyDLQ = true;
			const std::string key = msg.key.has_value() ? msg.key->data() : "NO_KEY";
			m_dlqProducer->send( StreamProducerMessage{
				.topic = std::format( "{}.DLQ", msg.topic ),
				.id = msg.offset,
				.key = key,
				.data = msg.data
			} );
		}
	}

	if( anyDLQ )
		m_dlqProducer->flush();
}

bool RefDataAuditProjectorService::insertIntoAuditDB( const RD::RecordCollection& rcdColl )
{
	bool allInserted = true;
	rcdColl.visitVectors( [this, &allInserted] ( const auto& recordsVec )
	{
		using VectorType = std::remove_cvref_t<decltype( recordsVec )>;
		using EntityType = typename VectorType::value_type::EntityType;

		if( recordsVec.empty() )
			return;

		bool inserted = false;
		m_backoffPolicy.reset();
		while( shouldRun() && !inserted )
		{
			try
			{
				Log( Module::EXE ).debug( "Inserting {} {} refdata entities into the audit DB", recordsVec.size(), RD::Traits<EntityType>::name() );
				m_auditRDSource->insert( recordsVec );
				inserted = true;
			}
			catch( ARQException& e )
			{
				auto delayTimeOpt = m_backoffPolicy.nextDelay();
				if( delayTimeOpt )
				{
					Log( Module::EXE ).error( e, "Exception thrown when inserting {} refdata entities into the audit DB - trying again in {}ms ({})",
											  RD::Traits<EntityType>::name(), *delayTimeOpt, m_backoffPolicy.attemptStr() );
					std::this_thread::sleep_for( *delayTimeOpt );
				}
				else
				{
					auto errMsg = std::format( "Exception thrown when inserting {} refdata entities into the audit DB - max save attempts exceeded - STOPPING SERVICE!",
											   RD::Traits<EntityType>::name() );
					Log( Module::EXE ).critical( "", errMsg );
					e.str() += " - " + errMsg;
					throw;
				}
			}
		}

		allInserted &= inserted;
	} );

	return allInserted;
}
