#pragma once
#include <ARQNats/dll.h>

#include <ARQCore/messaging_service_interface.h>

#include "nats/nats.h"

#include <mutex>
#include <shared_mutex>

namespace ARQ
{

class NatsSubscription : public ISubscription
{
public:
	NatsSubscription( natsSubscription* const natsSub )
		: m_natsSub( natsSub )
	{
	}

	[[nodiscard]] ARQNats_API int64_t          getID()                      override;
	[[nodiscard]] ARQNats_API std::string_view getTopic()                   override;
	[[nodiscard]] ARQNats_API bool             isValid()                    override;
	[[nodiscard]] ARQNats_API SubStats         getStats()                   override;
	              ARQNats_API void             unsubscribe()                override;
	              ARQNats_API void             drainAndUnsubscribe()        override;
	              ARQNats_API void             blockOnDrainAndUnsubscribe() override;

private:
	natsSubscription* m_natsSub;
};

class NatsMessagingService : public IMessagingService
{
public:
	ARQNats_API NatsMessagingService( const std::string_view dsh );
	ARQNats_API ~NatsMessagingService();

public: // IMessagingService implementation
	              ARQNats_API void                           publish( const std::string_view topic, const Message& msg )                                        override;
	[[nodiscard]] ARQNats_API std::unique_ptr<ISubscription> subscribe( const std::string_view topicPattern, std::shared_ptr<ISubscriptionHandler> subHandler ) override;

	[[nodiscard]] ARQNats_API GlobalStats                    getStats() const                                                                                   override;

private: // Connection
	void         connect( const std::string_view dsh );
	void         disconnect();

private: // Callbacks
	void onNatsError( natsSubscription* subscription, natsStatus err );
	void onNatsClosed();
	void onNatsDisconnected();
	void onNatsReconnected();
	void onNatsDiscoveredServers();
	void onNatsLameDuck();

	friend void natsErrorCB( natsConnection* nc, natsSubscription* subscription, natsStatus err, void* closure );
	friend void natsClosedCB( natsConnection* nc, void* closure );
	friend void natsDisconnectedCB( natsConnection* nc, void* closure );
	friend void natsReconnectedCB( natsConnection* nc, void* closure );
	friend void natsDiscoveredServersCB( natsConnection* nc, void* closure );
	friend void natsLameDuckCB( natsConnection* nc, void* closure );

private:
	std::string m_dsh;

	natsConnection* m_natsConn;

	using HandlerAndTopic = std::pair<std::weak_ptr<ISubscriptionHandler>, std::string>;
	std::map<int64_t, HandlerAndTopic> m_natsSubID2HandlerAndTopic;
	std::shared_mutex                  m_natsSubID2HandlerAndTopicMutex;
};

}