#include <ARQNats/nats_messaging_service_interface.h>

#include <ARQUtils/error.h>
#include <ARQCore/data_source_config.h>

namespace ARQ
{

static std::string formatNatsError( natsStatus rc )
{
	return std::format( "nats.c error: code={}, error={}, message={}", rc, natsStatus_GetText( rc ), nats_GetLastError( &rc ) );
}

/*
*********************************************
*  Implementation of NatsMessagingService   *
*********************************************
*/

NatsMessagingService::NatsMessagingService( const std::string_view dsh )
	: m_dsh( dsh )
{
	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );
	
	natsStatus rc;

	natsOptions* natsOptions;
	if( rc = natsOptions_Create( &natsOptions ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_Create() call failed with {}", formatNatsError( rc ) ) );

}

void NatsMessagingService::publish( const std::string_view topic, const Message& msg )
{

}

std::unique_ptr<ISubscription> NatsMessagingService::subscribe( const std::string_view topicPattern, std::shared_ptr<ISubscriptionHandler> subHandler )
{

}

GlobalStats NatsMessagingService::getStats() const
{

}

/*
*********************************************
*    Implementation of NatsSubscription     *
*********************************************
*/

int64_t NatsSubscription::getID()
{
}

std::string_view NatsSubscription::getTopic()
{
}

bool NatsSubscription::isValid()
{
}

SubStats NatsSubscription::getStats()
{
}

void NatsSubscription::unsubscribe()
{
}

void NatsSubscription::drainAndUnsubscribe()
{
}

void NatsSubscription::blockOnDrainAndUnsubscribe()
{
}

}