#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/http.h>
#include <ARQUtils/cfg_wrangler.h>
#include <ARQCore/logger.h>

#include <atomic>
#include <thread>

namespace ARQ
{

namespace SvcExitCodes
{

static constexpr int UNKNOWN_ERROR   = -1;
static constexpr int SUCCESS         =  0;
static constexpr int STARTUP_ERROR   =  1;
static constexpr int RUNTIME_ERROR   =  2;
static constexpr int SHUTDOWN_ERROR  =  3;
static constexpr int ADMIN_SRV_ERROR =  4;

}

class ServiceRunner;

/**
 * @brief Abstract base class for all ARQ services.
 * * This class defines the mandatory lifecycle hooks and standard configuration
 * used by the ServiceRunner engine. Derived classes must implement the pure
 * virtual methods to define their specific business logic.
 */
class ServiceBase
{
public: // Mandatory service interface
	ARQCore_API virtual ~ServiceBase() = default;

	/**
	 * @brief Identity hook.
	 * @return The name of the service (e.g. "PricingEngine").
	 */
	ARQCore_API virtual std::string_view serviceName()        = 0;
	/**
	 * @brief Description hook.
	 * @return A human-readable description of what this service does.
	 * Displayed in help text and admin metadata.
	 */
	ARQCore_API virtual std::string_view serviceDescription() = 0;

	/**
	 * @brief Lifecycle Hook: Startup.
	 * * Called after configuration parsing but before the main run loop.
	 * Use this to initialize resources (DB connections, Kafka consumers, memory allocation).
	 * @note If this function throws or fails, the service will exit with STARTUP_ERROR.
	 */
	ARQCore_API virtual void onStartup() = 0;
	/**
	 * @brief Lifecycle Hook: Shutdown.
	 * * Called after the run loop exits (gracefully or via error).
	 * Use this to close resources, flush logs, and disconnect from middleware.
	 * @note This is guaranteed to be called if onStartup() succeeds, even if run() fails.
	 */
	ARQCore_API virtual void onShutdown() = 0;

	/**
	 * @brief Lifecycle Hook: Main Execution.
	 * * The main blocking loop of the service. This function should contain the
	 * primary business logic. It should periodically check shouldRun() to see
	 * if a shutdown signal has been received.
	 * @note This function is expected to block. When it returns, the service shuts down.
	 */
	ARQCore_API virtual void run() = 0;

public: // Optional overrides

	/**
	 * @brief Configuration Hook.
	 * * Override this to register custom command line arguments, environment variables,
	 * or config file options specific to this service.
	 * @param cfg The configuration wrangler instance to register options against.
	 */
	ARQCore_API virtual void registerConfigOptions( Cfg::ConfigWrangler& cfg ) {}

	/**
	 * @brief Admin Server Hook.
	 * * Override this to register custom HTTP endpoints on the admin server.
	 * Useful for service-specific status pages, control commands, or debugging data.
	 * @param httpServer The running HTTP server instance.
	 * @warning Callbacks registered here run on the Admin Thread, not the Main Thread.
	 * Ensure thread safety when accessing service state.
	 */
	ARQCore_API virtual void registerCustomAdminEndpoints( http::Server& httpServer ) {}

protected:
	struct BaseConfig
	{
		std::string env       = "PROD";
		LogLevel    logLevel  = DEFAULT_LOG_LEVEL;
		LogLevel    logLevel2 = DEFAULT_LOG_LEVEL;
		int32_t     adminPort = 8080;
	} m_baseConfig;

protected:
	/**
	 * @brief Checks if the service has been requested to stop.
	 * @return true if the run loop should continue, false if a signal to stop was received.
	 */
	bool shouldRun() const { return m_shouldRun; }
	/**
	 * @brief Checks the current readiness state.
	 * @return true if the service is ready to receive traffic (Readiness Probe passing).
	 */
	bool isReady()   const { return m_ready; }

	/**
	 * @brief Sets the readiness state.
	 * * Call this to update the readiness probe status exposed via the admin server.
	 * @param ready true if the service is ready to receive traffic, false otherwise.
	 */
	void setReady( const bool ready ) { m_ready = ready; }

private:
	std::atomic<bool> m_shouldRun = true;
	std::atomic<bool> m_healthy   = true;
	std::atomic<bool> m_ready     = false;
	std::atomic<bool> m_started   = false;

	friend class ServiceRunner;
};

template<typename T>
concept c_ServiceBase = std::is_base_of_v<ServiceBase, T>;

/**
 * @brief The Service Orchestrator.
 * * Manages the "boring" infrastructure of a running service:
 * - Signal Handling (SIGINT, SIGTERM)
 * - Configuration Loading (CLI -> Env -> Config File)
 * - Admin HTTP Server (Health checks, Metrics, Logging control)
 * - Exception handling and lifecycle management
 */
class ServiceRunner
{
public:

	/**
	 * @brief Static Entry Point.
	 * * Instantiates the ServiceRunner on the stack, registers the service instance,
	 * and begins the execution lifecycle.
	 * @tparam T The specific Service class (must derive from ServiceBase).
	 * @param argc Command line argument count.
	 * @param argv Command line argument vector.
	 * @return An integer exit code (see SvcExitCodes namespace).
	 */
	template<c_ServiceBase T>
	static int run( int argc, char* argv[] )
	{
		T svc;
		ServiceRunner svcRunner( svc );
		s_instPtr = &svcRunner;

		const int rc = s_instPtr->tryRunImpl( argc, argv );

		s_instPtr = nullptr;
		return rc;
	}

private: // Singleton instance
	ARQCore_API static ServiceRunner* s_instPtr;

private:
	ServiceRunner( ServiceBase& service );

	ARQCore_API int tryRunImpl( int argc, char* argv[] );
	int runImpl( int argc, char* argv[] );

	void registerSignalHandlers();

	void addConfigOptions();
	void applyCommonOptions();

	void setUpAdminServer();
	bool runAdminServer();
	void shutdownAdminServer();

	bool startupService();
	bool runService();
	bool shutdownService();

	static void handleSignal( int signum );

private:
	ServiceBase&        m_service;

	Cfg::ConfigWrangler m_cfgWrangler;

	http::Server        m_adminServer;
	std::thread         m_adminServerThread;
};

}