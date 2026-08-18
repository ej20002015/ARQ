#include <ARQCore/service_base.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>
#include <string_view>
#include <vector>

using namespace ARQ;

namespace
{

enum class FailurePoint
{
	NONE,
	STARTUP,
	RUN,
	SHUTDOWN
};

class LifecycleTestService final : public ServiceBase
{
public:
	LifecycleTestService()
	{
		m_baseConfig.adminServerEnabled = false;
	}

	std::string_view serviceName() override
	{
		return "LifecycleTestService";
	}

	std::string_view serviceDescription() override
	{
		return "Exercises the readily testable ServiceRunner lifecycle";
	}

	void registerConfigOptions( Cfg::ConfigWrangler& ) override
	{
		s_events.push_back( "register-config" );
	}

	void registerCustomAdminEndpoints( http::Server& ) override
	{
		s_events.push_back( "register-admin" );
	}

	void onStartup() override
	{
		s_events.push_back( "startup" );
		s_readyDuringStartup = isReady();
		s_shouldRunDuringStartup = shouldRun();
		if( s_failurePoint == FailurePoint::STARTUP )
			throw std::runtime_error( "startup failure" );
	}

	void run() override
	{
		s_events.push_back( "run" );
		s_readyDuringRun = isReady();
		s_shouldRunDuringRun = shouldRun();
		if( s_failurePoint == FailurePoint::RUN )
			throw std::runtime_error( "runtime failure" );
	}

	void onShutdown() override
	{
		s_events.push_back( "shutdown" );
		s_readyDuringShutdown = isReady();
		if( s_failurePoint == FailurePoint::SHUTDOWN )
			throw std::runtime_error( "shutdown failure" );
	}

	static void reset( const FailurePoint failurePoint = FailurePoint::NONE )
	{
		s_failurePoint = failurePoint;
		s_events.clear();
		s_readyDuringStartup = true;
		s_readyDuringRun = false;
		s_readyDuringShutdown = true;
		s_shouldRunDuringStartup = false;
		s_shouldRunDuringRun = false;
	}

	static inline FailurePoint s_failurePoint = FailurePoint::NONE;
	static inline std::vector<std::string_view> s_events;
	static inline bool s_readyDuringStartup = true;
	static inline bool s_readyDuringRun = false;
	static inline bool s_readyDuringShutdown = true;
	static inline bool s_shouldRunDuringStartup = false;
	static inline bool s_shouldRunDuringRun = false;
};

int runLifecycleService()
{
	char executableName[] = "service-runner-test";
	char* argv[] = { executableName };
	return ServiceRunner::run<LifecycleTestService>( 1, argv );
}

}

TEST( ServiceRunnerTest, RunsLifecycleWithoutStartingDisabledAdminServer )
{
	LifecycleTestService::reset();

	EXPECT_EQ( runLifecycleService(), SvcExitCodes::SUCCESS );

	EXPECT_THAT( LifecycleTestService::s_events,
		::testing::ElementsAre( "register-config", "startup", "run", "shutdown" ) );
	EXPECT_FALSE( LifecycleTestService::s_readyDuringStartup );
	EXPECT_TRUE( LifecycleTestService::s_readyDuringRun );
	EXPECT_FALSE( LifecycleTestService::s_readyDuringShutdown );
	EXPECT_TRUE( LifecycleTestService::s_shouldRunDuringStartup );
	EXPECT_TRUE( LifecycleTestService::s_shouldRunDuringRun );
}

TEST( ServiceRunnerTest, ReportsStartupFailureWithoutRunningOrShuttingDownService )
{
	LifecycleTestService::reset( FailurePoint::STARTUP );

	EXPECT_EQ( runLifecycleService(), SvcExitCodes::STARTUP_ERROR );
	EXPECT_THAT( LifecycleTestService::s_events,
		::testing::ElementsAre( "register-config", "startup" ) );
}

TEST( ServiceRunnerTest, StillShutsDownAfterRuntimeFailure )
{
	LifecycleTestService::reset( FailurePoint::RUN );

	EXPECT_EQ( runLifecycleService(), SvcExitCodes::RUNTIME_ERROR );
	EXPECT_THAT( LifecycleTestService::s_events,
		::testing::ElementsAre( "register-config", "startup", "run", "shutdown" ) );
	EXPECT_FALSE( LifecycleTestService::s_readyDuringShutdown );
}

TEST( ServiceRunnerTest, ShutdownFailureTakesPrecedenceOverSuccessfulRun )
{
	LifecycleTestService::reset( FailurePoint::SHUTDOWN );

	EXPECT_EQ( runLifecycleService(), SvcExitCodes::SHUTDOWN_ERROR );
	EXPECT_THAT( LifecycleTestService::s_events,
		::testing::ElementsAre( "register-config", "startup", "run", "shutdown" ) );
}
