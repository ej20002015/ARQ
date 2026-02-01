#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/time.h>
#include <ARQUtils/id.h>
#include <ARQCore/refdata_entities.h>
#include <ARQCore/refdata_commands.h>
#include <ARQCore/messaging_service_interface.h>
#include <ARQCore/streaming_service.h>
#include <ARQCore/type_registry.h>
#include <ARQCore/serialiser.h>
#include <ARQUtils/logger.h>

#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>

using namespace std::string_view_literals;
using namespace std::chrono_literals;

namespace ARQ::RD
{

struct CommandResponse
{
	enum Status
	{
		_NOTSET_ = -1,
		SUCCESS,
		REJECTED,
		ERRO,
		TIMEOUT
	};

	ID::UUID                   corrID;
	Status                     status = _NOTSET_;
	std::optional<std::string> message;
};

using CommandCallback = std::function<void( const CommandResponse& resp )>;

class CommandManager
{
public:
	struct Config
	{
		std::string messagingServiceDSH           = "NATS";
		std::string streamingServiceDSH           = "Kafka";
		std::shared_ptr<Serialiser> serialiser    = SerialiserFactory::create( SerialiserFactory::SerialiserImpl::Protobuf );
		std::chrono::milliseconds checkerInterval = 1ms;
	};

public:
	CommandManager() = default;

	ARQCore_API void init( const Config& config );
	ARQCore_API void start();
	ARQCore_API void stop();

	ARQCore_API void registerOnResponseCallback( const CommandCallback& callback );

	template<Cmd::c_Command T>
	ID::UUID sendCommand( const T& cmd, const CommandCallback& callback, const Time::Milliseconds timeout = Time::Milliseconds( 1000 ) )
	{
		Buffer buf      = m_config.serialiser->serialise( cmd );
		std::string key = Cmd::Traits<T>::getKey( cmd ).toString();

		return sendCommandImpl(
			std::move( buf ),
			std::move( key ),
			Cmd::Traits<T>::name(),
			Cmd::Traits<T>::entity(),
			Cmd::Traits<T>::action(),
			callback,
			timeout
		);
	}

private:
	struct InFlightCommand
	{
		CommandCallback callback;
		Time::DateTime  startTime;
		Time::DateTime  timeoutTime;
	};

private:
	class SubHandler : public ISubscriptionHandler
	{
	public:
		SubHandler( CommandManager& owner )
			: m_owner( owner )
		{
		}

		void             onMsg( Message&& msg ) override;
		std::string_view getDesc() const        override { return "RD::CommandManager::SubHandler"sv; }

	private:
		CommandManager& m_owner;
	};

private: // Worker thread function
	void checkInFlightCommands();

private: // Helpers
	ID::UUID              sendCommandImpl( Buffer&& buf, const std::string key, const std::string_view cmdName, const std::string_view cmdEntity, const std::string_view cmdAction, const CommandCallback& callback, const Time::Milliseconds timeout );
	StreamProducerMessage formStreamMsg( Buffer&& buf, const std::string_view key, const ID::UUID& corrID, const std::string_view cmdName, const std::string_view cmdEntity, const std::string_view cmdAction );
	std::string           getStreamTopic( const std::string_view cmdEntity ) const;
	void                  createInFlightCommand( const ID::UUID& corrID, const CommandCallback& callback, const Time::Milliseconds timeout );

private: // On response handler
	void onCommandResponse( const CommandResponse& resp );

private:
	static constexpr auto SUB_TOPIC_PFX = "ARQ.RefData.Commands.Response.Session-";

private:
	Config                             m_config;
	std::shared_ptr<IMessagingService> m_messagingService;
	std::shared_ptr<IStreamProducer>   m_streamProducer;
	std::string                        m_subTopicPattern;

	std::atomic<bool> m_running;

	std::shared_ptr<SubHandler>    m_subHandler;
	std::unique_ptr<ISubscription> m_subscription;

	std::map<ID::UUID, InFlightCommand> m_inFlightCommands;
	std::shared_mutex                   m_inFlightCommandsMut;

	std::thread m_commandCheckerThread;

	std::vector<CommandCallback> m_globalCallbacks;
	std::shared_mutex            m_globalCallbacksMut;
};

}

ARQ_REG_TYPE( ARQ::RD::CommandResponse )