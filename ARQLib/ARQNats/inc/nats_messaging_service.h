#pragma once
#include <ARQNats/dll.h>

#include <ARQCore/messaging_service.h>
#include <ARQNats/nats_messaging_service_interface.h>

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace ARQ
{

extern "C" ARQNats_API IMessagingService* createMessagingService( const std::string_view dsh );

class NatsMessagingServiceManager
{
public:
	static NatsMessagingServiceManager& inst();

	NatsMessagingService* get( const std::string_view dsh );

private:
	std::unordered_map<std::string_view, std::shared_ptr<NatsMessagingService>> m_messagingServices;
	std::shared_mutex                                                           m_messageServicesMutex;
};

}