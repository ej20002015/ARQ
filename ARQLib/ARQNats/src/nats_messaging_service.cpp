#include <ARQNats/nats_messaging_service.h>

#include <ARQNats/nats_messaging_service_interface.h>

namespace ARQ
{

IMessagingService* ARQ::createMessagingService( const std::string_view dsh )
{
    return NatsMessagingServiceManager::inst().get( dsh );
}

NatsMessagingServiceManager& NatsMessagingServiceManager::inst()
{
    static NatsMessagingServiceManager s_inst;
    return s_inst;
}

NatsMessagingService* ARQ::NatsMessagingServiceManager::get( const std::string_view dsh )
{
    NatsMessagingService* svc = nullptr;
    {
        std::shared_lock<std::shared_mutex> sl( m_messageServicesMutex );
        const auto it = m_messagingServices.find( dsh );
        if( it != m_messagingServices.end() )
            svc = it->second.get();
    }

    if( !svc ) // if no such messaging svc exists yet
    {
        std::unique_lock<std::shared_mutex> ul( m_messageServicesMutex );

        // Need to check if inbetween an instance was added
        const auto it = m_messagingServices.find( dsh );
        if( it != m_messagingServices.end() )
            svc = it->second.get();
        else
        {
            auto svcSPtr = std::make_shared<NatsMessagingService>( dsh );
            const auto it = m_messagingServices.emplace( dsh, std::move( svcSPtr ) ).first;
            svc = it->second.get();
        }
    }

    return svc;
}

}