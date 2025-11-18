#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/buffer.h>

#include <string>
#include <map>

namespace ARQ
{

using HeaderMap = std::map<std::string, std::string>;

struct Message
{
	Buffer      data;
	std::string topic;
	HeaderMap   headers;
};

enum class MessagingEvent
{
	CONN_LOST,
	// TODO
};

struct SubscriptionEvent
{
	MessagingEvent event;
	std::string    topic;
};

class ISubscriptionHandler
{
public:
	              ARQCore_API virtual void             onMsg( Message&& msg )               = 0;
	              ARQCore_API virtual void             onEvent( SubscriptionEvent&& event ) = 0;
	[[nodiscard]] ARQCore_API virtual std::string_view getDesc() const                      = 0;
};

struct SubStats
{
	uint64_t deliveredMsgs;
	uint64_t droppedMsgs;
	uint32_t pendingMsgs;
	uint32_t pendingBytes;
	uint32_t maxPendingMsgs;
	uint32_t maxPendingBytes;
};

class ISubscription
{
public:
	virtual ~ISubscription() = default;

	[[nodiscard]] ARQCore_API virtual int64_t          getID()                      = 0;
	[[nodiscard]] ARQCore_API virtual std::string_view getTopic()                   = 0;
	[[nodiscard]] ARQCore_API virtual bool             isValid()                    = 0;
	[[nodiscard]] ARQCore_API virtual SubStats         getStats()                   = 0;
	              ARQCore_API virtual void             unsubscribe()                = 0;
	              ARQCore_API virtual void             drainAndUnsubscribe()        = 0;
	              ARQCore_API virtual void             blockOnDrainAndUnsubscribe() = 0;
};

struct GlobalStats
{
	uint64_t inMsgs;
	uint64_t inBytes;
	uint64_t outMsgs;
	uint64_t outBytes;
	uint64_t reconnects;
};

class IMessagingService
{
public:
	virtual ~IMessagingService() = default;

	              ARQCore_API virtual void                           publish( const std::string_view topic, const Message& msg )                                        = 0;
	[[nodiscard]] ARQCore_API virtual std::unique_ptr<ISubscription> subscribe( const std::string_view topicPattern, std::shared_ptr<ISubscriptionHandler> subHandler ) = 0;

	[[nodiscard]] ARQCore_API virtual GlobalStats                    getStats() const                                                                                   = 0;
};

}