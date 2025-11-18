#pragma once
#include <ARQNats/dll.h>

#include <ARQCore/messaging_service_interface.h>

#include "nats/nats.h"

namespace ARQ
{

class NatsSubscription : public ISubscription
{
public:
	[[nodiscard]] ARQNats_API int64_t          getID()                      override;
	[[nodiscard]] ARQNats_API std::string_view getTopic()                   override;
	[[nodiscard]] ARQNats_API bool             isValid()                    override;
	[[nodiscard]] ARQNats_API SubStats         getStats()                   override;
	              ARQNats_API void             unsubscribe()                override;
	              ARQNats_API void             drainAndUnsubscribe()        override;
	              ARQNats_API void             blockOnDrainAndUnsubscribe() override;
};

class NatsMessagingService : public IMessagingService
{
public:
	ARQNats_API NatsMessagingService( const std::string_view dsh );

public: // IMessagingService implementation
	              ARQNats_API void                           publish( const std::string_view topic, const Message& msg )                                        override;
	[[nodiscard]] ARQNats_API std::unique_ptr<ISubscription> subscribe( const std::string_view topicPattern, std::shared_ptr<ISubscriptionHandler> subHandler ) override;

	[[nodiscard]] ARQNats_API GlobalStats                    getStats() const                                                                                   override;

private:
	std::string m_dsh;

	natsConnection* m_natsConn;
};

}