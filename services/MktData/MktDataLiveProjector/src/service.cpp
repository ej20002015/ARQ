#include "service.h"

#include <ARQUtils/algos.h>
#include <ARQMarket/mktdata_entities.h>
#include <ARQMarket/mktdata_meta.h>
#include <ARQMarket/mktdata_topics.h>

void MktDataLiveProjectorService::onStartup()
{
	m_backoffPolicy = BackoffPolicy( m_config.dbBackoffPolicy );

	m_serialiser = SerialiserFactory::inst().create( SerialiserFactory::SerialiserImpl::Protobuf );

	m_offsetSource     = StreamOffsetSourceFactory::inst().create( m_config.liveDSH );
	m_liveMarketSource = MD::MarketSourceFactory::inst().create( m_config.liveDSH );

	StreamConsumerOptions opts( "MktDataLiveProjector::UpdateConsumer",
								"ARQ.MktData.LiveProjectors",
								StreamConsumerOptions::FetchPreset::LowLatency,
								StreamConsumerOptions::AutoCommitOffsets::Disabled,
								StreamConsumerOptions::AutoOffsetReset::Earliest );
	m_updateConsumer = StreamingServiceFactory::inst().createConsumer( m_config.streamSvcDSH, opts );

	const std::set<std::string_view> entities = Algos::makeEffectiveSet( m_config.entities, MD::Meta::getAllNames(), m_config.disabledEntities );
	const auto updateTopics = entities
		| std::views::transform( [] ( std::string_view name ) { return MD::getUpdateTopic( name ); } )
		| std::ranges::to<std::set<std::string>>();
	m_updateConsumer->subscribe( updateTopics );

	StreamProducerOptions prodOpts( "MktDataLiveProjector::DLQProducer" );
	m_dlqProducer = StreamingServiceFactory::inst().createProducer( m_config.streamSvcDSH, prodOpts );

	m_messagingService = MessagingServiceFactory::inst().create( m_config.msgSvcDSH );
}

void MktDataLiveProjectorService::onShutdown()
{
	m_updateConsumer.reset();
	m_dlqProducer.reset();
	m_liveMarketSource.reset();
	m_offsetSource.reset();
	m_messagingService.reset();
}

void MktDataLiveProjectorService::run()
{
	std::unordered_map<std::string, MD::MarketUpdateBatch> updateBatches;
	
	while( shouldRun() )
	{
		updateBatches.clear();

		auto msgBatch = m_updateConsumer->poll( 5ms, StreamConsumerReadHeaders::SKIP_HEADERS );
		if( msgBatch->empty() )
			continue;

		processMsgBatch( std::move( msgBatch ), updateBatches );
		insertIntoLiveMarketSource( updateBatches );
		publishToMessagingService( updateBatches );
		m_updateConsumer->commitOffsetsAsync();
	}
}

void MktDataLiveProjectorService::registerConfigOptions( Cfg::ConfigWrangler& cfg )
{
	cfg.add( m_config.streamSvcDSH,     "--streamServiceDSH", "The DSH of the streaming service to read updates from" );
	cfg.add( m_config.liveDSH,          "--liveDSH",          "The DSH of the live market source to write to" );
	cfg.add( m_config.msgSvcDSH,        "--msgSvcDSH",        "The DSH of the messaging service to publish updates to" );
	cfg.add( m_config.mkts,             "--mkts",             "The set of markets to process updates for. If empty, process updates on all markets." );
	cfg.add( m_config.entities,         "--entities",         "The set of market data entities to process updates for. If empty, subscribes to all entities." );
	cfg.add( m_config.disabledEntities, "--disabledEntities", "The set of market data entities to NOT process updates for." );
	cfg.add( m_config.dbBackoffPolicy,  "--dbBackoffPolicy",  "The backoff policy to use when retrying saves to the live market source\n" + std::string( BackoffPolicy::HelpText ) );
}

