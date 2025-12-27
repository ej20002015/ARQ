#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/time.h>
#include <ARQUtils/id.h>
#include <ARQCore/refdata_entities.h>
#include <ARQCore/messaging_service_interface.h>
#include <ARQCore/type_registry.h>
#include <ARQCore/serialiser.h>

#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>

using namespace std::string_view_literals;
using namespace std::chrono_literals;

namespace ARQ
{

class IMessagingService;

struct RefDataCommandResponse
{
	enum Status
	{
		_NOTSET_ = -1,
		SUCCESS,
		REJECTED,
		ERROR,
		TIMEOUT
	};

	ID::UUID                   corrID;
	Status                     status = _NOTSET_;
	std::optional<std::string> message;
};

REG_ARQ_TYPE( ARQ::RefDataCommandResponse )

using RefDataCommandCallback = std::function<void( const RefDataCommandResponse& resp )>;

class RefDataCommandManager
{
public:

	struct Config
	{
		std::string messagingServiceDSH           = "NATS";
		std::shared_ptr<Serialiser> serialiser    = SerialiserFactory::create( SerialiserFactory::SerialiserImpl::Protobuf );
		std::chrono::milliseconds checkerInterval = 50ms;
	};

public:
	RefDataCommandManager() = default;

	ARQCore_API void init( const Config& config );
	ARQCore_API void start();
	ARQCore_API void stop();

	ARQCore_API void registerOnResponseCallback( const RefDataCommandCallback& callback );

	[[nodiscard]] ARQCore_API ID::UUID upsertCurrencies( const std::vector<RDEntities::Currency>& currencies, const RefDataCommandCallback& callback, const Time::Milliseconds timeout = Time::Milliseconds( 1000 ) );

private:

	struct InFlightCommand
	{
		RefDataCommandCallback callback;
		Time::DateTime startTime;
		Time::DateTime timeoutTime;
	};

private:

	class SubHandler : public ISubscriptionHandler
	{
	public:
		SubHandler( RefDataCommandManager& owner )
			: m_owner( owner )
		{
		}

		void             onMsg( Message&& msg ) override;
		std::string_view getDesc() const        override { return "RefDataCommandManager::SubHandler"sv; }

	private:
		RefDataCommandManager& m_owner;
	};

private:
	void checkInFlightCommands();
	
	void onCommandResponse( const RefDataCommandResponse& resp );

private:
	static constexpr auto SUB_TOPIC_PFX = "ARQ.RefData.Commands.Response.Session-";

private:
	Config m_config;
	std::shared_ptr<IMessagingService> m_messagingService;
	std::string m_subTopicPattern;

	std::shared_ptr<SubHandler> m_subHandler;
	std::unique_ptr<ISubscription> m_subscription;

	std::map<ID::UUID, InFlightCommand> m_inFlightCommands;
	std::shared_mutex m_inFlightCommandsMut;

	std::thread m_commandCheckerThread;
	std::atomic<bool> m_running;

	std::vector<RefDataCommandCallback> m_globalCallbacks;
	std::shared_mutex m_globalCallbacksMut;
};

}