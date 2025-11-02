#pragma once

#include <ARQUtils/time.h>
#include <ARQUtils/id.h>
#include <ARQCore/refdata_entities.h>

#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

namespace ARQ
{

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

	ID::UUID corrID;
	Status status = _NOTSET_;
	std::optional<std::string> message;
};

using RefDataCommandCallback = std::function<void( const RefDataCommandResponse& resp )>;

class RefDataCommandManager
{
public:
	RefDataCommandManager() = default;

	void init();
	void start();
	void stop();

	void registerOnResponseCallback( const RefDataCommandCallback& callback );

	ID::UUID upsertCurrencies( const std::vector<RDEntities::Currency>& currencies, const RefDataCommandCallback& callback, const Time::Milliseconds timeout = Time::Milliseconds( 1000 ) );

private:
	struct InFlightCommand
	{
		RefDataCommandCallback callback;
		Time::DateTime startTime;
		Time::DateTime timeoutTime;
	};

private:
	void checkInFlightCommands();
	
	void onCommandResponse( const RefDataCommandResponse& resp );

private:
	std::chrono::milliseconds m_checkerInterval = 50ms; // TODO: Pass in via init

	std::map<ID::UUID, InFlightCommand> m_inFlightCommands;
	std::shared_mutex m_inFlightCommandsMut;

	std::thread m_commandCheckerThread;
	std::atomic<bool> m_running;

	std::vector<RefDataCommandCallback> m_globalCallbacks;
	std::shared_mutex m_globalCallbacksMut;
};

}