void MktDataLiveProjectorService::processMsgBatch( std::unique_ptr<IStreamConsumerMessageBatch> msgBatch, std::unordered_map<std::string, MD::MarketUpdateBatch>& updateBatches )
{
	Log( Module::EXE ).debug( "Processing {} update messages", msgBatch->size() );

	bool anyDLQ = false;
	for( const StreamConsumerMessageView& msg : *msgBatch )
	{
		ARQ_DO_IN_TRY( arqExc, errMsg );
		{
			if( !msg.key.has_value() )
				throw ARQException( "Message missing key - cannot determine market routing" );

			const MD::Type entityType = MD::getTypeFromUpdateTopic( msg.topic );

			MD::dispatch( entityType, [this, &msg, &updateBatches] <MD::c_MktData T> ()
			{
				auto rcdMsg = m_serialiser->deserialise<MD::RecordMessage<T>>( msg.data );
				MD::MarketUpdateBatch& updateBatch = updateBatches[rcdMsg.mktName];
				updateBatch.records.get<MD::Record<T>>().push_back( std::move( rcdMsg.record ) );
				updateBatch.offsets[StreamTopicPartition{ msg.topic, msg.partition }] = msg.offset;
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
			Log( Module::EXE ).error( "Exception thrown when processing message so sending to DLQ [{}] - what: {}", msg.idStr(), errMsg );
			toDLQ = true;
		}

		if( toDLQ )
		{
			anyDLQ = true;
			const std::string key = msg.key.has_value() ? std::string( msg.key->data() ) : "NO_KEY";
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

	for( auto& [mktName, updateBatch] : updateBatches )
		updateBatch.marketName = Mkt::Name::fromStr( mktName );
}

void MktDataLiveProjectorService::insertIntoLiveMarketSource( const std::unordered_map<std::string, MD::MarketUpdateBatch>& updateBatches )
{
	for( const auto& [mktName, updateBatch] : updateBatches )
	{
		if( updateBatch.records.empty() )
			continue;

		insertMktData( mktName, updateBatch.records );
		insertOffsets( mktName, updateBatch.offsets );
	}
}

void MktDataLiveProjectorService::insertMktData( const std::string_view marketName, const MD::RecordCollection& rcdColl )
{
	m_backoffPolicy.reset();
	while( shouldRun() )
	{
		try
		{
			Log( Module::EXE ).debug( "Saving {} mkdata entities for market [{}] into the live market source", rcdColl.size(), marketName );
			m_liveMarketSource->save( marketName, rcdColl );
			break;
		}
		catch( ARQException& e )
		{
			auto delayTimeOpt = m_backoffPolicy.nextDelay();
			if( delayTimeOpt )
			{
				Log( Module::EXE ).error( e, "Exception thrown when saving mkdata entities into the live market source - trying again in {}ms ({})", *delayTimeOpt, m_backoffPolicy.attemptStr() );
				std::this_thread::sleep_for( *delayTimeOpt );
			}
			else
			{
				static constexpr std::string_view errMsg = "Exception thrown when saving mkdata entities into the live market source - max save attempts exceeded - STOPPING SERVICE!";
				Log( Module::EXE ).critical( errMsg );
				e.str() += " - " + std::string( errMsg );
				throw;
			}
		}
	}
}

void MktDataLiveProjectorService::insertOffsets( const std::string_view marketName, const StreamTopicPartitionOffsets& offsets )
{
	m_backoffPolicy.reset();
	while( shouldRun() )
	{
		try
		{
			Log( Module::EXE ).debug( "Saving topic partition offsets for market [{}] into the live market source", marketName );
			m_offsetSource->saveOffsets( std::format( "{}:{}", MARKETS_KEY_NAMESPACE, marketName ), offsets );
			break;
		}
		catch( ARQException& e )
		{
			auto delayTimeOpt = m_backoffPolicy.nextDelay();
			if( delayTimeOpt )
			{
				Log( Module::EXE ).error( e, "Exception thrown when saving offsets into the live market source - trying again in {}ms ({})", *delayTimeOpt, m_backoffPolicy.attemptStr() );
				std::this_thread::sleep_for( *delayTimeOpt );
			}
			else
			{
				static constexpr std::string_view errMsg = "Exception thrown when saving offsets into the live market source - max save attempts exceeded - STOPPING SERVICE!";
				Log( Module::EXE ).critical( errMsg );
				e.str() += " - " + std::string( errMsg );
				throw;
			}
		}
	}
}

void MktDataLiveProjectorService::publishToMessagingService( const std::unordered_map<std::string, MD::MarketUpdateBatch>& updateBatches )
{
	for( const auto& [mktName, updateBatch] : updateBatches )
	{
		if( updateBatch.records.empty() )
			continue;

		m_backoffPolicy.reset();
		while( shouldRun() )
		{
			try
			{
				Log( Module::EXE ).debug( "Publishing market update batch for market [{}] to the messaging service", mktName );

				Message msg{
					.data = m_serialiser->serialise( updateBatch )
				};
				const std::string topic = std::string( UPDATES_PUB_TOPIC_PFX ) + mktName;

				// Note: We assume that the publish message is small enough that it's below the max message size of the messaging service. If this is not the case, we would need to implement chunking logic here to split the batch into multiple messages.
				m_messagingService->publish( topic, std::move( msg ) );

				break;
			}
			catch( ARQException& e )
			{
				auto delayTimeOpt = m_backoffPolicy.nextDelay();
				if( delayTimeOpt )
				{
					Log( Module::EXE ).error( e, "Exception thrown when publishing market update batch - trying again in {}ms ({})", *delayTimeOpt, m_backoffPolicy.attemptStr() );
					std::this_thread::sleep_for( *delayTimeOpt );
				}
				else
				{
					static constexpr std::string_view errMsg = "Exception thrown when publishing market update batch - max publish attempts exceeded - STOPPING SERVICE!";
					Log( Module::EXE ).critical( errMsg );
					e.str() += " - " + std::string( errMsg );
					throw;
				}
			}
		}
	}
}
