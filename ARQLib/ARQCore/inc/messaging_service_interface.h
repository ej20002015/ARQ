#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/buffer.h>

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <optional>

namespace ARQ
{

using HeaderMap = std::map<std::string, std::vector<std::string>>;

struct Message
{
	Buffer      data;
	std::string topic;
	HeaderMap   headers;
};

enum class MessagingEvent
{
	NO_SERVER,         /// Could not connect, the server could not be reached or is not running
	CONN_CLOSED,       /// Connection has been closed
	CONN_STALE,        /// The server closed our connection because it did not receive PINGs at the expected interval
	CONN_DISCONNECTED, /// The connection was disconnected. Will attempt reconnect.
	CONN_RECONNECTED,  /// The connection was reconnected
	SLOW_SUBSCRIBER,   /// The maximum number of messages waiting to be delivered has been reached. Messages are dropped.
	DRAINING           /// A subscription entered the draining mode
};

using MessagingEventCallbackFunc = std::function<void( const MessagingEvent event, const std::optional<int64_t> subscriptionID )>;

enum SubscriptionOptions
{
	NONE = 0,
	DISABLE_HEADERS = 1 << 0, /// Reading headers on every message has allocation overhead - best to disable if not needed

	DEFAULT = DISABLE_HEADERS /// Default flags include DISABLE_HEADERS
};

class ISubscriptionHandler
{
public:
	              ARQCore_API virtual void                onMsg( Message&& msg ) = 0;
	[[nodiscard]] ARQCore_API virtual std::string_view    getDesc() const        = 0;
	[[nodiscard]] ARQCore_API virtual SubscriptionOptions getSubOptions()        { return SubscriptionOptions::DEFAULT; }
};

struct SubStats
{
	int32_t  pendingMsgs;
	int32_t  pendingBytes;
	int32_t  maxPendingMsgs;
	int32_t  maxPendingBytes;
	int64_t  deliveredMsgs;
	int64_t  droppedMsgs;
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

	              ARQCore_API virtual void                           registerEventCallback( const MessagingEventCallbackFunc& eventCallbackFunc )                       = 0;

	[[nodiscard]] ARQCore_API virtual GlobalStats                    getStats() const                                                                                   = 0;
};

}