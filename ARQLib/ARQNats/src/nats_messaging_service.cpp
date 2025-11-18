#include <ARQNats/nats_messaging_service.h>

#include <ARQNats/nats_messaging_service_interface.h>

namespace ARQ
{

IMessagingService* ARQ::createMessagingService( const std::string_view dsh )
{
    return new NatsMessagingService( dsh );
}

}